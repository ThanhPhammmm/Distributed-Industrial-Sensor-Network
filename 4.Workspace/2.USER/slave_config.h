#ifndef SLAVE_CONFIG_H
#define SLAVE_CONFIG_H

#include "stm32f10x.h"

#define SLAVE_ADDRESS           0x01U
#define SLAVE_SENSOR_READ_MS    500
#define SLAVE_SENSOR_TABLE_HASH 0x01U

#define RS485_DE_PORT    GPIOA
#define RS485_DE_PIN     GPIO_Pin_8

#define ACT_LED_PORT     GPIOC
#define ACT_LED_PIN      GPIO_Pin_14

#define INTER_FRAME_GAP_MS  2U

#define TASK_PROTO_PRIO   2U
#define TASK_ACT_PRIO     1U

#define TASK_PROTO_STACK  256U
#define TASK_ACT_STACK    128U
 
#define RX_FRAME_QUEUE_SIZE     4U
#define ACTUATOR_CMD_QUEUE_SIZE 8U

#define RX_IDLE_TIMEOUT_MS   1500U
#endif /* SLAVE_CONFIG_H */
