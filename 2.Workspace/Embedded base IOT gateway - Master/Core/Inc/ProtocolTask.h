#ifndef INC_PROTOCOLTASK_H_
#define INC_PROTOCOLTASK_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "ProtocolDefinition.h"
#include "Configuration.h"

typedef struct {
    uint8_t addr;
    uint8_t cmd;
    uint8_t version;       /* maps to VER field in frame */
    uint8_t payloadLen;
    uint8_t payload[PROTO_MAX_PAYLOAD];
} TxCmd_t;

extern QueueHandle_t xQueue_ValidFrame;  /* Protocol → DeviceManager */
extern QueueHandle_t xQueue_TxCmd;       /* DevMgr → Protocol  */

void Protocol_Init(void);
void Protocol_Task(void *pvParams);

#endif /* INC_PROTOCOLTASK_H_ */
