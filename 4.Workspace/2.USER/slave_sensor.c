#include "slave_sensor.h"
#include "slave_config.h"
#include "protocol_definition.h"
#include "slave_actuator.h"
#include <string.h>
#include <stdlib.h>

#if SLAVE_ADDRESS == 0x02
void delay_us(uint32_t us){
    uint32_t cycles = (SystemCoreClock / 1000000L) * us;
    uint32_t start = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < cycles);
}

void delay_ms(uint32_t ms){
    while(ms--)
        delay_us(1000);
}

#define I2C_TIMEOUT_US   2000U
#define I2C_MAX_RETRIES  3U

static uint8_t _I2C_WaitFlag(I2C_TypeDef *I2Cx,
                              uint32_t    flag,
                              FlagStatus  expected,
                              uint32_t    timeout_us)
{
    uint32_t cycles = (SystemCoreClock / 1000000UL) * timeout_us;
    uint32_t start  = DWT->CYCCNT;

    while(I2C_GetFlagStatus(I2Cx, flag) != expected){
        if((DWT->CYCCNT - start) >= cycles) return 0U;
    }
    return 1U;
}

static uint8_t _I2C_WaitEvent(I2C_TypeDef *I2Cx,
                               uint32_t    event,
                               uint32_t    timeout_us)
{
    uint32_t cycles = (SystemCoreClock / 1000000UL) * timeout_us;
    uint32_t start  = DWT->CYCCNT;

    while(!I2C_CheckEvent(I2Cx, event)){
        if((DWT->CYCCNT - start) >= cycles) return 0U;
    }
    return 1U;
}

static uint16_t s_bh1750_last_valid = 0U;

static uint8_t _BH1750_Start_Safe(void){
    if(!_I2C_WaitFlag(I2C2, I2C_FLAG_BUSY, RESET, I2C_TIMEOUT_US)) return 0U;

    I2C_GenerateSTART(I2C2, ENABLE);
    if(!_I2C_WaitEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT, I2C_TIMEOUT_US)){
        I2C_GenerateSTOP(I2C2, ENABLE);
        return 0U;
    }

    I2C_Send7bitAddress(I2C2, BH1750_I2C_ADDR, I2C_Direction_Transmitter);
    if(!_I2C_WaitEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2C_TIMEOUT_US)){
        I2C_GenerateSTOP(I2C2, ENABLE);
        return 0U;
    }

    I2C_SendData(I2C2, BH1750_I2C_CMD);
    if(!_I2C_WaitEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED, I2C_TIMEOUT_US)){
        I2C_GenerateSTOP(I2C2, ENABLE);
        return 0U;
    }

    I2C_GenerateSTOP(I2C2, ENABLE);
    return 1U;
}

void BH1750_Start(void){
    for(uint8_t attempt = 0U; attempt < I2C_MAX_RETRIES; attempt++){
        if(_BH1750_Start_Safe()) return;
    }
}

static uint8_t _BH1750_Read_Once(uint16_t *out){
    uint8_t msb, lsb;

    if(!_I2C_WaitFlag(I2C2, I2C_FLAG_BUSY, RESET, I2C_TIMEOUT_US)) return 0U;

    I2C_GenerateSTART(I2C2, ENABLE);
    if(!_I2C_WaitEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT, I2C_TIMEOUT_US)){
        I2C_GenerateSTOP(I2C2, ENABLE);
        return 0U;
    }

    I2C_Send7bitAddress(I2C2, BH1750_I2C_ADDR, I2C_Direction_Receiver);
    if(!_I2C_WaitEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2C_TIMEOUT_US)){
        I2C_GenerateSTOP(I2C2, ENABLE);
        return 0U;
    }

    if(!_I2C_WaitEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED, I2C_TIMEOUT_US)){
        I2C_GenerateSTOP(I2C2, ENABLE);
        return 0U;
    }
    msb = I2C_ReceiveData(I2C2);
		
		I2C_AcknowledgeConfig(I2C2, DISABLE);

    if(!_I2C_WaitEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED, I2C_TIMEOUT_US)){
        I2C_AcknowledgeConfig(I2C2, ENABLE);
        return 0U;
    }
    lsb = I2C_ReceiveData(I2C2);
		
    I2C_AcknowledgeConfig(I2C2, ENABLE);
    I2C_GenerateSTOP(I2C2, ENABLE);

    *out = (uint16_t)((msb << 8) | lsb);
    return 1U;
}

static void _I2C2_BusRecovery(void){
    I2C_DeInit(I2C2);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);

    GPIO_InitTypeDef g;
    g.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
    g.GPIO_Mode  = GPIO_Mode_Out_OD;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &g);

    GPIO_SetBits(GPIOB, GPIO_Pin_11);   /* SDA HIGH */

    for(uint8_t i = 0; i < 9U; i++){
        GPIO_ResetBits(GPIOB, GPIO_Pin_10);
        delay_us(10);
        GPIO_SetBits(GPIOB, GPIO_Pin_10);
        delay_us(10);
    }

    GPIO_ResetBits(GPIOB, GPIO_Pin_11);
    delay_us(10);
    GPIO_SetBits(GPIOB, GPIO_Pin_10);
    delay_us(10);
    GPIO_SetBits(GPIOB, GPIO_Pin_11);
    delay_us(10);

    g.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &g);

    I2C_InitTypeDef i2c;
    i2c.I2C_ClockSpeed          = 400000;
    i2c.I2C_Mode                = I2C_Mode_I2C;
    i2c.I2C_DutyCycle           = I2C_DutyCycle_2;
    i2c.I2C_Ack                 = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_OwnAddress1         = 0x00;
    I2C_Init(I2C2, &i2c);
    I2C_Cmd(I2C2, ENABLE);
}

uint16_t BH1750_Read(void){
    uint16_t result = 0U;

    for(uint8_t attempt = 0U; attempt < I2C_MAX_RETRIES; attempt++){
        if(_BH1750_Read_Once(&result)){
            s_bh1750_last_valid = result;
						delay_us(180000U); // Measurement Time is typically 120ms.
            return result;
        }
				_I2C2_BusRecovery();
				_BH1750_Start_Safe();
        delay_us(180000U); // Measurement Time is typically 120ms.
    }
    return s_bh1750_last_valid;
}

void DHT11_Set_Input(void){
    GPIO_InitTypeDef g;

    g.GPIO_Pin = DHT11_PIN;
    g.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    g.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(DHT11_PORT, &g);
}

void DHT11_Set_Output(void){
    GPIO_InitTypeDef g;

    g.GPIO_Pin = DHT11_PIN;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(DHT11_PORT, &g);
}

static void DHT11_Bus_Reset(void){
    DHT11_Set_Output();
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    delay_ms(2);
}

void DHT11_Start(void){
		GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
		delay_ms(18); // Delay 18ms
		GPIO_SetBits(DHT11_PORT, DHT11_PIN);
		delay_us(40); // Delay 20-40us
		DHT11_Set_Input();
}

/* Edge detection */
static uint8_t DHT11_Wait_Pin(uint8_t state, uint32_t timeout_us){
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = (SystemCoreClock / 1000000) * timeout_us;

    while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == state)
    {
        if((DWT->CYCCNT - start) > cycles) return 0;
    }
    return 1;
}

static uint8_t DHT11_Check_Response(void){
    if(!DHT11_Wait_Pin(1, 100)) return 0;

    if(!DHT11_Wait_Pin(0, 100)) return 0;

    if(!DHT11_Wait_Pin(1, 100)) return 0;

    return 1;
}

static uint8_t DHT11_Read_Byte(void){
		uint8_t data = 0;

    for(uint8_t i = 0; i < 8; i++){
        if(!DHT11_Wait_Pin(0, 100)) return 0xFF;
        delay_us(40);
        if(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)) data = (data << 1) | 1;
        else data = (data << 1);
        if(!DHT11_Wait_Pin(1, 100)) return 0xFF;
    }
    return data;
}

uint16_t dht11_temp = 0;
uint16_t dht11_humid = 0;
static bool is_dht11_valid = false;

static void DHT11_Read_Data(){
		uint8_t Rh_byte1, Rh_byte2;
    uint8_t Temp_byte1, Temp_byte2;
    uint8_t checksum;

    Rh_byte1   = DHT11_Read_Byte();
    Rh_byte2   = DHT11_Read_Byte();
    Temp_byte1 = DHT11_Read_Byte();
    Temp_byte2 = DHT11_Read_Byte();
    checksum   = DHT11_Read_Byte();

    if(checksum == ((Rh_byte1 + Rh_byte2 + Temp_byte1 + Temp_byte2) & 0xFF)){
        dht11_humid = Rh_byte1;
        dht11_temp  = Temp_byte1;
				is_dht11_valid = true;
    }
		DHT11_Set_Output();
}
#elif SLAVE_ADDRESS == 0x01
uint16_t ADC_Read(uint8_t channel){
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_55Cycles5);

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)); // Possible hanging

    return ADC_GetConversionValue(ADC1);
}
#endif

#if SLAVE_ADDRESS == 0x02
uint16_t returnDht11HumidData(){
	return dht11_humid;
}

uint16_t returnDht11TempData(){
	return dht11_temp;
}
#endif

double Read_Temp_Float(void){
		#if SLAVE_ADDRESS == 0x02
				vTaskSuspendAll();
			
				DHT11_Bus_Reset();
				DHT11_Start();
				if(DHT11_Check_Response()){
						DHT11_Read_Data();
				}
				xTaskResumeAll();
				return (uint16_t)returnDht11TempData();
		#elif SLAVE_ADDRESS == 0x01
				return ((double)rand() / RAND_MAX) * 1000;
		#endif

}
double Read_Temp_Char(void)    { return ((double)rand() / RAND_MAX) * 100; }

double Read_Humid_Int(void){
		/*
		__disable_irq();
		taskENTER_CRITICAL();
		DHT11_Start();
		if(DHT11_Check_Response()){
			DHT11_Read_Data();
		}
		__enable_irq();
		taskEXIT_CRITICAL();
		*/
		#if SLAVE_ADDRESS == 0x02
				return (uint16_t)returnDht11HumidData();
		#elif SLAVE_ADDRESS == 0x01
				return ((double)rand() / RAND_MAX) * 1000;
		#endif
}
double Read_Humid_Int32(void)  { return ((double)rand() / RAND_MAX) * 1000; }

double Read_Press_Double(void) { return ((double)rand() / RAND_MAX) * 1000; }

double Read_ADC_Int32(void){
		#if SLAVE_ADDRESS == 0x01
				return ADC_Read(ADC_Channel_7);
		#elif SLAVE_ADDRESS == 0x02
				return ((double)rand() / RAND_MAX) * 1000;
		#endif
}
double Read_ADC_Int(void){
		#if SLAVE_ADDRESS == 0x01
				return ADC_Read(ADC_Channel_6);
		#elif SLAVE_ADDRESS == 0x02
				return ((double)rand() / RAND_MAX) * 1000;
		#endif
}

double Read_DI_Float(void) {
		#if SLAVE_ADDRESS == 0x02
				uint16_t raw = BH1750_Read();
				return raw / 1.2f;
		#elif SLAVE_ADDRESS == 0x01
				return ((double)rand() / RAND_MAX) * 1000;
		#endif
}
double Read_DI_Char(void)      { return ((double)rand() / RAND_MAX) * 100; }

typedef double (*SensorReadFn)(void);

typedef struct {
    eSensorType sensorType;
    eDataType dataType;
    SensorReadFn read_ptr;
} SensorDriverMap_t;

typedef struct {
    eSensorType sensorType;
    eDataType dataType;
    double warnLow;
    double warnHigh;
    double critLow;
    double critHigh;
} SensorRuleMap_t;

static const SensorRuleMap_t g_rules_map[] = {
		#if SLAVE_ADDRESS == 0x01
		{	
				SENSOR_ADC_RAW, 
				DTYPE_INT,
				MQ2_THRESHOLD_WARNINGLOW, 
				MQ2_THRESHOLD_WARNINGHIGH, 
				MQ2_THRESHOLD_CRITICALLOW,
				MQ2_THRESHOLD_CRITICALHIGH
		}
		#elif SLAVE_ADDRESS == 0x02
		{
				SENSOR_DIGITAL_IN,
				DTYPE_FLOAT,
				BH1750_THRESHOLD_WARNINGLOW, 
				BH1750_THRESHOLD_WARNINGHIGH, 
				BH1750_THRESHOLD_CRITICALLOW,
				BH1750_THRESHOLD_CRITICALHIGH
		},
		
		{
				SENSOR_TEMPERATURE,
				DTYPE_FLOAT,
				DHT11_TEMP_THRESHOLD_WARNINGLOW,
				DHT11_TEMP_THRESHOLD_WARNINGHIGH,
				DHT11_TEMP_THRESHOLD_CRITICALLOW,
				DHT11_TEMP_THRESHOLD_CRITICALHIGH,
		},
		#endif
};

const uint8_t rule_cnt = (uint8_t)(sizeof(g_rules_map) / sizeof(g_rules_map[0]));
static eActMode currentLevel[rule_cnt];

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
    { SENSOR_ADC_RAW,     DTYPE_INT32,  Read_ADC_Int32 }, // Potentiometer
    { SENSOR_ADC_RAW,     DTYPE_INT,    Read_ADC_Int },		// MQ2

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
		{ 4, SENSOR_DIGITAL_IN, DTYPE_FLOAT},
		{ 5, SENSOR_DIGITAL_IN, DTYPE_CHAR },
		{ 6, SENSOR_TEMPERATURE, DTYPE_CHAR},
		{ 7, SENSOR_HUMIDITY, DTYPE_INT},
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

static eActMode _Evaluate(const SensorRuleMap_t *r, double v){
		#if SLAVE_ADDRESS == 0x01
    if(v <= r->critLow || v >= r->critHigh) return ACT_MODE_CRITICAL;
    if(v <= r->warnLow || v >= r->warnHigh) return ACT_MODE_WARNING;
    return ACT_MODE_NORMAL;
		#elif SLAVE_ADDRESS == 0x02
    if(v >= r->critHigh) return ACT_MODE_CRITICAL;
    if(v <= r->warnLow) return ACT_MODE_WARNING;
    return ACT_MODE_NORMAL;
		#endif
}

eActMode oldLevel = ACT_MODE_NORMAL;

static void _ActorLevelCheck(eDataType dataType, eSensorType sensorType, double rawValue){
	for (uint8_t i = 0; i < rule_cnt; i++) {
		const SensorRuleMap_t* r = &g_rules_map[i];
		if(r->sensorType != sensorType) continue;
		if(r->dataType != dataType) continue;
		
		eActMode newLevel = _Evaluate(r, rawValue);
		eActMode oldLevel = currentLevel[i];
		if(oldLevel == newLevel) continue;
		else if(oldLevel != newLevel){
				currentLevel[i] = newLevel;
				ActuatorCmd_t act = {
					.sensorType = r->sensorType,
					.level = newLevel
				};
				xQueueSend(xQueue_ActuatorCmd, &act, 0);
		}
	}
}

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

void Slave_Sensors_Read(void){
    for(uint8_t i = 0; i < k_cnt; i++){
        SensorEntry_t *s = &g_sensors[i];
        double raw_value = 0.0;
        uint8_t found = 0;

        for(uint8_t j = 0; j < k_driver_cnt; j++){
            if (g_driver_map[j].sensorType == (eSensorType)s->sensorType && 
                g_driver_map[j].dataType == (eDataType)s->dataType) {
                
                raw_value = g_driver_map[j].read_ptr();
                found = 1;
                break;
            }
        }

        if(found){
            switch ((eDataType)s->dataType) {
                case DTYPE_FLOAT: s->reading.f = (float)raw_value;  break;
                case DTYPE_DOUBLE: s->reading.d = (double)raw_value; break;
                case DTYPE_INT32:s->reading.i = (int32_t)raw_value;break;
                case DTYPE_INT: s->reading.i2 = (int)raw_value;    break;
                case DTYPE_CHAR: s->reading.c = (char)raw_value;   break;
                default: break;
            }
						#if SLAVE_ADDRESS == 0x02
						if(!is_dht11_valid) continue;
						#endif
						_ActorLevelCheck((eDataType)s->dataType, (eSensorType)s->sensorType, raw_value);
        } 
				else{
						memset(&s->reading, 0, sizeof(SensorReading_t));
				}
    }
}

uint8_t Slave_Sensors_GetCount(void) { return k_cnt; }

uint8_t Slave_Sensors_PackTable(uint8_t *buf, uint8_t bufMax){
    uint8_t i;
    if((uint8_t)(1U + k_cnt * 3U) > bufMax) return 0U;
    buf[0] = k_cnt;
    for(i = 0; i < k_cnt; i++){
        buf[1U + i * 3U] 				= g_sensors[i].id;
        buf[1U + i * 3U + 1U] 	= g_sensors[i].sensorType;
        buf[1U + i * 3U + 2U] 	= g_sensors[i].dataType;
    }
    return (uint8_t)(1U + k_cnt * 3U);
}

uint8_t Slave_Sensors_PackAllData(uint8_t *buf, uint8_t bufMax){
    uint8_t pos = 0;
    for(uint8_t i = 0; i < k_cnt; i++){
        SensorEntry_t *s = &g_sensors[i];
        uint8_t sz = DataType_Size((eDataType)s->dataType);
        if ((uint8_t)(pos + 2U + sz) > bufMax) break;
        buf[pos++] = s->id;
        memcpy(&buf[pos], s->reading.bytes, sz);
        pos = (uint8_t)(pos + sz);
    }
    return pos;
}
