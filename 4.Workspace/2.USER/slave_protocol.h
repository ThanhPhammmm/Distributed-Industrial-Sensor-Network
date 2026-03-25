#ifndef SLAVE_PROTOCOL_H
#define SLAVE_PROTOCOL_H

#include <stdint.h>

void Protocol_Init(uint8_t myAddr);
void Task_Protocol(void *pvParams);

void Slave_Protocol_OnRxDmaComplete(void);
void Slave_Protocol_OnTxDmaComplete(void);
void Slave_Protocol_OnTxComplete(void);

#endif
