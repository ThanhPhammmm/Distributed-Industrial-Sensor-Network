#ifndef INC_SLAVE_CONFIG_H_
#define INC_SLAVE_CONFIG_H_

typedef enum {
    SENSOR_TEMPERATURE = 0x01,
    SENSOR_HUMIDITY = 0x02,
    SENSOR_PRESSURE = 0x03,
    SENSOR_ADC_RAW = 0x04,
    SENSOR_DIGITAL_IN = 0x05,
} eSensorType;

typedef enum {
    DTYPE_FLOAT = 0x01,
    DTYPE_INT32 = 0x02,
    DTYPE_DOUBLE = 0x03,
	DTYPE_INT = 0x04,
	DTYPE_CHAR = 0x05,
} eDataType;

typedef struct {
    uint8_t sensorId;
    uint8_t sensorType;
    uint8_t dataType;
} SensorDesc_t;

typedef union {
    float f;
    int32_t i;
    double d;
    int	i2;
    char c;

    uint8_t bytes[8];
} SensorReading_t;

#endif /* INC_SLAVE_CONFIG_H_ */
