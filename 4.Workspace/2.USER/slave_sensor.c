#include "slave_sensor.h"
#include "slave_config.h"
#include "protocol_definition.h"
#include <string.h>

typedef struct {
    uint8_t id;
    uint8_t sensorType;
    uint8_t dataType;
    SensorReading_t reading;
} SensorEntry_t;

#if SLAVE_ADDRESS == 0x01
static SensorEntry_t g_sensors[] = {
    { 1, SENSOR_DIGITAL_IN, DTYPE_FLOAT},
    { 2, SENSOR_ADC_RAW, DTYPE_INT32},
		{ 3, SENSOR_PRESSURE, DTYPE_DOUBLE},
		{ 4, SENSOR_HUMIDITY, DTYPE_INT},
		{ 5, SENSOR_TEMPERATURE, DTYPE_CHAR},
};

#elif SLAVE_ADDRESS == 0x02
static SensorEntry_t g_sensors[] = {
    { 1, SENSOR_TEMPERATURE, DTYPE_FLOAT},
    { 2, SENSOR_HUMIDITY, DTYPE_INT32},
    { 3, SENSOR_PRESSURE, DTYPE_DOUBLE },
    { 4, SENSOR_ADC_RAW, DTYPE_INT},
		{ 5, SENSOR_DIGITAL_IN, DTYPE_CHAR },
};

#elif SLAVE_ADDRESS == 0x03
static SensorEntry_t g_sensors[] = {
    { 1, SENSOR_TEMPERATURE, DTYPE_FLOAT},
    { 2, SENSOR_HUMIDITY, DTYPE_FLOAT},
    { 3, SENSOR_PRESSURE, DTYPE_DOUBLE },
    { 4, SENSOR_ADC_RAW, DTYPE_INT32},
    { 5, SENSOR_DIGITAL_IN, DTYPE_INT32},
};

#else
#error "SLAVE_ADDRESS khong hop le"
#endif

static const uint8_t k_cnt = (uint8_t)(sizeof(g_sensors) / sizeof(g_sensors[0]));

/* -- Init ---------------------------------------------------------- */
void Slave_Sensors_Init(void){
    uint8_t i;
    for (i = 0; i < k_cnt; i++)
        memset(g_sensors[i].reading.bytes, 0, 8);
}

float temp1 = 1.0f;
int32_t temp2 = 2;

void Slave_Sensors_Read(void){
    uint8_t i;
    for (i = 0; i < k_cnt; i++) {
        SensorEntry_t *s = &g_sensors[i];
				memset(&s->reading, 0, sizeof(SensorReading_t));
			
        switch ((eDataType)s->dataType) {
        case DTYPE_FLOAT:
            switch ((eSensorType)s->sensorType) {
							case SENSOR_TEMPERATURE: 
								s->reading.f = 25.11f; 
							break;
							case SENSOR_HUMIDITY:
								s->reading.f = 62.22f; 
							break;
							case SENSOR_PRESSURE:
								s->reading.f = 98.33f;
							break;
							case SENSOR_ADC_RAW:
								s->reading.f = 101.44f;
							break;
							case SENSOR_DIGITAL_IN:
								s->reading.f = 1.55f;
							break;
							
							default:
								s->reading.f = 8.8f;
							break;
            }
            break;
        case DTYPE_DOUBLE:
            switch ((eSensorType)s->sensorType) {
							case SENSOR_TEMPERATURE: 
								s->reading.d = 11.25f; 
							break;
							case SENSOR_HUMIDITY:
								s->reading.d = 22.62f; 
							break;
							case SENSOR_PRESSURE:
								s->reading.d = 33.98f;
							break;
							case SENSOR_ADC_RAW:
								s->reading.d = 44.101f;
							break;
							case SENSOR_DIGITAL_IN:
								s->reading.d = 2.78f;
							break;
							
							default: 
								s->reading.d = 8.8f;
							break;
            }
            break;
        case DTYPE_INT32:
            switch ((eSensorType)s->sensorType) {
							case SENSOR_TEMPERATURE: 
								s->reading.i = 11; 
							break;
							case SENSOR_HUMIDITY:
								s->reading.i = 22; 
							break;
							case SENSOR_PRESSURE:
								s->reading.i = 33;
							break;
							case SENSOR_ADC_RAW:
								s->reading.i = 44;
							break;
							case SENSOR_DIGITAL_IN:
								s->reading.i = 2;
							break;
							
							default: 
								s->reading.i = 8;
							break;
            }
            break;
				case DTYPE_INT:
            switch ((eSensorType)s->sensorType) {
							case SENSOR_TEMPERATURE: 
								s->reading.i2 = 11; 
							break;
							case SENSOR_HUMIDITY:
								s->reading.i2 = 22; 
							break;
							case SENSOR_PRESSURE:
								s->reading.i2 = 33;
							break;
							case SENSOR_ADC_RAW:
								s->reading.i2 = 44;
							break;
							case SENSOR_DIGITAL_IN:
								s->reading.i2 = 2;
							break;
							
							default: 
								s->reading.i2 = 8;
							break;
            }
            break;
				case DTYPE_CHAR:
            switch ((eSensorType)s->sensorType) {
							case SENSOR_TEMPERATURE: 
								s->reading.c = 55; 
							break;
							case SENSOR_HUMIDITY:
								s->reading.c = 66; 
							break;
							case SENSOR_PRESSURE:
								s->reading.c = 77;
							break;
							case SENSOR_ADC_RAW:
								s->reading.c = 88;
							break;
							case SENSOR_DIGITAL_IN:
								s->reading.c = 99;
							break;
							
							default: 
								s->reading.c = 8;
							break;
            }
            break;
        }
    }
}

uint8_t Slave_Sensors_GetCount(void) { return k_cnt; }


uint8_t Slave_Sensors_PackTable(uint8_t *buf, uint8_t bufMax){
    uint8_t i;
    if ((uint8_t)(1U + k_cnt * 3U) > bufMax) return 0U;
    buf[0] = k_cnt;
    for (i = 0; i < k_cnt; i++) {
        buf[1U + i * 3U] = g_sensors[i].id;
        buf[1U + i * 3U + 1U] = g_sensors[i].sensorType;
        buf[1U + i * 3U + 2U] = g_sensors[i].dataType;
    }
    return (uint8_t)(1U + k_cnt * 3U);
}

uint8_t Slave_Sensors_PackAllData(uint8_t *buf, uint8_t bufMax){
    uint8_t pos = 0;
    uint8_t i;
    for (i = 0; i < k_cnt; i++) {
        SensorEntry_t *s = &g_sensors[i];
        uint8_t sz = DataType_Size((eDataType)s->dataType);
        if ((uint8_t)(pos + 2U + sz) > bufMax) break;
        buf[pos++] = s->id;
        buf[pos++] = s->dataType;
        memcpy(&buf[pos], s->reading.bytes, sz);
        pos = (uint8_t)(pos + sz);
    }
    return pos;
}
