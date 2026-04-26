#ifndef INC_SLAVEREGISTRY_H_
#define INC_SLAVEREGISTRY_H_

#include <stdint.h>
#include <stdbool.h>
#include "Configuration.h"
#include "slave_config.h"

typedef enum {
    SREG_UNREGISTERED 	= 0,  /* slot inactive                       */
    SREG_DECLARED 		= 1,  /* registered, waiting for RUN fetch   */
    SREG_FETCHING 		= 2,  /* ping / table fetch in progress      */
    SREG_READY 			= 3,  /* table valid, ready to poll          */
    SREG_ONLINE 		= 4,  /* polling, data arriving              */
    SREG_OFFLINE 		= 5,  /* too many consecutive missed polls   */
    SREG_ERROR 			= 6,  /* ping / fetch failed                 */
} eSlaveRegState;

typedef struct {
    uint32_t totalPolls;
    uint32_t timeouts;
    uint32_t nacks;
} SlaveStats_t;

typedef struct {
    uint8_t addr;
    bool registered;
    eSlaveRegState state;
    uint8_t configVersion;
    uint8_t sensorCount;
    SensorDesc_t sensors[MAX_SENSORS_PER_SLAVE];
    SensorReading_t lastReading[MAX_SENSORS_PER_SLAVE]; /* index = sensorId-1 */
    uint32_t lastSeenMs;
    uint8_t missedPolls;
    SlaveStats_t stats;
} SlaveSlot_t;

void Registry_Init(void);
SlaveSlot_t Registry_GetSlot(uint8_t slotIdx);
void Registry_SetState(uint8_t idx, eSlaveRegState s);
void Registry_SetSensorTable(uint8_t idx, uint8_t count, const SensorDesc_t *d);
void Registry_SetConfigVersion(uint8_t idx, uint8_t ver);
//void Registry_UpdateReading(uint8_t idx, uint8_t sensorId, eDataType dt, SensorReading_t reading);
void Registry_SetLastSeen(uint8_t idx, uint32_t ms);
void Registry_SetMissedPolls(uint8_t idx, uint8_t n);
void Registry_IncrementPoll(uint8_t idx);
void Registry_SetRegistered(uint8_t idx, bool value);
void Registry_IncrementNack(uint8_t idx);
uint8_t Registry_GetRegisteredCount(void);
uint8_t Registry_CountByState(eSlaveRegState s);
bool Registry_IsRegistered(uint8_t idx);
bool Registry_Toggle(uint8_t idx);
void Registry_ResetForRun(void);
void Registry_IncrementTimeout(uint8_t idx);
uint8_t Registry_GetAddr(uint8_t idx);
void Registry_SetSensorCount(uint8_t idx, uint8_t count);
uint8_t Registry_GetSensorCount(uint8_t idx);
uint8_t Registry_GetOnlineCount(void);
void Registry_UpdateReading(uint8_t idx, uint8_t sensorId, SensorReading_t reading);

#endif /* INC_SLAVEREGISTRY_H_ */
