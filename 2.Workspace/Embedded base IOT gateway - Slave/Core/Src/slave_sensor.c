#include "slave_sensor.h"

typedef struct {
    uint8_t         id;
    uint8_t         sensorType;   /* eSensorType */
    uint8_t         dataType;     /* eDataType   */
    SensorReading_t reading;
} SensorEntry_t;

#if   SLAVE_ADDRESS == 0x01
static SensorEntry_t g_sensors[] = {
    { .id=1, .sensorType=SENSOR_TEMPERATURE, .dataType=DTYPE_FLOAT  },
    { .id=2, .sensorType=SENSOR_HUMIDITY,    .dataType=DTYPE_FLOAT  },
    { .id=3, .sensorType=SENSOR_ADC_RAW,     .dataType=DTYPE_INT32  },
};

#elif SLAVE_ADDRESS == 0x02
static SensorEntry_t g_sensors[] = {
    { .id=1, .sensorType=SENSOR_TEMPERATURE, .dataType=DTYPE_FLOAT  },
    { .id=2, .sensorType=SENSOR_HUMIDITY,    .dataType=DTYPE_FLOAT  },
    { .id=3, .sensorType=SENSOR_PRESSURE,    .dataType=DTYPE_DOUBLE },
    { .id=4, .sensorType=SENSOR_ADC_RAW,     .dataType=DTYPE_INT32  },
};

#elif SLAVE_ADDRESS == 0x03
static SensorEntry_t g_sensors[] = {
    { .id=1, .sensorType=SENSOR_TEMPERATURE, .dataType=DTYPE_FLOAT  },
    { .id=2, .sensorType=SENSOR_HUMIDITY,    .dataType=DTYPE_FLOAT  },
    { .id=3, .sensorType=SENSOR_PRESSURE,    .dataType=DTYPE_DOUBLE },
    { .id=4, .sensorType=SENSOR_ADC_RAW,     .dataType=DTYPE_INT32  },
    { .id=5, .sensorType=SENSOR_DIGITAL_IN,  .dataType=DTYPE_INT32  },
};

#else
#error "SLAVE_ADDRESS not recognised – add sensor list for this address"
#endif
/* ─────────────────────────────────────────────────────────────────── */

static const uint8_t k_cnt = (uint8_t)(sizeof(g_sensors) / sizeof(g_sensors[0]));

uint8_t Slave_Sensors_GetCount(void) { return k_cnt; }

uint8_t Slave_Sensors_PackTable(uint8_t *buf, uint8_t bufMax){
    if ((uint8_t)(1U + k_cnt * 3U) > bufMax) return 0U;
    buf[0] = k_cnt;
    for (uint8_t i = 0; i < k_cnt; i++) {
        buf[1U + i * 3U]      = g_sensors[i].id;
        buf[1U + i * 3U + 1U] = g_sensors[i].sensorType;
        buf[1U + i * 3U + 2U] = g_sensors[i].dataType;
    }
    return (uint8_t)(1U + k_cnt * 3U);
}

uint8_t Slave_Sensors_PackAllData(uint8_t *buf, uint8_t bufMax){
    uint8_t pos = 0;
    for (uint8_t i = 0; i < k_cnt; i++) {
        SensorEntry_t *s  = &g_sensors[i];
        uint8_t        sz = DataType_Size((eDataType)s->dataType);
        if ((uint8_t)(pos + 2U + sz) > bufMax) break;   /* safety */
        buf[pos++] = s->id;
        buf[pos++] = s->dataType;
        memcpy(&buf[pos], s->reading.bytes, sz);
        pos = (uint8_t)(pos + sz);
    }
    return pos;
}



