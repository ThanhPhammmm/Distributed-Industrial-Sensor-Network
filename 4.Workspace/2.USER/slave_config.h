#ifndef SLAVE_CONFIG_H
#define SLAVE_CONFIG_H

#include "stm32f10x.h"

#define SLAVE_ADDRESS           0x01U
#define SLAVE_SENSOR_READ_MS    200U
#define SLAVE_SENSOR_TABLE_HASH 0x01U

#define RS485_DE_PORT    GPIOA
#define RS485_DE_PIN     GPIO_Pin_8


#endif /* SLAVE_CONFIG_H */
