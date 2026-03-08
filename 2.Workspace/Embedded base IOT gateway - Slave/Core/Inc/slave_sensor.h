#ifndef INC_SLAVE_SENSOR_H_
#define INC_SLAVE_SENSOR_H_

#include <string.h>
#include "slave_config.h"
#include "stdint.h"
#include "ProtocolDefinition.h"

uint8_t Slave_Sensors_GetCount(void);
uint8_t Slave_Sensors_PackTable(uint8_t *buf, uint8_t bufMax);
uint8_t Slave_Sensors_PackAllData(uint8_t *buf, uint8_t bufMax);

#endif /* INC_SLAVE_SENSOR_H_ */
