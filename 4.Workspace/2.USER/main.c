#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"

#include "slave_config.h"
#include "slave_sensor.h"
#include "slave_protocol.h"
#include "slave_actuator.h"
#include "FreeRTOS.h"
#include "task.h"
#include "slave_config_store.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName){
    volatile char *name = pcTaskName;
    (void)name;
    while(1){}
}

void vApplicationMallocFailedHook(void){
    while(1){}
}

void DMA1_Channel6_IRQHandler(void){
    if(DMA_GetITStatus(DMA1_IT_TC6)){
        DMA_ClearITPendingBit(DMA1_IT_TC6);
        Slave_Protocol_OnRxDmaComplete();
    }
}

void DMA1_Channel7_IRQHandler(void){
    if(DMA_GetITStatus(DMA1_IT_TC7)){
        DMA_ClearITPendingBit(DMA1_IT_TC7);
        Slave_Protocol_OnTxDmaComplete();
    }
}

void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_TC) != RESET){
				Slave_Protocol_OnTxComplete();
    }
}

#if SLAVE_ADDRESS == 0x02
void DWT_Init(void){
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // enable DWT
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
#endif

static void Clock_Init(void);
static void GPIO_Config(void);
static void DMA_Config(void);
static void USART2_Config(void);
static void NVIC_Config(void);
#if SLAVE_ADDRESS == 0x01
static void ADC_Config(void);
#elif SLAVE_ADDRESS == 0x02
static void I2C_Config(void);
#endif

int main(void){
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    Clock_Init();
    SysTick_Config(SystemCoreClock / 1000U);
    GPIO_Config();
    DMA_Config();
    USART2_Config();
		#if SLAVE_ADDRESS == 0x01
		ADC_Config();
		#elif SLAVE_ADDRESS == 0x02
		I2C_Config();
		#endif
    NVIC_Config();
		#if SLAVE_ADDRESS == 0x02
		DWT_Init();
		#endif
	
    Slave_Sensors_Init();
    Actuator_Init();
    Protocol_Init(SLAVE_ADDRESS);
 
    BaseType_t r;
 
    r = xTaskCreate(Task_Protocol, "Proto", TASK_PROTO_STACK, NULL, TASK_PROTO_PRIO, NULL);
    configASSERT(r == pdPASS);
 
    r = xTaskCreate(Task_Actuator, "Act", TASK_ACT_STACK, NULL, TASK_ACT_PRIO, NULL);
    configASSERT(r == pdPASS);
 
    vTaskStartScheduler();
 
    __disable_irq();
}

static void DMA_Config(void){
    DMA_InitTypeDef d;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA1_Channel6);

    d.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;
    //d.DMA_MemoryBaseAddr     = 0;
    d.DMA_DIR                = DMA_DIR_PeripheralSRC;
    //d.DMA_BufferSize         = 0;
    d.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    d.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    d.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    d.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    d.DMA_Mode               = DMA_Mode_Normal;
    d.DMA_Priority           = DMA_Priority_High;
    d.DMA_M2M                = DMA_M2M_Disable;

    DMA_Init(DMA1_Channel6, &d);

    DMA_ClearFlag(DMA1_FLAG_GL6);
    DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);

    DMA_DeInit(DMA1_Channel7);

    d.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;
    //d.DMA_MemoryBaseAddr     = 0;
    d.DMA_DIR                = DMA_DIR_PeripheralDST;
    //d.DMA_BufferSize         = 0;
    d.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    d.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    d.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    d.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    d.DMA_Mode               = DMA_Mode_Normal;
    d.DMA_Priority           = DMA_Priority_High;
    d.DMA_M2M                = DMA_M2M_Disable;

    DMA_Init(DMA1_Channel7, &d);

    DMA_ClearFlag(DMA1_FLAG_GL7);
    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);
}

static void USART2_Config(void){
    USART_InitTypeDef u;

		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
    u.USART_BaudRate            = 115200U;
    u.USART_WordLength          = USART_WordLength_8b;
    u.USART_StopBits            = USART_StopBits_1;
    u.USART_Parity              = USART_Parity_No;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
	
    USART_Init(USART2, &u);
    USART_Cmd(USART2, ENABLE);
}

static void ADC_Config(void){
	ADC_InitTypeDef a;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	a.ADC_Mode = ADC_Mode_Independent;
	a.ADC_ScanConvMode = DISABLE;
	a.ADC_ContinuousConvMode = ENABLE;
	a.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	a.ADC_DataAlign = ADC_DataAlign_Right;
	a.ADC_NbrOfChannel = 1;

	ADC_Init(ADC1, &a);
	
	#if SLAVE_ADDRESS == 0x01
	ADC_RegularChannelConfig(ADC1, MQ2_ADC_CHANNEL, 1, ADC_SampleTime_55Cycles5);	
	ADC_RegularChannelConfig(ADC1, POTENTIONMETER_ADC_CHANNEL, 1, ADC_SampleTime_55Cycles5);
	#endif

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	
	while(ADC_GetResetCalibrationStatus(ADC1));

	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

static void I2C_Config(void){
	I2C_InitTypeDef i2c;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	
	i2c.I2C_ClockSpeed 					= 400000;
	i2c.I2C_Mode 								= I2C_Mode_I2C;
	i2c.I2C_DutyCycle 					= I2C_DutyCycle_2;
	i2c.I2C_Ack 								= I2C_Ack_Enable;
	i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	i2c.I2C_OwnAddress1 = 0x00;

	I2C_Init(I2C2, &i2c);
	I2C_Cmd(I2C2, ENABLE);
}

static void GPIO_Config(void){
    GPIO_InitTypeDef g;

		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
		GPIO_StructInit(&g);
    /* PA8  RS485 DE  Out_PP  LOW */
    g.GPIO_Pin   = RS485_DE_PIN;        /* GPIO_Pin_8 */
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RS485_DE_PORT, &g);
    GPIO_WriteBit(RS485_DE_PORT, RS485_DE_PIN, Bit_RESET);
	
		GPIO_StructInit(&g);
    g.GPIO_Pin   = GPIO_Pin_2; /* UART */
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);
	
		GPIO_StructInit(&g);
    g.GPIO_Pin   = GPIO_Pin_3; /* UART */
    g.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &g);
		
		
		GPIO_StructInit(&g);
		g.GPIO_Pin   = GPIO_Pin_13;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &g);
		
		
		#if SLAVE_ADDRESS == 0x01
		GPIO_StructInit(&g);
    g.GPIO_Pin   = BUZZER_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(BUZZER_PORT, &g);
		GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
		
		GPIO_StructInit(&g);
		g.GPIO_Pin   = MQ2_ADC_PIN;
    g.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_Init(MQ2_ADC_PORT, &g);
		
		GPIO_StructInit(&g);
		g.GPIO_Pin   = POTENTIONMETER_ADC_PIN;
    g.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_Init(POTENTIONMETER_ADC_PORT, &g);
		#elif SLAVE_ADDRESS == 0x02
		GPIO_StructInit(&g);
		g.GPIO_Pin 			= BH1750_I2C_SCL_PIN;
		g.GPIO_Mode 		= GPIO_Mode_AF_OD;
		g.GPIO_Speed 		= GPIO_Speed_50MHz;
    GPIO_Init(BH1750_I2C_PORT, &g);
		
		GPIO_StructInit(&g);
		g.GPIO_Pin 			= BH1750_I2C_SDA_PIN;
		g.GPIO_Mode 		= GPIO_Mode_AF_OD;
		g.GPIO_Speed 		= GPIO_Speed_50MHz;
    GPIO_Init(BH1750_I2C_PORT, &g);
		
		GPIO_StructInit(&g);
		g.GPIO_Pin   = LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED_PORT, &g);
    GPIO_WriteBit(LED_PORT, LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN, Bit_RESET);

		GPIO_StructInit(&g);
		g.GPIO_Pin 			= DHT11_PIN;
		g.GPIO_Mode 		= GPIO_Mode_Out_PP;
		g.GPIO_Speed 		= GPIO_Speed_2MHz;
    GPIO_Init(DHT11_PORT, &g);
		GPIO_WriteBit(DHT11_PORT, DHT11_PIN, Bit_SET);

		#else
		#error "SLAVE_ADDRESS khong hop le"
		#endif
		GPIO_StructInit(&g);
		g.GPIO_Pin   = RELAY_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(RELAY_PORT, &g);
		GPIO_ResetBits(RELAY_PORT, RELAY_PIN);
}

static void NVIC_Config(void){
    NVIC_InitTypeDef n;

    n.NVIC_IRQChannel                   = DMA1_Channel6_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 6U;
    n.NVIC_IRQChannelSubPriority        = 0U;
    n.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&n);

    n.NVIC_IRQChannel                   = DMA1_Channel7_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 6U;
    n.NVIC_IRQChannelSubPriority        = 0U;
    NVIC_Init(&n);

    n.NVIC_IRQChannel                   = USART2_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 6U;
    n.NVIC_IRQChannelSubPriority        = 0U;
    NVIC_Init(&n);
}

static void Clock_Init(void){
    ErrorStatus status;

    RCC_HSEConfig(RCC_HSE_ON);
    status = RCC_WaitForHSEStartUp();
    if(status != SUCCESS){
        return;
    }

		FLASH_SetLatency(FLASH_Latency_2);
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
		
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {}

    RCC_HCLKConfig(RCC_SYSCLK_Div1);    /* AHB  = 72 MHz */
    RCC_PCLK1Config(RCC_HCLK_Div2);     /* APB1 = 36 MHz */
    RCC_PCLK2Config(RCC_HCLK_Div1);     /* APB2 = 72 MHz */

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while(RCC_GetSYSCLKSource() != 0x08U) {} /* 0x08 = PLL */

    SystemCoreClockUpdate();
}
