#include "alarm.h"
#include "Configuration.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <string.h>
#include "ProtocolTask.h"

QueueHandle_t xQueue_AlarmEvent = NULL;

static const AlarmRule_t k_rules[] = {
	// Sensor 1: SENSOR_MQ2, DTYPE_INT32
	{
		.slaveAddr = 0x01,
		.sensorId = 1,
		.sensorType = SENSOR_GAS,
		.dataType = DTYPE_INT,
		.warnLow = 10,
		.warnHigh = 1500,
		.critLow = 5,
		.critHigh = 2000,
		.actuatorSlaveAddr = 0x01,
		.actuatorId = 1,
		.actuatorOnWarn = 1,
		.actuatorOnCrit = 0,
		.actuatorOnNormal = 1,
	},
	// Sensor 2: SENSOR_RESISTOR, DTYPE_INT
	{
		.slaveAddr = 0x01,
		.sensorId = 2,
		.sensorType = SENSOR_RESISTOR,
		.dataType = DTYPE_INT32,
		.warnLow = 10,
		.warnHigh = 3000,
		.critLow = 5,
		.critHigh = 4000,
		.actuatorSlaveAddr = 0x01,
		.actuatorId = 2,
		.actuatorOnWarn = 1,
		.actuatorOnCrit = 0,
		.actuatorOnNormal = 1,
	},
	// Sensor 3: SENSOR_TEMPERATURE, DTYPE_FLOAT
	{
		.slaveAddr = 0x02,
		.sensorId = 1,
		.sensorType = SENSOR_TEMPERATURE,
		.dataType = DTYPE_FLOAT,
		.warnLow = 10.0f,
		.warnHigh = 30.0f,
		.critLow = 5.0f,
		.critHigh = 50.0f,
		.actuatorSlaveAddr = 0x01,
		.actuatorId = 3,
		.actuatorOnWarn = 1,
		.actuatorOnCrit = 0,
		.actuatorOnNormal = 1,
	},
	// Sensor 4: SENSOR_HUMIDITY, DTYPE_INT
	{
		.slaveAddr = 0x02,
		.sensorId = 2,
		.sensorType = SENSOR_HUMIDITY,
		.dataType = DTYPE_INT,
		.warnLow = 10,
		.warnHigh = 100,
		.critLow = 5,
		.critHigh = 200,
		.actuatorSlaveAddr = 0x01,
		.actuatorId = 4,
		.actuatorOnWarn = 1,
		.actuatorOnCrit = 0,
		.actuatorOnNormal = 1,
	},
	// Sensor 5: SENSOR_LIGHT, DTYPE_FLOAT
	{
		.slaveAddr = 0x02,
		.sensorId = 3,
		.sensorType = SENSOR_LIGHT,
		.dataType = DTYPE_FLOAT,
		.warnLow = 10.0f,
		.warnHigh = 1500.0f,
		.critLow = 5.0f,
		.critHigh = 2000.0f,
		.actuatorSlaveAddr = 0x01,
		.actuatorId = 5,
		.actuatorOnWarn = 1,
		.actuatorOnCrit = 0,
		.actuatorOnNormal = 1,
	},
};

#define RULE_COUNT  (sizeof(k_rules) / sizeof(k_rules[0]))

static eAlarmLevel g_currentLevel[RULE_COUNT];

//static void _TriggerActuator(const AlarmRule_t *r, uint8_t value, eAlarmLevel level){
//    extern QueueHandle_t xQueue_TxCmd;
//    extern TaskHandle_t g_protocolTaskHandle;
//
//    TxCmd_t tx;
//    memset(&tx, 0, sizeof(tx));
//    tx.addr 		= r->actuatorSlaveAddr;
//    tx.cmd 			= CMD_SET_ACTUATOR;
//    tx.payloadLen 	= 3U;
//    tx.payload[0] 	= r->actuatorId;
//    tx.payload[1] 	= value;   /* valueType: ON/OFF */
//    tx.payload[2] 	= level;
//
//    xQueueSend(xQueue_TxCmd, &tx, pdMS_TO_TICKS(10));
//    xTaskNotifyGive(g_protocolTaskHandle);
//}

static eAlarmLevel _Evaluate(const AlarmRule_t *r, float v){
    if(v <= r->critLow || v >= r->critHigh) return ALARM_CRITICAL;
    if(v <= r->warnLow || v >= r->warnHigh) return ALARM_WARN;
    return ALARM_NONE;
}

void Alarm_Init(void){
    xQueue_AlarmEvent = xQueueCreate(8, sizeof(AlarmEvent_t));
    memset(g_currentLevel, ALARM_NONE, sizeof(g_currentLevel));
}

void Alarm_Check(uint8_t slaveAddr, uint8_t sensorId, uint8_t sensorType, eDataType dt, SensorReading_t readingData){
    float v = 0.0f;
    switch(dt){
		case DTYPE_FLOAT:
			v = readingData.f;
			break;
		case DTYPE_INT32:
			v = (float)readingData.i;
			break;
		case DTYPE_DOUBLE:
			v = (float)readingData.d;
			break;
		case DTYPE_INT:
			v = (float)readingData.i2;
			break;
		case DTYPE_CHAR:
			v = (float)readingData.c;
			break;
		default: return;
    }

    for(uint8_t i = 0; i < RULE_COUNT; i++){
        const AlarmRule_t *r = &k_rules[i];
        if(r->slaveAddr != slaveAddr) continue;
        if(r->sensorId != sensorId) continue;
        if(r->sensorType != sensorType) continue;
        if(r->dataType != dt) continue;

        eAlarmLevel newLevel = _Evaluate(r, v);
        eAlarmLevel oldLevel = g_currentLevel[i];
	    if(newLevel == oldLevel) continue;

        g_currentLevel[i] = newLevel;

//        switch (newLevel) {
//        case ALARM_CRITICAL: _TriggerActuator(r, r->actuatorOnCrit, ALARM_CRITICAL); break;
//        case ALARM_WARN: _TriggerActuator(r, r->actuatorOnWarn, ALARM_WARN); break;
//        case ALARM_NONE: _TriggerActuator(r, r->actuatorOnNormal, ALARM_NONE); break;
//        default: break;
//        }

        AlarmEvent_t ev = {
            .slaveAddr = slaveAddr,
            .sensorId = sensorId,
            .sensorType = sensorType,
            .level = newLevel,
            .dataType = dt,
            .readingData = readingData,
            //.timestampMs = xTaskGetTickCount(),
        };
        xQueueSend(xQueue_AlarmEvent, &ev, 0);
    }
}
