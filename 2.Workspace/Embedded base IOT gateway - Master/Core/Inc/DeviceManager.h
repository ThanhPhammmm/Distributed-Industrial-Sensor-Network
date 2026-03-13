#ifndef INC_DEVICEMANAGER_H_
#define INC_DEVICEMANAGER_H_

#include "ProtocolDefinition.h"
#include "ProtocolTask.h"
#include "rs485_driver.h"
#include "Configuration.h"
#include "SlaveRegistry.h"
#include "system_state.h"

typedef enum { DM_IDLE = 0, DM_FETCHING = 1, DM_POLLING = 2 } eDmPhase;

void DeviceManager_Init(void);
void DeviceManager_Task(void *pvParams);
eDmPhase DeviceManager_GetState(void);

#endif /* INC_DEVICEMANAGER_H_ */
