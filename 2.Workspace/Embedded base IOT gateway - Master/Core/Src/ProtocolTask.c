#include "ProtocolTask.h"
#include "rs485_driver.h"
#include "watchdog.h"

QueueHandle_t xQueue_ValidFrame = NULL;
QueueHandle_t xQueue_TxCmd      = NULL;

extern QueueHandle_t xQueue_RS485_RxFrame;
extern QueueHandle_t xQueue_RS485_TxFrame;

extern TIM_HandleTypeDef htim7;

typedef struct{
	bool waiting;
    uint8_t txSeq;
    TxCmd_t pending;
    uint8_t retryCnt;
} SlaveProtoState_t;

static SlaveProtoState_t g_state[MAX_SLAVE_SLOTS];

static void _TimerStart(void){
    __HAL_TIM_SET_COUNTER(&htim7, 0);
    HAL_TIM_Base_Start_IT(&htim7);
}

static void _TimerStop(void) { HAL_TIM_Base_Stop_IT(&htim7); }

void Protocol_Init(void){
    xQueue_TxCmd      = xQueueCreate(PROTO_TXCMD_QUEUE_SIZE, sizeof(TxCmd_t));
    xQueue_ValidFrame = xQueueCreate(PROTO_VALID_FRAME_QUEUE_SIZE, sizeof(Frame_t));
    configASSERT(xQueue_TxCmd && xQueue_ValidFrame);
    memset(g_state, 0, sizeof(g_state));
}

static SlaveProtoState_t* _findSlaveByName(uint8_t addr){
    if(addr < PROTO_ADDR_MIN || addr > PROTO_ADDR_MAX) return NULL;
    return &g_state[addr - 1U];
}

static void _DoSend(SlaveProtoState_t *s){
    RS485_Send(s->pending.addr, s->txSeq,
               s->pending.cmd, STATUS_OK, s->pending.version,
               s->pending.payloadLen ? s->pending.payload : NULL,
               s->pending.payloadLen);
    s->waiting = true;
    _TimerStart();
}

void Protocol_Task(void *pvParams){

	(void)pvParams;
    Frame_t frame;
    TxCmd_t c;

	while(1){
		volatile size_t  freeHeap    = xPortGetFreeHeapSize();
		volatile UBaseType_t rxQLen  = uxQueueMessagesWaiting(xQueue_RS485_RxFrame);
		volatile UBaseType_t txQLen  = uxQueueMessagesWaiting(xQueue_RS485_TxFrame);
		volatile uint32_t    notifyCnt = ulTaskNotifyValueClear(NULL, 0);

		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(DEVMGR_LOOP_MS));
		Watchdog_Kick(WDG_TASK_PROTOCOL);

		while(xQueueReceive(xQueue_RS485_RxFrame, &frame, 0) == pdTRUE){
            if (frame.addr == 0U && frame.cmd == 0U) {
                for (int i = 0; i < PROTO_ADDR_MAX; i++) {
                	SlaveProtoState_t *s = &g_state[i];
                    if (!s->waiting) continue;
                    s->retryCnt++;
                    if (s->retryCnt <= PROTO_RETRY_MAX) {
                        _DoSend(s);
                    }
                    else {
                        s->waiting   = false;
                        s->retryCnt  = 0;
                        Frame_t ol   = { .addr   = s->pending.addr,
                                         .cmd    = 0xFFU,
                                         .status = STATUS_ERROR };
                        xQueueSend(xQueue_ValidFrame, &ol, 0);
                    }
                    break;
                }
                continue;
            }
            SlaveProtoState_t *s = _findSlaveByName(frame.addr);
            if (s == NULL || !s->waiting){
                continue;
            }

            if(frame.seq != s->txSeq){
                continue;
            }
            _TimerStop();
            s->retryCnt = 0;
            s->waiting = false;
            s->txSeq = (uint8_t)((s->txSeq + 1) & 0xFFU);
            xQueueSend(xQueue_ValidFrame, &frame, pdMS_TO_TICKS(10));
		}

		if (xQueueReceive(xQueue_TxCmd, &c, 0) == pdTRUE) {
			SlaveProtoState_t *s = _findSlaveByName(c.addr);
			if(s && !s->waiting){
				s->pending = c;
				_DoSend(s);
			}
		}
	}
}
