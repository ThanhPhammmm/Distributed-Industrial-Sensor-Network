/*
 * rs485.h
 *
 *  Created on: Mar 2, 2026
 *      Author: ThanhPham25
 */

#ifndef INC_RS485_DRIVER_H_
#define INC_RS485_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "ProtocolDefinition.h"
#include "main.h"
#include <string.h>
#include "Configuration.h"

typedef enum{
    RX_STAGE_PREFIX = 0,   /* waiting for SOF(2)+LEN(1)              */
    RX_STAGE_REST,         /* waiting for ADDR..DATA + CRC           */
}eRxStage;

extern TaskHandle_t   g_protocolTaskHandle;



typedef struct{
    uint8_t buf[PROTOCOL_FRAME_MAX];
    uint8_t len;
}RS485TxReq_t;

void RS485_Driver_Init(void);
bool RS485_Send(uint8_t        addr,
                uint8_t        seq,
                uint8_t        cmd,
                uint8_t        status,
                uint8_t        version,
                const FrameData_t *data);   /* NULL = no DATA field */
void RS485_OnRxDmaComplete(void);   /* DMA1_Stream5 */
void RS485_OnTxDmaComplete(void);   /* DMA1_Stream6 */
void RS485_Timer_IRQHandler(void);        /* TIM6         */

#endif /* INC_RS485_DRIVER_H_ */
