#include "slave_sensor.h"
#include "slave_config.h"
#include "protocol_definition.h"
#include <string.h>

typedef struct {
    uint8_t         id;
    uint8_t         sensorType;
    uint8_t         dataType;
    SensorReading_t reading;
} SensorEntry_t;

#if SLAVE_ADDRESS == 0x01
static SensorEntry_t g_sensors[] = {
    { 1, SENSOR_TEMPERATURE, DTYPE_FLOAT},
    { 2, SENSOR_HUMIDITY, DTYPE_INT32},
};

#elif SLAVE_ADDRESS == 0x02
static SensorEntry_t g_sensors[] = {
    { 1, SENSOR_TEMPERATURE, DTYPE_FLOAT  },
    { 2, SENSOR_HUMIDITY,    DTYPE_FLOAT  },
    { 3, SENSOR_PRESSURE,    DTYPE_DOUBLE },
    { 4, SENSOR_ADC_RAW,     DTYPE_INT32  },
};

#elif SLAVE_ADDRESS == 0x03
static SensorEntry_t g_sensors[] = {
    { 1, SENSOR_TEMPERATURE, DTYPE_FLOAT  },
    { 2, SENSOR_HUMIDITY,    DTYPE_FLOAT  },
    { 3, SENSOR_PRESSURE,    DTYPE_DOUBLE },
    { 4, SENSOR_ADC_RAW,     DTYPE_INT32  },
    { 5, SENSOR_DIGITAL_IN,  DTYPE_INT32  },
};

#else
#error "SLAVE_ADDRESS khong hop le"
#endif

static const uint8_t k_cnt = (uint8_t)(sizeof(g_sensors) / sizeof(g_sensors[0]));

/* -- Init ---------------------------------------------------------- */
void Slave_Sensors_Init(void)
{
    uint8_t i;
    for (i = 0; i < k_cnt; i++)
        memset(g_sensors[i].reading.bytes, 0, 8);
}

float temp1 = 1.0f;
int32_t temp2 = 2;

void Slave_Sensors_Read(void)
{
    uint8_t i;
    for (i = 0; i < k_cnt; i++) {
        SensorEntry_t *s = &g_sensors[i];
				memset(&s->reading, 0, sizeof(SensorReading_t));
			
        switch ((eDataType)s->dataType) {
        case DTYPE_FLOAT:
            switch ((eSensorType)s->sensorType) {
							case SENSOR_TEMPERATURE: 
								s->reading.f = temp1++; 
							break;
							case SENSOR_HUMIDITY:    
								s->reading.f = 62.0f; 
							break;
							default:                 
								s->reading.f = 8.8f;  
							break;
            }
            break;
        case DTYPE_DOUBLE:
            s->reading.d = 101325.123;
            break;
        case DTYPE_INT32:
            switch ((eSensorType)s->sensorType) {
							case SENSOR_ADC_RAW:    
								s->reading.i = 2048; 
							break;
							case SENSOR_DIGITAL_IN: 
								s->reading.i = 1;    
							break;
							default:                
								s->reading.i = temp2++;    
							break;
            }
            break;
        }
    }
}

uint8_t Slave_Sensors_GetCount(void) { return k_cnt; }


uint8_t Slave_Sensors_PackTable(uint8_t *buf, uint8_t bufMax)
{
    uint8_t i;
    if ((uint8_t)(1U + k_cnt * 3U) > bufMax) return 0U;
    buf[0] = k_cnt;
    for (i = 0; i < k_cnt; i++) {
        buf[1U + i * 3U]      = g_sensors[i].id;
        buf[1U + i * 3U + 1U] = g_sensors[i].sensorType;
        buf[1U + i * 3U + 2U] = g_sensors[i].dataType;
    }
    return (uint8_t)(1U + k_cnt * 3U);
}

uint8_t Slave_Sensors_PackAllData(uint8_t *buf, uint8_t bufMax){
    uint8_t pos = 0;
    uint8_t i;
    for (i = 0; i < k_cnt; i++) {
        SensorEntry_t *s  = &g_sensors[i];
        uint8_t sz = DataType_Size((eDataType)s->dataType);
        if ((uint8_t)(pos + 2U + sz) > bufMax) break;
        buf[pos++] = s->id;
        buf[pos++] = s->dataType;
        memcpy(&buf[pos], s->reading.bytes, sz);
        pos = (uint8_t)(pos + sz);
    }
    return pos;
}
