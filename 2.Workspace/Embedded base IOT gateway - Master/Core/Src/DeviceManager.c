#include "DeviceManager.h"

typedef enum {
    OP_NONE = 0,
    OP_PING,
    OP_GET_TABLE,
    OP_GET_ALL_DATA,
} ePendingOp;

typedef struct {
    ePendingOp op;
    uint8_t    addr;
    uint32_t   sentAtMs;
} Pending_t;

static eDmPhase  g_state = DM_IDLE;
static Pending_t g_pending = { .op = OP_NONE };

static bool _handleResponse(const Frame_t *f){
    if (g_pending.op == OP_NONE) return false;

    switch (g_pending.op) {
    	case OP_NONE:

	    case OP_PING:
			break;
	    case OP_GET_TABLE:
	    	break;
	    case OP_GET_ALL_DATA:
	    	break;
    }
	return 0;
}

void DeviceManager_Init(void){
	g_state = DM_IDLE;
	g_pending.op = OP_NONE;
}


void DeviceManager_Task(void *pvParams){
    (void)pvParams;
    Frame_t frame;

    while(1){
        const bool gotFrame = (xQueueReceive(xQueue_ValidFrame, &frame, pdMS_TO_TICKS(DEVMGR_LOOP_MS)) == pdTRUE);
        if(gotFrame){
        	if(!_handleResponse(&frame)){

        	}
        }
    }
}
