#include "stm32f10x.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "slave_config.h"
#include "slave_sensor.h"
#include "slave_protocol.h"

void DMA1_Channel6_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC6))
    {
        DMA_ClearITPendingBit(DMA1_IT_TC6);
        Slave_Protocol_OnRxDmaComplete();
    }
}

void DMA1_Channel7_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC7))
    {
        DMA_ClearITPendingBit(DMA1_IT_TC7);
        Slave_Protocol_OnTxDmaComplete();
    }
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_TC) != RESET)
    {
				Slave_Protocol_OnTxComplete();
    }
}

static volatile uint32_t g_tick = 0U;

void SysTick_Handler(void) { g_tick++; }

uint32_t millis(void) { return g_tick; }

void delay_ms(uint32_t ms)
{
    uint32_t start = g_tick;
    while ((g_tick - start) < ms) {}
}

static void Clock_Init(void);
static void GPIO_Config(void);
static void DMA_Config(void);
static void USART2_Config(void);
static void NVIC_Config(void);

int main(void)
{
    Clock_Init();

    SysTick_Config(SystemCoreClock / 1000U);

    GPIO_Config();
    DMA_Config();
    USART2_Config();
    NVIC_Config();

    Slave_Sensors_Init();
    Slave_Protocol_Init(SLAVE_ADDRESS);

    uint32_t lastReadMs = 0U;

    while (1) {
        uint32_t now = millis();
        if ((now - lastReadMs) >= SLAVE_SENSOR_READ_MS) {
            Slave_Sensors_Read();
            lastReadMs = now;
        }
        Slave_Protocol_Process();
    }
}

static void DMA_Config(void)
{
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

static void USART2_Config(void)
{
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

static void GPIO_Config(void)
{
    GPIO_InitTypeDef g;

		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);
	
    /* PA8 – RS485 DE  Out_PP  LOW */
    g.GPIO_Pin   = RS485_DE_PIN;        /* GPIO_Pin_8 */
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RS485_DE_PORT, &g);
    GPIO_WriteBit(RS485_DE_PORT, RS485_DE_PIN, Bit_RESET);

    g.GPIO_Pin   = GPIO_Pin_2;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin   = GPIO_Pin_3;
    g.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &g);
		
    g.GPIO_Pin   = GPIO_Pin_13;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &g);
		
}

static void NVIC_Config(void)
{
    NVIC_InitTypeDef n;

    n.NVIC_IRQChannel                   = DMA1_Channel6_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 1U;
    n.NVIC_IRQChannelSubPriority        = 0U;
    n.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&n);

    n.NVIC_IRQChannel                   = DMA1_Channel7_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 1U;
    n.NVIC_IRQChannelSubPriority        = 1U;
    NVIC_Init(&n);

    n.NVIC_IRQChannel                   = USART2_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 0U;
    n.NVIC_IRQChannelSubPriority        = 0U;
    NVIC_Init(&n);
}

static void Clock_Init(void)
{
    ErrorStatus status;

    RCC_HSEConfig(RCC_HSE_ON);
    status = RCC_WaitForHSEStartUp();
    if (status != SUCCESS) {
        return;
    }

		FLASH_SetLatency(FLASH_Latency_2);
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
		
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {}

    RCC_HCLKConfig(RCC_SYSCLK_Div1);    /* AHB  = 72 MHz */
    RCC_PCLK1Config(RCC_HCLK_Div2);     /* APB1 = 36 MHz */
    RCC_PCLK2Config(RCC_HCLK_Div1);     /* APB2 = 72 MHz */

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08U) {} /* 0x08 = PLL */

    SystemCoreClockUpdate();
}
