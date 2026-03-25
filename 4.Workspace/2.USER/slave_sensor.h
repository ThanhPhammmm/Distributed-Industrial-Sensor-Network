#ifndef SLAVE_SENSORS_H
#define SLAVE_SENSORS_H

#include <stdint.h>
#include "protocol_definition.h"

void Slave_Sensors_Init(void);
void Slave_Sensors_Read(void);

uint8_t Slave_Sensors_GetCount(void);
uint8_t Slave_Sensors_PackTable(uint8_t *buf, uint8_t bufMax);
uint8_t Slave_Sensors_PackAllData(uint8_t *buf, uint8_t bufMax);

#endif
