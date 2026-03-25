#ifndef SLAVE_ACTUATOR_H
#define SLAVE_ACTUATOR_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

typedef enum {
    ACT_MODE_NORMAL   = 0,
    ACT_MODE_CRITICAL = 1,
	ACT_MODE_WARNING  = 2,
} eActMode;

typedef struct {
    uint8_t actuatorId;
    uint8_t valueType;
    uint8_t level;
} ActuatorCmd_t;

extern QueueHandle_t xQueue_ActuatorCmd;

void Actuator_Init(void);
void Task_Actuator(void *pvParams);

#endif /* SLAVE_ACTUATOR_H */
