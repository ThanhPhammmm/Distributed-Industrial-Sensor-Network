#include "slave_protocol.h"
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "slave_config.h"
#include <string.h>
#include "protocol_definition.h"
#include "slave_sensor.h"
#include "slave_config_store.h"

#define INTER_FRAME_GAP_MS   2U

typedef enum { RX_STAGE_PREFIX = 0, RX_STAGE_REST } eRxStage;

static eRxStage g_rxStage;
static uint8_t  g_pfx[PROTO_PREFIX_SIZE];
static uint8_t  g_rest[PROTO_LEN_MAX + PROTO_CRC_SIZE];

static uint8_t  g_txBuf[PROTO_FRAME_MAX];

static volatile uint8_t g_frameReady = 0U;
static Frame_t           g_frame;
static uint8_t           g_myAddr;
static uint8_t           g_seq = 0U;

extern void delay_ms(uint32_t ms);

static void _AbortRxDma(void)
{
    USART_DMACmd(USART2, USART_DMAReq_Rx, DISABLE);
    DMA_Cmd(DMA1_Channel6, DISABLE);
    while (DMA1_Channel6->CCR & DMA_CCR1_EN) {}
    DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6 | DMA1_FLAG_HT6 | DMA1_FLAG_TE6);
}

static void _ArmPrefix(void)
{
    _AbortRxDma();
		
    DMA1_Channel6->CMAR  = (uint32_t)g_pfx;
    DMA1_Channel6->CNDTR = PROTO_PREFIX_SIZE;
    g_rxStage = RX_STAGE_PREFIX;
		DMA_ClearFlag(DMA1_FLAG_GL6);
		DMA_Cmd(DMA1_Channel6, ENABLE);
		USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
}

static void _ArmRest(uint8_t len)
{
    _AbortRxDma();
		
    DMA1_Channel6->CMAR  = (uint32_t)g_rest;
    DMA1_Channel6->CNDTR = (uint16_t)(len + PROTO_CRC_SIZE);
    g_rxStage = RX_STAGE_REST;
		DMA_ClearFlag(DMA1_FLAG_GL6);
		DMA_Cmd(DMA1_Channel6, ENABLE);
		USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
}

void Slave_Protocol_Init(uint8_t myAddr){
    g_myAddr     = myAddr;
    g_frameReady = 0U;
    _ArmPrefix();
}

static void _StartTxDma(const uint8_t *buf, uint8_t n)
{
    DMA_Cmd(DMA1_Channel7, DISABLE);
    while (DMA1_Channel7->CCR & DMA_CCR1_EN) {}
    DMA_ClearFlag(DMA1_FLAG_GL7 | DMA1_FLAG_TC7 |
                  DMA1_FLAG_HT7 | DMA1_FLAG_TE7);

    DMA1_Channel7->CMAR  = (uint32_t)buf;
    DMA1_Channel7->CNDTR = n;
			
		USART_ClearFlag(USART2, USART_FLAG_TC);
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
    DMA_Cmd(DMA1_Channel7, ENABLE);
}

static void _Send(uint8_t cmd, uint8_t status, uint8_t ver, const uint8_t *payload, uint8_t payloadLen)
{
    uint8_t n = Frame_Build(g_txBuf, g_myAddr, g_seq++, cmd, status, ver, payload, payloadLen);

    _AbortRxDma();

    GPIO_WriteBit(RS485_DE_PORT, RS485_DE_PIN, Bit_SET);
    _StartTxDma(g_txBuf, n);
}

static void _OnPing(void)
{
    uint8_t cnt = Slave_Sensors_GetCount();
    _Send(CMD_ACK, STATUS_OK, SlaveConfig_GetVersion(), &cnt, 1U);
	
		GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
		delay_ms(1);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);   // LED OFF
		delay_ms(1);
}

static void _OnGetSensorTable(void)
{
    uint8_t buf[PROTO_MAX_PAYLOAD];
    uint8_t len = Slave_Sensors_PackTable(buf, PROTO_MAX_PAYLOAD);
    if (len == 0U) { _Send(CMD_NACK, STATUS_ERROR, 0U, NULL, 0U); return; }
    _Send(CMD_SENSOR_TABLE, STATUS_OK, 0U, buf, len);
		
		GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
		delay_ms(1);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);   // LED OFF
		delay_ms(1);
}

static void _OnGetAllData(void)
{
    uint8_t buf[PROTO_MAX_PAYLOAD];
    uint8_t len = Slave_Sensors_PackAllData(buf, PROTO_MAX_PAYLOAD);
    if (len == 0) { 
			_Send(CMD_NACK, STATUS_ERROR, 0U, NULL, 0U); 
			return; 
		}
		
    delay_ms(INTER_FRAME_GAP_MS);
    _Send(CMD_ALL_DATA, STATUS_OK, 0U, buf, len);

		GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED ON
		delay_ms(1);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);   // LED OFF
		delay_ms(1);
}

void Slave_Protocol_Process(void)
{
    if (!g_frameReady) return;
    g_frameReady = 0U;

    if (g_frame.cmd == CMD_ACK || g_frame.cmd == CMD_NACK) {
        _ArmPrefix(); return;
    }

    switch (g_frame.cmd) {
    case CMD_PING:             _OnPing();           break;
    case CMD_GET_SENSOR_TABLE: _OnGetSensorTable(); break;
    case CMD_GET_ALL_DATA:     _OnGetAllData();     break;
    case CMD_RESET:
        _Send(CMD_ACK, STATUS_OK, 0U, NULL, 0U);
        delay_ms(10U);
        NVIC_SystemReset();
        break;
    default:
        _Send(CMD_NACK, STATUS_INVALID_CMD, 0U, NULL, 0U);
        break;
    }
}

void Slave_Protocol_OnRxDmaComplete(void)
{
    DMA_ClearFlag(DMA1_FLAG_TC6);

    if (g_rxStage == RX_STAGE_PREFIX) {
        if (g_pfx[0] != PROTO_SOF_0 || g_pfx[1] != PROTO_SOF_1) {
            _ArmPrefix(); return;
        }
        uint8_t len = g_pfx[2];
        if (len < PROTO_LEN_MIN || len > PROTO_LEN_MAX) {
            _ArmPrefix(); return;
        }
        _ArmRest(len);
        return;
    }

    uint8_t len   = g_pfx[2];
    uint8_t total = (uint8_t)(PROTO_PREFIX_SIZE + len + PROTO_CRC_SIZE);

		if(total < PROTO_LEN_MIN  || total > PROTO_LEN_MAX){
        _ArmPrefix();
        return;
    }
				
    static uint8_t raw[PROTO_FRAME_MAX];
    memcpy(raw,                     g_pfx,  PROTO_PREFIX_SIZE);
    memcpy(raw + PROTO_PREFIX_SIZE, g_rest, (size_t)(len + PROTO_CRC_SIZE));

    if (!Frame_ValidCRC(raw, total)) { _ArmPrefix(); return; }

    uint8_t addr = raw[3];
    if (addr != g_myAddr) {
        _ArmPrefix(); return;
    }

    Frame_Parse(raw, total, &g_frame);
    g_frameReady = 1U;
}

void Slave_Protocol_OnTxDmaComplete(void)
{
    DMA_ClearFlag(DMA1_FLAG_TC7);
    USART_DMACmd(USART2, USART_DMAReq_Tx, DISABLE);
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);
}

void Slave_Protocol_OnTxComplete(void)
{
    USART_ITConfig(USART2, USART_IT_TC, DISABLE);
    USART_ClearFlag(USART2, USART_FLAG_TC);
    GPIO_WriteBit(RS485_DE_PORT, RS485_DE_PIN, Bit_RESET);
		volatile uint8_t tmp;
		while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE))
				tmp = USART_ReceiveData(USART2);
    _ArmPrefix();
}
