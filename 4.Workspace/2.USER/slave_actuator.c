#include "slave_actuator.h"
#include "stm32f10x_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "slave_config.h"

#define LED_ON()     GPIO_ResetBits(ACT_LED_PORT, ACT_LED_PIN)
#define LED_OFF()    GPIO_SetBits(ACT_LED_PORT, ACT_LED_PIN)
#define LED_TOGGLE() (ACT_LED_PORT->ODR ^= ACT_LED_PIN)

#define BLINK_CRITICAL_HALF_MS  150U

QueueHandle_t xQueue_ActuatorCmd = NULL;

void Actuator_Init(void){
    xQueue_ActuatorCmd = xQueueCreate(ACTUATOR_CMD_QUEUE_SIZE, sizeof(ActuatorCmd_t));
    configASSERT(xQueue_ActuatorCmd != NULL);

    LED_OFF();
}

void Task_Actuator(void *pvParams){
    (void)pvParams;

    ActuatorCmd_t cmd;
    eActMode mode = ACT_MODE_NORMAL;
    TickType_t blinkDelay = portMAX_DELAY;

    LED_ON();

    while (1) {
        if (xQueueReceive(xQueue_ActuatorCmd, &cmd, blinkDelay) == pdTRUE) {

            if (cmd.level == ACT_MODE_CRITICAL) {
                mode = ACT_MODE_CRITICAL;
                blinkDelay = pdMS_TO_TICKS(BLINK_CRITICAL_HALF_MS);
                LED_ON();
            } 
			else if(cmd.level == ACT_MODE_NORMAL){
                mode = ACT_MODE_NORMAL;
                blinkDelay = portMAX_DELAY;
                LED_ON();
            }
			else if(cmd.level == ACT_MODE_WARNING){
				mode = ACT_MODE_WARNING;
                blinkDelay = portMAX_DELAY;
                LED_ON();
			}

        }
		else {
            if (mode == ACT_MODE_CRITICAL) {
                LED_TOGGLE();
            }
        }
    }
}
