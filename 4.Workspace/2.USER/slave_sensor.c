#include "slave_sensor.h"
#include "slave_config.h"
#include "protocol_definition.h"
#include <string.h>
#include <stdlib.h>

double Read_Temp_Float(void)   { return ((double)rand() / RAND_MAX) * 1000; }
double Read_Temp_Char(void)    { return ((double)rand() / RAND_MAX) * 100; }

double Read_Humid_Int(void)    { return ((double)rand() / RAND_MAX) * 1000; }
double Read_Humid_Int32(void)  { return ((double)rand() / RAND_MAX) * 1000; }

double Read_Press_Double(void) { return ((double)rand() / RAND_MAX) * 1000; }

double Read_ADC_Int32(void)    { return ((double)rand() / RAND_MAX) * 1000; }
double Read_ADC_Int(void)      { return ((double)rand() / RAND_MAX) * 1000; }

double Read_DI_Float(void)     { return ((double)rand() / RAND_MAX) * 1000; }
double Read_DI_Char(void)      { return ((double)rand() / RAND_MAX) * 100; }

typedef double (*SensorReadFn)(void);

typedef struct {
    eSensorType sensorType;
    eDataType dataType;
    SensorReadFn read_ptr;
} SensorDriverMap_t;

static const SensorDriverMap_t g_driver_map[] = {
    // TEMPERATURE
    { SENSOR_TEMPERATURE, DTYPE_FLOAT,  Read_Temp_Float },
    { SENSOR_TEMPERATURE, DTYPE_CHAR,   Read_Temp_Char },

    // HUMIDITY
    { SENSOR_HUMIDITY,    DTYPE_INT,    Read_Humid_Int },
    { SENSOR_HUMIDITY,    DTYPE_INT32,  Read_Humid_Int32 },

    // PRESSURE
    { SENSOR_PRESSURE,    DTYPE_DOUBLE, Read_Press_Double },

    // ADC
    { SENSOR_ADC_RAW,     DTYPE_INT32,  Read_ADC_Int32 },
    { SENSOR_ADC_RAW,     DTYPE_INT,    Read_ADC_Int },

    // DIGITAL IN
    { SENSOR_DIGITAL_IN,  DTYPE_FLOAT,  Read_DI_Float },
    { SENSOR_DIGITAL_IN,  DTYPE_CHAR,   Read_DI_Char },
};

static const uint8_t k_driver_cnt = sizeof(g_driver_map) / sizeof(g_driver_map[0]);

#if SLAVE_ADDRESS == 0x01
SensorEntry_t g_sensors[] = {
    { 1, SENSOR_DIGITAL_IN, DTYPE_FLOAT},
    { 2, SENSOR_ADC_RAW, DTYPE_INT32},
		{ 3, SENSOR_PRESSURE, DTYPE_DOUBLE},
		{ 4, SENSOR_HUMIDITY, DTYPE_INT32},
		{ 5, SENSOR_TEMPERATURE, DTYPE_FLOAT},
		{ 6, SENSOR_TEMPERATURE, DTYPE_CHAR},
		{ 7, SENSOR_ADC_RAW, DTYPE_INT},
		{ 8, SENSOR_PRESSURE, DTYPE_DOUBLE},
};

#elif SLAVE_ADDRESS == 0x02
SensorEntry_t g_sensors[] = {
    { 1, SENSOR_TEMPERATURE, DTYPE_FLOAT},
    { 2, SENSOR_HUMIDITY, DTYPE_INT32},
    { 3, SENSOR_PRESSURE, DTYPE_DOUBLE },
    { 4, SENSOR_ADC_RAW, DTYPE_INT},
		{ 5, SENSOR_DIGITAL_IN, DTYPE_FLOAT},
		{ 6, SENSOR_DIGITAL_IN, DTYPE_CHAR },
		{ 7, SENSOR_TEMPERATURE, DTYPE_CHAR},
		{ 8, SENSOR_HUMIDITY, DTYPE_INT},
};

#elif SLAVE_ADDRESS == 0x03
SensorEntry_t g_sensors[] = {
    { 1, SENSOR_TEMPERATURE, DTYPE_FLOAT},
    { 2, SENSOR_HUMIDITY, DTYPE_FLOAT},
    { 3, SENSOR_PRESSURE, DTYPE_DOUBLE },
    { 4, SENSOR_ADC_RAW, DTYPE_INT32},
    { 5, SENSOR_DIGITAL_IN, DTYPE_INT32},
};

#else
#error "SLAVE_ADDRESS khong hop le"
#endif

const uint8_t k_cnt = (uint8_t)(sizeof(g_sensors) / sizeof(g_sensors[0]));
static uint8_t g_tableVersion = 0U;


static uint8_t _CRC8_Update(uint8_t crc, uint8_t byte){
    crc ^= byte;
    for (uint8_t b = 0U; b < 8U; b++) {
        crc = (crc & 0x01U)
            ? (uint8_t)((crc >> 1U) ^ 0x8CU)
            : (uint8_t)(crc >> 1U);
    }
    return crc;
}
 
static uint8_t _ComputeTableHash(void){
    uint8_t crc = 0x00U;
 
    crc = _CRC8_Update(crc, k_cnt);
 
    for (uint8_t i = 0U; i < k_cnt; i++) {
        crc = _CRC8_Update(crc, g_sensors[i].id);
        crc = _CRC8_Update(crc, g_sensors[i].sensorType);
        crc = _CRC8_Update(crc, g_sensors[i].dataType);
    }
 
    return (crc == 0U) ? 0xFFU : crc;
}

void Slave_Sensors_Init(void){
    uint8_t i;
    for (i = 0; i < k_cnt; i++)
        memset(g_sensors[i].reading.bytes, 0, 8);
	
	  g_tableVersion = _ComputeTableHash();
}

uint8_t Slave_Sensors_GetTableVersion(void){
    return g_tableVersion;
}

void Slave_Sensors_Read(void) {
    for (uint8_t i = 0; i < k_cnt; i++) {
        SensorEntry_t *s = &g_sensors[i];
        double raw_value = 0.0;
        uint8_t found = 0;

        for (uint8_t j = 0; j < k_driver_cnt; j++) {
            if (g_driver_map[j].sensorType == (eSensorType)s->sensorType && 
                g_driver_map[j].dataType == (eDataType)s->dataType) {
                
                raw_value = g_driver_map[j].read_ptr();
                found = 1;
                break;
            }
        }

        if (found) {
            switch ((eDataType)s->dataType) {
                case DTYPE_FLOAT: s->reading.f = (float)raw_value;  break;
                case DTYPE_DOUBLE: s->reading.d = (double)raw_value; break;
                case DTYPE_INT32:s->reading.i = (int32_t)raw_value;break;
                case DTYPE_INT: s->reading.i2 = (int)raw_value;    break;
                case DTYPE_CHAR: s->reading.c = (char)raw_value;   break;
                default: break;
            }
        } 
				else {
            memset(&s->reading, 0, sizeof(SensorReading_t));
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
