#ifndef INC_UPSTREAM_H_
#define INC_UPSTREAM_H_

#include "SlaveRegistry.h"
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t xQueue_UpstreamSnapshot;

void Upstream_Init(void);
void Upstream_Task(void *pvParams);

#endif /* INC_UPSTREAM_H_ */
