#ifndef SLAVE_PROTOCOL_H
#define SLAVE_PROTOCOL_H

#include <stdint.h>

void Slave_Protocol_Init(uint8_t myAddr);

void Slave_Protocol_OnRxDmaComplete(void);
void Slave_Protocol_OnTxDmaComplete(void);
void Slave_Protocol_OnTxComplete(void);

void Slave_Protocol_Process(void);

#endif
