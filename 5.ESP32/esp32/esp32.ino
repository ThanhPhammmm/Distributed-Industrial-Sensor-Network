#include <Arduino.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

#define LED_PIN 2

#define STM32_UART_PORT     UART_NUM_2
#define STM32_UART_RX_PIN   16
#define STM32_UART_TX_PIN   17
#define STM32_UART_BAUDRATE 115200
#define STM32_UART_RX_BUF   1024
#define STM32_UART_TX_BUF   1024
#define STM32_UART_EVT_SIZE 20

#define SOF_0            0xAAU
#define SOF_1            0x55U
#define PREFIX_SIZE      3
#define HEADER_SIZE      5
#define CRC_SIZE         2
#define PROTO_LEN_MIN    5
#define PROTO_LEN_MAX    101
#define FRAME_MAX        (PREFIX_SIZE + PROTO_LEN_MAX + CRC_SIZE)

#define CMD_UPSTREAM_PUSH  0x09

#define DTYPE_FLOAT  0x01
#define DTYPE_INT32  0x02
#define DTYPE_DOUBLE 0x03
#define DTYPE_INT    0x04
#define DTYPE_CHAR   0x05

static QueueHandle_t g_uartEventQueue = NULL;

typedef enum {
    RX_WAIT_SOF0 = 0,
    RX_WAIT_SOF1,
    RX_WAIT_LEN,
    RX_PAYLOAD,
} eRxState;

static eRxState g_rxState = RX_WAIT_SOF0;
static uint8_t g_rxFrame[FRAME_MAX];
static uint16_t g_rxPos = 0;
static uint16_t g_rxExpected = 0;

static uint16_t crc16(const uint8_t *buf, uint16_t len){
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)buf[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021)
                                 : (uint16_t)(crc << 1);
    }
    return crc;
}

static bool validateCRC(const uint8_t *raw, uint16_t total){
    uint16_t calc = crc16(&raw[3], (uint16_t)(total - PREFIX_SIZE - CRC_SIZE));
    uint16_t recv = ((uint16_t)raw[total - 2] << 8) | raw[total - 1];
    return (calc == recv);
}

static uint8_t dataTypeSize(uint8_t dt){
    return (dt == DTYPE_DOUBLE) ? 8U : 4U;
}

bool ledState = false;

static void rxByte(uint8_t b)
{
    switch (g_rxState) {

    case RX_WAIT_SOF0:
        if (b == SOF_0) {
            g_rxFrame[0] = b;
            g_rxPos = 1;
            g_rxState = RX_WAIT_SOF1;
            Serial.println("[SM] GOT SOF0");
        }
        break;

    case RX_WAIT_SOF1:
        if (b == SOF_1) {
            g_rxFrame[1] = b;
            g_rxPos = 2;
            g_rxState = RX_WAIT_LEN;
            Serial.println("[SM] GOT SOF1");
        } 
        else {
            Serial.println("[SM] GOT SOF1");
            g_rxState = RX_WAIT_SOF0;
            if (b == SOF_0) { g_rxFrame[0] = b; g_rxPos = 1; g_rxState = RX_WAIT_SOF1; }
        }
        break;

    case RX_WAIT_LEN: {
        uint8_t len = b;
        Serial.printf("[SM] LEN=0x%02X (%u)\n", len, len);
        
        if (len < PROTO_LEN_MIN || len > PROTO_LEN_MAX) {
          Serial.printf("[SM] BAD LEN, reset\n");
          
          g_rxState = RX_WAIT_SOF0; 
          break; 
          }
        g_rxFrame[2] = b;
        g_rxPos = 3;
        g_rxExpected = (uint16_t)(PREFIX_SIZE + len + CRC_SIZE);
        g_rxState = RX_PAYLOAD;
        Serial.printf("[SM] expecting %u bytes total\n", g_rxExpected);
        break;
    }

    case RX_PAYLOAD:
        if (g_rxPos < FRAME_MAX)
            g_rxFrame[g_rxPos++] = b;
        if (g_rxPos >= g_rxExpected) {
            Serial.printf("[SM] frame complete, CRC %s\n", validateCRC(g_rxFrame, g_rxPos) ? "OK" : "FAIL");

            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
                
            g_rxState = RX_WAIT_SOF0;
            g_rxPos = 0;
        }
        break;
    }
}

static void uartTask(void *pvParams){
    uart_event_t event;
    uint8_t rxBuf[FRAME_MAX];

    while(1){
        if (xQueueReceive(g_uartEventQueue, &event, portMAX_DELAY) != pdTRUE)
            continue;

        switch (event.type) {

        case UART_DATA:
            while (true) {
                int len = uart_read_bytes(STM32_UART_PORT, rxBuf, sizeof(rxBuf), pdMS_TO_TICKS(5));
                if (len <= 0) break;
                
                Serial.printf("RAW (%d): ", len);
                for (int i = 0; i < len; i++)
                Serial.printf("%02X ", rxBuf[i]);
                Serial.println();
                
                for (int i = 0; i < len; i++)
                    rxByte(rxBuf[i]);
            }
            break;

        case UART_FIFO_OVF:
        case UART_BUFFER_FULL:
            uart_flush_input(STM32_UART_PORT);
            xQueueReset(g_uartEventQueue);
            g_rxState = RX_WAIT_SOF0;
            g_rxPos = 0;
            break;

        case UART_FRAME_ERR:
        case UART_PARITY_ERR:
            uart_flush_input(STM32_UART_PORT);
            g_rxState = RX_WAIT_SOF0;
            g_rxPos = 0;
            break;

        default:
            break;
        }
    }
}
void setup(){
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);

    uart_config_t cfg = {
        .baud_rate = STM32_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(STM32_UART_PORT, &cfg);
    uart_set_pin(STM32_UART_PORT, STM32_UART_TX_PIN, STM32_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    esp_err_t ret = uart_driver_install(STM32_UART_PORT, STM32_UART_RX_BUF, STM32_UART_TX_BUF, STM32_UART_EVT_SIZE, &g_uartEventQueue, 0);
    Serial.printf("UART driver install: %s\n", esp_err_to_name(ret));
    
    xTaskCreatePinnedToCore(uartTask, "uartTask", 4096, NULL, 5, NULL, 0);
}

void loop()
{
    delay(10);
}
