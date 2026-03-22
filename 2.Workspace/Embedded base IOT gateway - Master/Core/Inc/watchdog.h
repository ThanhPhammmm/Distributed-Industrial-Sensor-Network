#ifndef INC_WATCHDOG_H_
#define INC_WATCHDOG_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

typedef enum {
    WDG_TASK_PROTOCOL = 0,
    WDG_TASK_DEVMGR,
    WDG_TASK_LCD,
    WDG_TASK_BUTTON,
	WDG_TASK_UPSTREAM,
    WDG_TASK_COUNT,
} eWdgTask;

void Watchdog_Init(void);
void Watchdog_Kick(eWdgTask task);
void Watchdog_Task(void *pvParams);

#endif /* INC_WATCHDOG_H_ */
