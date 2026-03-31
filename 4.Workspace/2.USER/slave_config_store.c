#include "slave_config_store.h"
#include "slave_sensor.h"

uint8_t SlaveConfig_GetVersion(void){
    return Slave_Sensors_GetTableVersion();
}
