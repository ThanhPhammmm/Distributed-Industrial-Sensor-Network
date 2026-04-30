#include "slave_protocol.h"
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "slave_config.h"
#include "protocol_definition.h"
#include "slave_sensor.h"
#include "slave_config_store.h"
#include "slave_actuator.h"
#include <string.h>


typedef enum { RX_STAGE_PREFIX = 0, RX_STAGE_REST } eRxStage;
static eRxStage g_rxStage;
static uint8_t g_pfx[PROTO_PREFIX_SIZE];
static uint8_t g_rest[PROTO_LEN_MAX + PROTO_CRC_SIZE];

static uint8_t g_txBuf[PROTO_FRAME_MAX];

static uint8_t g_myAddr;

static QueueHandle_t xQueue_RxFrame = NULL;

static void _AbortRxDma(void){
    USART_DMACmd(USART2, USART_DMAReq_Rx, DISABLE);
    DMA_Cmd(DMA1_Channel6, DISABLE);
    while(DMA1_Channel6->CCR & DMA_CCR1_EN) {}
    DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6 | DMA1_FLAG_HT6 | DMA1_FLAG_TE6);
}

static void _ArmPrefix(void){
    _AbortRxDma();
    DMA1_Channel6->CMAR = (uint32_t)g_pfx;
    DMA1_Channel6->CNDTR = PROTO_PREFIX_SIZE;
    g_rxStage = RX_STAGE_PREFIX;
    DMA_ClearFlag(DMA1_FLAG_GL6);
    DMA_Cmd(DMA1_Channel6, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
}

static void _ArmRest(uint8_t len){
    _AbortRxDma();
    DMA1_Channel6->CMAR = (uint32_t)g_rest;
    DMA1_Channel6->CNDTR = (uint16_t)(len + PROTO_CRC_SIZE);
    g_rxStage = RX_STAGE_REST;
    DMA_ClearFlag(DMA1_FLAG_GL6);
    DMA_Cmd(DMA1_Channel6, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
}

static void _StartTxDma(const uint8_t *buf, uint8_t n){
    DMA_Cmd(DMA1_Channel7, DISABLE);
    while(DMA1_Channel7->CCR & DMA_CCR1_EN) {}
    DMA_ClearFlag(DMA1_FLAG_GL7 | DMA1_FLAG_TC7 | DMA1_FLAG_HT7 | DMA1_FLAG_TE7);
    DMA1_Channel7->CMAR = (uint32_t)buf;
    DMA1_Channel7->CNDTR = n;
    USART_ClearFlag(USART2, USART_FLAG_TC);
		DMA_Cmd(DMA1_Channel7, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
}

static void _Send(uint8_t seq, uint8_t cmd, uint8_t status, uint8_t ver,
                  const uint8_t *payload, uint8_t payloadLen)
{
    uint8_t n = Frame_Build(g_txBuf, g_myAddr, seq, cmd, status, ver, payload, payloadLen);
    _AbortRxDma();
    GPIO_WriteBit(RS485_DE_PORT, RS485_DE_PIN, Bit_SET);
    _StartTxDma(g_txBuf, n);
}


static void _ProcessFrame(const Frame_t *f){
    switch (f->cmd) {

    case CMD_PING: {
        uint8_t cnt = Slave_Sensors_GetCount();
        _Send(f->seq, CMD_ACK, STATUS_OK, SlaveConfig_GetVersion(), &cnt, 1U);
			
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
            vTaskDelay(1);
            GPIO_SetBits(GPIOC, GPIO_Pin_13); // LED OFF
            vTaskDelay(1);
			
        break;
    }

    case CMD_GET_SENSOR_TABLE: {
        uint8_t buf[PROTO_MAX_PAYLOAD];
        uint8_t len = Slave_Sensors_PackTable(buf, PROTO_MAX_PAYLOAD);
        if(len == 0U){
            _Send(f->seq, CMD_NACK, STATUS_ERROR, SlaveConfig_GetVersion(), NULL, 0U);
        } 
		else {
            _Send(f->seq, CMD_SENSOR_TABLE, STATUS_OK, SlaveConfig_GetVersion(), buf, len);
        }
				
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
            vTaskDelay(1);
            GPIO_SetBits(GPIOC, GPIO_Pin_13); // LED OFF
            vTaskDelay(1);
				
        break;
    }

    case CMD_GET_ALL_DATA: {
        uint8_t buf[PROTO_MAX_PAYLOAD];
        uint8_t len = Slave_Sensors_PackAllData(buf, PROTO_MAX_PAYLOAD);
        if(len == 0U){
            _Send(f->seq, CMD_NACK, STATUS_ERROR, SlaveConfig_GetVersion(), NULL, 0U);
        } 
				else {
            _Send(f->seq, CMD_ALL_DATA, STATUS_OK, SlaveConfig_GetVersion(), buf, len);
        }
				
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
            vTaskDelay(1);
            GPIO_SetBits(GPIOC, GPIO_Pin_13); // LED OFF
            vTaskDelay(1);
				
        break;
    }

		/**
    case CMD_SET_ACTUATOR: {
        if (f->payloadLen >= 3U) {
            ActuatorCmd_t act;
            act.actuatorId = f->payload[0];
            act.valueType = f->payload[1];
            act.level = f->payload[2];
            xQueueSend(xQueue_ActuatorCmd, &act, 0U);
        }
        //_Send(f->seq, CMD_ACK, STATUS_OK, 0U, NULL, 0U);
        break;
    }
		**/

    case CMD_RESET: {
        _Send(f->seq, CMD_ACK, STATUS_OK, 0U, NULL, 0U);
        vTaskDelay(pdMS_TO_TICKS(10U));
        NVIC_SystemReset();
        break;
    }

    default: {
        _Send(f->seq, CMD_NACK, STATUS_INVALID_CMD, 0U, NULL, 0U);
        break;
    }
    }
}

void Protocol_Init(uint8_t myAddr){
    g_myAddr = myAddr;
    xQueue_RxFrame = xQueueCreate(RX_FRAME_QUEUE_SIZE, sizeof(Frame_t));
    configASSERT(xQueue_RxFrame != NULL);
    _ArmPrefix();
}

static void _FlushAndRearm(void){
    DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, DISABLE);

    _AbortRxDma();

    while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE)){
        (void)USART_ReceiveData(USART2);
    }
    if(USART_GetFlagStatus(USART2, USART_FLAG_ORE)){
        (void)USART_ReceiveData(USART2);
    }

    Frame_t f;
    while(xQueueReceive(xQueue_RxFrame, &f, 0) == pdTRUE) {}

    DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);
    _ArmPrefix();
}

/*
#if SLAVE_ADDRESS == 0x02
static void BH1750_Start(void){
    while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));

    I2C_GenerateSTART(I2C2, ENABLE);
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C2, BH1750_I2C_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C2, BH1750_I2C_CMD); // Continuous H-Resolution
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C2, ENABLE);
}
#endif
*/

extern void BH1750_Start(void);
void Task_Protocol(void *pvParams){
    (void)pvParams;

    Frame_t frame;
	  TickType_t lastFrameMs   = xTaskGetTickCount();
    TickType_t lastSensorTick = xTaskGetTickCount();

		#if SLAVE_ADDRESS == 0x02
		BH1750_Start();
		#endif
    while(1){
		if(xQueueReceive(xQueue_RxFrame, &frame, pdMS_TO_TICKS(10U)) == pdTRUE){
            lastFrameMs = xTaskGetTickCount();
            if(frame.cmd == CMD_ACK || frame.cmd == CMD_NACK){
                _ArmPrefix();
            }
            else{
                _ProcessFrame(&frame);
            }
        }

		TickType_t now = xTaskGetTickCount();
				
        if((now - lastFrameMs) >= pdMS_TO_TICKS(RX_IDLE_TIMEOUT_MS)){
            _FlushAndRearm();
            lastFrameMs = now;
        }
				
        if((now - lastSensorTick) >= pdMS_TO_TICKS(SLAVE_SENSOR_READ_MS)){
            Slave_Sensors_Read();
            lastSensorTick = now;
        }
    }
}

void Slave_Protocol_OnRxDmaComplete(void){
    DMA_ClearFlag(DMA1_FLAG_TC6);

    if(g_rxStage == RX_STAGE_PREFIX){
        if(g_pfx[0] != PROTO_SOF_0 || g_pfx[1] != PROTO_SOF_1){
            _ArmPrefix();
            return;
        }
        uint8_t len = g_pfx[2];
        if(len < PROTO_LEN_MIN || len > PROTO_LEN_MAX){
            _ArmPrefix();
            return;
        }
        _ArmRest(len);
        return;
    }

    uint8_t len = g_pfx[2];
    uint8_t total = (uint8_t)(PROTO_PREFIX_SIZE + len + PROTO_CRC_SIZE);

    static uint8_t raw[PROTO_FRAME_MAX];
    memcpy(raw, g_pfx, PROTO_PREFIX_SIZE);
    memcpy(raw + PROTO_PREFIX_SIZE, g_rest, (size_t)(len + PROTO_CRC_SIZE));

    if(!Frame_ValidCRC(raw, total)){
        _ArmPrefix();
        return;
    }

    if(raw[3] != g_myAddr){
        _ArmPrefix();
        return;
    }

    Frame_t f;
    Frame_Parse(raw, total, &f);

    BaseType_t woken = pdFALSE;
    if(xQueueSendFromISR(xQueue_RxFrame, &f, &woken) != pdTRUE){
        _ArmPrefix();
    }
    portYIELD_FROM_ISR(woken);
}
                           
void Slave_Protocol_OnTxDmaComplete(void){
    DMA_ClearFlag(DMA1_FLAG_TC7);
    USART_DMACmd(USART2, USART_DMAReq_Tx, DISABLE);
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);
}

void Slave_Protocol_OnTxComplete(void){
    USART_ITConfig(USART2, USART_IT_TC, DISABLE);
    USART_ClearFlag(USART2, USART_FLAG_TC);
    GPIO_WriteBit(RS485_DE_PORT, RS485_DE_PIN, Bit_RESET);

    volatile uint8_t tmp;
    while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE)) {
        tmp = (uint8_t)USART_ReceiveData(USART2);
    }
    (void)tmp;

    _ArmPrefix();
}
