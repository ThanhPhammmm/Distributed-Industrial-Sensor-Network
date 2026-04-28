#include "slave_actuator.h"
#include "stm32f10x_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "slave_config.h"

#if SLAVE_ADDRESS == 0x01
void Buzzer_On(void){
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_Off(void){
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
}
#endif

void Relay_On(void){
    GPIO_SetBits(RELAY_PORT, RELAY_PIN);
	  //GPIO_ResetBits(RELAY_PORT, RELAY_PIN);
}

void Relay_Off(void){
		GPIO_ResetBits(RELAY_PORT, RELAY_PIN);
	  //GPIO_SetBits(RELAY_PORT, RELAY_PIN);
}

#if SLAVE_ADDRESS == 0x02
void Led_On_Red(void){
		GPIO_SetBits(LED_PORT, LED_RED_PIN);
}

void Led_On_Green(void){
		GPIO_SetBits(LED_PORT, LED_GREEN_PIN);
		//GPIO_ResetBits(LED_PORT, LED_GREEN_PIN);
}

void Led_On_Blue(void){
		GPIO_SetBits(LED_PORT, LED_BLUE_PIN);
		//GPIO_ResetBits(LED_PORT, LED_BLUE_PIN);	
}

void Led_Off_All(void){
		GPIO_ResetBits(LED_PORT, LED_RED_PIN);
		GPIO_ResetBits(LED_PORT, LED_GREEN_PIN);
		GPIO_ResetBits(LED_PORT, LED_BLUE_PIN);
}

#endif

QueueHandle_t xQueue_ActuatorCmd = NULL;

void Actuator_Init(void){
    xQueue_ActuatorCmd = xQueueCreate(ACTUATOR_CMD_QUEUE_SIZE, sizeof(ActuatorCmd_t));
    configASSERT(xQueue_ActuatorCmd != NULL);
}

void Task_Actuator(void *pvParams){
    (void)pvParams;

    ActuatorCmd_t cmd;
    TickType_t actuatorDelay = portMAX_DELAY;

    while(1){
				if(xQueueReceive(xQueue_ActuatorCmd, &cmd, actuatorDelay) == pdTRUE){
						if(cmd.level == ACT_MODE_CRITICAL){
								#if SLAVE_ADDRESS == 0x01
								Buzzer_On();
								Relay_On();
								#elif SLAVE_ADDRESS == 0x02
								if(cmd.sensorType == SENSOR_DIGITAL_IN){
										Led_Off_All();
										Led_On_Red();
										vTaskDelay(100);
								}
								if(cmd.sensorType == SENSOR_TEMPERATURE){
										Relay_On();
								}
								#endif
						} 
						else if(cmd.level == ACT_MODE_NORMAL){
								#if SLAVE_ADDRESS == 0x01
										Buzzer_Off();
										Relay_Off ();
								#elif SLAVE_ADDRESS == 0x02
								if(cmd.sensorType == SENSOR_DIGITAL_IN){
										Led_Off_All();
								}
								if(cmd.sensorType == SENSOR_TEMPERATURE){
										Relay_Off ();
								}
								#endif
						}
						else if(cmd.level == ACT_MODE_WARNING){
								#if SLAVE_ADDRESS == 0x01
										Buzzer_Off();
										Relay_On();
								#elif SLAVE_ADDRESS == 0x02
								if(cmd.sensorType == SENSOR_DIGITAL_IN){
										Led_Off_All();
										Led_On_Blue();	
										vTaskDelay(100);
										//Led_Off_All();
										Led_On_Green();
										vTaskDelay(100);
										//Led_Off_All();
								}
								if(cmd.sensorType == SENSOR_TEMPERATURE){
										Relay_On();
								}
								#endif
						}
					}
					else{
					if (cmd.level == ACT_MODE_CRITICAL){
							#if SLAVE_ADDRESS == 0x01
									Buzzer_On();
							#endif		
									Relay_On();
					}
					else if(cmd.level == ACT_MODE_NORMAL){
							#if SLAVE_ADDRESS == 0x01
									Buzzer_Off();
									Relay_Off ();
							#endif
					}
					else if(cmd.level == ACT_MODE_WARNING){
							Relay_On();
							#if SLAVE_ADDRESS == 0x01
									Buzzer_Off();
							#endif
					}
			}
    }
}
