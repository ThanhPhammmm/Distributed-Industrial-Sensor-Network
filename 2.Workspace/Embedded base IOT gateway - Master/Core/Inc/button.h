#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "Configuration.h"

typedef enum {
	BTN_UP = 0,
	BTN_DOWN = 1,
	BTN_OK = 2
} eBtn;

typedef enum {
	BTN_SHORT = 0,
	BTN_LONG = 1
} eBtnType;

typedef struct {
    eBtn    btn;
    eBtnType type;
} BtnEvent_t;

extern QueueHandle_t xQueue_BtnEvent;

void Button_Init(void);
void Button_Task(void *pvParams);

#endif /* INC_BUTTON_H_ */
