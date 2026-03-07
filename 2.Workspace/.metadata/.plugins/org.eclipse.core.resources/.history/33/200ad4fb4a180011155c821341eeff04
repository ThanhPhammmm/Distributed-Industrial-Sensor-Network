#ifndef INC_DEVICEMANAGER_H_
#define INC_DEVICEMANAGER_H_

#include "ProtocolDefinition.h"
#include "ProtocolTask.h"
#include "rs485_driver.h"
#include "Configuration.h"
#include "semphr.h"

typedef enum{
    DEVMGR_STATE_IDLE     = 0,
    DEVMGR_STATE_SCANNING = 1,
    DEVMGR_STATE_RUNNING  = 2,
}eDevMgrState;

typedef enum{
    SLAVE_UNKNOWN = 0,
    SLAVE_ONLINE  = 1,
    SLAVE_OFFLINE = 2,
}eSlaveState;

typedef struct{
    uint8_t      addr;
    eSlaveState  state;
    uint8_t      configVersion;
    uint8_t      sensorCount;
    SensorDesc_t sensors[MAX_SENSORS_PER_SLAVE];
} SlaveEntry_t;

void DeviceManager_Init(void);
void DeviceManager_Task(void *pvParams);

#endif /* INC_DEVICEMANAGER_H_ */
