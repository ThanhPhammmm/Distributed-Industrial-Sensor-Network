#ifndef INC_ALARM_H_
#define INC_ALARM_H_

#include <stdint.h>
#include <stdbool.h>
#include "ProtocolDefinition.h"
#include "FreeRTOS.h"
#include "queue.h"

typedef enum {
    ALARM_NONE = 0,
    ALARM_WARN = 1,
    ALARM_CRITICAL = 2,
} eAlarmLevel;

typedef struct {
    uint8_t slaveAddr;
    uint8_t sensorId;
    uint8_t sensorType;

    float warnLow;
    float warnHigh;
    float critLow;
    float critHigh;

    uint8_t actuatorSlaveAddr;
    uint8_t actuatorId;
    uint8_t actuatorOnWarn;
    uint8_t actuatorOnCrit;
    uint8_t actuatorOnNormal;
} AlarmRule_t;

typedef struct {
    uint8_t slaveAddr;
    uint8_t sensorId;
    uint8_t sensorType;
    eAlarmLevel level;
    eDataType dataType;
    SensorReading_t readingData;
    //uint32_t timestampMs;
} AlarmEvent_t;

extern QueueHandle_t xQueue_AlarmEvent;

void Alarm_Init(void);
void Alarm_Check(uint8_t slaveAddr, uint8_t sensorId, uint8_t sensorType, eDataType dt, SensorReading_t reading);

#endif /* INC_ALARM_H_ */
