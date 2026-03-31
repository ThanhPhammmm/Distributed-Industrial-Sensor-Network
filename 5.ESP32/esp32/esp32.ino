#include <Arduino.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_PIN             2
#define STM32_UART_PORT     UART_NUM_2
#define STM32_UART_RX_PIN   16
#define STM32_UART_TX_PIN   17
#define STM32_UART_BAUDRATE 115200
#define STM32_UART_RX_BUF   2048
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
#define CMD_UPSTREAM_ALARM 0x0A

#define DTYPE_FLOAT  0x01
#define DTYPE_INT32  0x02
#define DTYPE_DOUBLE 0x03
#define DTYPE_INT    0x04
#define DTYPE_CHAR   0x05

#define ALARM_NONE     0
#define ALARM_WARN     1
#define ALARM_CRITICAL 2

#define JSON_BUF_SIZE  768
#define MQTT_BUF_SIZE  (JSON_BUF_SIZE + 128)
#define TOPIC_PREFIX   "gateway"

#define WIFI_SSID      "Thu Trang 2007"
#define WIFI_PASSWORD  "trangbong"
#define MQTT_HOST      "192.168.0.103"
#define MQTT_PORT      1883
#define MQTT_CLIENT_ID "esp32-gateway"

typedef struct {
    uint8_t  data[FRAME_MAX];
    uint16_t len;
} sFrame_t;

#define FRAME_QUEUE_DEPTH  256

static QueueHandle_t g_frameQueue    = NULL;
static QueueHandle_t g_uartEventQueue = NULL;

static WiFiClient   g_wifi;
static PubSubClient g_mqtt(g_wifi);
static bool         g_ledState = false;

static uint16_t crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)buf[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021)
                                 : (uint16_t)(crc << 1);
    }
    return crc;
}

static bool validateCRC(const uint8_t *raw, uint16_t total)
{
    uint16_t calc = crc16(&raw[PREFIX_SIZE],
                          (uint16_t)(total - PREFIX_SIZE - CRC_SIZE));
    uint16_t recv = ((uint16_t)raw[total - 2] << 8) | raw[total - 1];
    return (calc == recv);
}

static uint8_t dataTypeSize(uint8_t dt)
{
    switch (dt) {
    case DTYPE_FLOAT:  return 4U;
    case DTYPE_INT32:  return 4U;
    case DTYPE_DOUBLE: return 8U;
    case DTYPE_INT:    return 4U;
    case DTYPE_CHAR:   return 1U;
    default:           return 0U;
    }
}

static const char *dataTypeName(uint8_t dt)
{
    switch (dt) {
    case DTYPE_FLOAT:  return "float";
    case DTYPE_INT32:  return "int32";
    case DTYPE_DOUBLE: return "double";
    case DTYPE_INT:    return "int";
    case DTYPE_CHAR:   return "char";
    default:           return "unknown";
    }
}

static const char *sensorTypeName(uint8_t st)
{
    switch (st) {
    case 0x01: return "temperature";
    case 0x02: return "humidity";
    case 0x03: return "pressure";
    case 0x04: return "adc_raw";
    case 0x05: return "digital_in";
    default:   return "unknown";
    }
}

static const char *alarmLevelName(uint8_t level)
{
    switch (level) {
    case ALARM_WARN:     return "WARN";
    case ALARM_CRITICAL: return "CRITICAL";
    default:             return "NONE";
    }
}

static void rawToValueStr(char *buf, size_t sz, uint8_t dt, const uint8_t *data)
{
    switch (dt) {
    case DTYPE_FLOAT:  { float   f; memcpy(&f, data, 4); snprintf(buf, sz, "%.2f",  f);       break; }
    case DTYPE_INT32:  { int32_t i; memcpy(&i, data, 4); snprintf(buf, sz, "%ld",   (long)i); break; }
    case DTYPE_DOUBLE: { double  d; memcpy(&d, data, 8); snprintf(buf, sz, "%.4f",  d);       break; }
    case DTYPE_INT:    { int    i2; memcpy(&i2,data, 4); snprintf(buf, sz, "%d",    i2);      break; }
    case DTYPE_CHAR:   { char   c;  memcpy(&c, data, 1); snprintf(buf, sz, "%d",    c);       break; }
    default:           snprintf(buf, sz, "null"); break;
    }
}

static void processDataFrame(const sFrame_t *frame)
{
    const uint8_t *raw   = frame->data;
    uint16_t       total = frame->len;

    uint8_t        payloadLen = (uint8_t)(total - PREFIX_SIZE - HEADER_SIZE - CRC_SIZE);
    const uint8_t *p          = &raw[PREFIX_SIZE + HEADER_SIZE];
    uint8_t        pos        = 0;

    while (pos + 2 <= payloadLen) {
        uint8_t slaveAddr   = p[pos++];
        uint8_t sensorCount = p[pos++];

        if (sensorCount == 0 || sensorCount > 8) {
            Serial.printf("[DATA] bad sensorCount=%u, abort\n", sensorCount);
            return;
        }

        char     json[JSON_BUF_SIZE];
        uint16_t jLen = 0;

        jLen += snprintf(json + jLen, sizeof(json) - jLen,
                         "{\"slaveAddr\":\"%02X\","
                         "\"timestamp\":%lu,"
                         "\"sensors\":[",
                         slaveAddr, (unsigned long)millis());

        for (uint8_t s = 0; s < sensorCount; s++) {
            if (pos + 3 > payloadLen) {
                Serial.printf("[DATA] truncated at sensor %u\n", s);
                return;
            }
            uint8_t sensorId   = p[pos++];
            uint8_t sensorType = p[pos++];
            uint8_t dataType   = p[pos++];
            uint8_t sz         = dataTypeSize(dataType);

            if (sz == 0 || pos + sz > payloadLen) {
                Serial.printf("[DATA] bad dataType=0x%02X sensor %u\n", dataType, s);
                return;
            }

            char valueStr[32];
            rawToValueStr(valueStr, sizeof(valueStr), dataType, &p[pos]);
            pos += sz;

            jLen += snprintf(json + jLen, sizeof(json) - jLen,
                             "%s{"
                             "\"sensorId\":%u,"
                             "\"sensorType\":\"%s\","
                             "\"dataType\":\"%s\","
                             "\"value\":%s"
                             "}",
                             s > 0 ? "," : "",
                             sensorId,
                             sensorTypeName(sensorType),
                             dataTypeName(dataType),
                             valueStr);
        }

        jLen += snprintf(json + jLen, sizeof(json) - jLen, "]}");

        char topic[48];
        snprintf(topic, sizeof(topic), "%s/%02X/data", TOPIC_PREFIX, slaveAddr);

        Serial.println("==== DATA FRAME ====");
        Serial.printf("Topic: %s\n", topic);
        Serial.printf("Value: %s\n", json);
        Serial.println("====================");

        if (g_mqtt.publish(topic, (uint8_t *)json, jLen, true))
            Serial.printf("[MQTT] pub %s ok\n", topic);
        else
            Serial.printf("[MQTT] pub %s FAIL\n", topic);
    }
}

static void processAlarmFrame(const sFrame_t *frame)
{
    const uint8_t *raw   = frame->data;
    uint16_t       total = frame->len;

    uint8_t        payloadLen = (uint8_t)(total - PREFIX_SIZE - HEADER_SIZE - CRC_SIZE);
    const uint8_t *p          = &raw[PREFIX_SIZE + HEADER_SIZE];

    if (payloadLen < 6) {
        Serial.printf("[ALARM] payload too short (%u)\n", payloadLen);
        return;
    }

    uint8_t slaveAddr  = p[0];
    uint8_t sensorId   = p[1];
    uint8_t sensorType = p[2];
    uint8_t alarmLevel = p[3];
    uint8_t dataType   = p[4];
    uint8_t sz         = dataTypeSize(dataType);

    if (sz == 0 || (uint8_t)(5 + sz) > payloadLen) {
        Serial.printf("[ALARM] bad dataType=0x%02X\n", dataType);
        return;
    }

    char valueStr[32];
    rawToValueStr(valueStr, sizeof(valueStr), dataType, &p[5]);

    char     json[JSON_BUF_SIZE];
    uint16_t jLen = snprintf(json, sizeof(json),
                             "{\"slaveAddr\":\"%02X\","
                             "\"timestamp\":%lu,"
                             "\"sensorId\":%u,"
                             "\"sensorType\":\"%s\","
                             "\"dataType\":\"%s\","
                             "\"value\":%s,"
                             "\"alarmLevel\":\"%s\"}",
                             slaveAddr, (unsigned long)millis(),
                             sensorId,
                             sensorTypeName(sensorType),
                             dataTypeName(dataType),
                             valueStr,
                             alarmLevelName(alarmLevel));

    char topic[48];
    snprintf(topic, sizeof(topic), "%s/%02X/alarm", TOPIC_PREFIX, slaveAddr);

    Serial.println("==== ALARM FRAME ====");
    Serial.printf("Topic:   %s\n", topic);
    Serial.printf("Payload: %s\n", json);
    Serial.println("=====================");

    if (g_mqtt.publish(topic, (uint8_t *)json, jLen, false))
        Serial.printf("[MQTT] pub %s [%s] ok\n", topic, alarmLevelName(alarmLevel));
    else
        Serial.printf("[MQTT] pub %s FAIL\n", topic);
}

static void frameTask(void *pvParams)
{
    sFrame_t frame;

    while (1) {
        if (xQueueReceive(g_frameQueue, &frame, portMAX_DELAY) != pdTRUE)
            continue;

        g_ledState = !g_ledState;
        digitalWrite(LED_PIN, g_ledState);

        uint8_t cmd = frame.data[5];
        switch (cmd) {
        case CMD_UPSTREAM_PUSH:  processDataFrame(&frame);  break;
        case CMD_UPSTREAM_ALARM: processAlarmFrame(&frame); break;
        default:
            Serial.printf("[FRAME] unknown cmd=0x%02X\n", cmd);
            break;
        }
    }
}

typedef enum {
    RX_WAIT_SOF0 = 0,
    RX_WAIT_SOF1,
    RX_WAIT_LEN,
    RX_PAYLOAD,
} eRxState;

static eRxState g_rxState    = RX_WAIT_SOF0;
static uint8_t  g_rxFrame[FRAME_MAX];
static uint16_t g_rxPos      = 0;
static uint16_t g_rxExpected = 0;

static void smReset(void)
{
    g_rxState = RX_WAIT_SOF0;
    g_rxPos   = 0;
}

static void rxByte(uint8_t b)
{
    switch (g_rxState) {

    case RX_WAIT_SOF0:
        if (b == SOF_0) {
            g_rxFrame[0] = b;
            g_rxPos      = 1;
            g_rxState    = RX_WAIT_SOF1;
        }
        break;

    case RX_WAIT_SOF1:
        if (b == SOF_1) {
            g_rxFrame[1] = b;
            g_rxPos      = 2;
            g_rxState    = RX_WAIT_LEN;
        } else {
            smReset();
            if (b == SOF_0) { g_rxFrame[0] = b; g_rxPos = 1; g_rxState = RX_WAIT_SOF1; }
        }
        break;

    case RX_WAIT_LEN: {
        uint8_t len = b;
        if (len < PROTO_LEN_MIN || len > PROTO_LEN_MAX) {
            Serial.printf("[SM] BAD LEN=%u, reset\n", len);
            smReset();
            break;
        }
        g_rxFrame[2]  = b;
        g_rxPos       = 3;
        g_rxExpected  = (uint16_t)(PREFIX_SIZE + len + CRC_SIZE);
        g_rxState     = RX_PAYLOAD;
        break;
    }

    case RX_PAYLOAD:
        if (g_rxPos < FRAME_MAX)
            g_rxFrame[g_rxPos++] = b;

        if (g_rxPos >= g_rxExpected) {

            if (!validateCRC(g_rxFrame, g_rxPos)) {
                Serial.println("[SM] CRC FAIL, discard");
                smReset();
                break;
            }

            sFrame_t frame;
            memcpy(frame.data, g_rxFrame, g_rxPos);
            frame.len = g_rxPos;

            BaseType_t sent = xQueueSend(g_frameQueue, &frame, 0);
            if (sent != pdTRUE)
                Serial.println("[SM] frame queue FULL, dropped!");
            else
                Serial.printf("[SM] frame queued (len=%u, cmd=0x%02X)\n",
                              g_rxPos, g_rxFrame[5]);

            smReset();
        }
        break;
    }
}

static void uartTask(void *pvParams)
{
    uart_event_t event;
    uint8_t      rxBuf[FRAME_MAX];

    while (1) {
        if (xQueueReceive(g_uartEventQueue, &event, portMAX_DELAY) != pdTRUE)
            continue;

        switch (event.type) {

        case UART_DATA:
            while (true) {
                int len = uart_read_bytes(STM32_UART_PORT, rxBuf, sizeof(rxBuf), pdMS_TO_TICKS(5));
                if (len <= 0) break;

                Serial.printf("RAW (%d): ", len);
                for (int i = 0; i < len; i++) Serial.printf("%02X ", rxBuf[i]);
                Serial.println();

                for (int i = 0; i < len; i++)
                    rxByte(rxBuf[i]);
            }
            break;

        case UART_FIFO_OVF:
        case UART_BUFFER_FULL:
            Serial.println("[UART] buffer overflow, flush");
            uart_flush_input(STM32_UART_PORT);
            xQueueReset(g_uartEventQueue);
            smReset();
            break;

        case UART_FRAME_ERR:
        case UART_PARITY_ERR:
            Serial.println("[UART] frame/parity err");
            uart_flush_input(STM32_UART_PORT);
            smReset();
            break;

        default:
            break;
        }
    }
}

static void wifiEnsureConnected(void)
{
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WiFi] connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("[WiFi] OK  IP=%s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("[WiFi] FAIL – will retry");
}

static void mqttEnsureConnected(void)
{
    if (g_mqtt.connected()) return;
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.printf("[MQTT] connecting %s:%d ...\n", MQTT_HOST, MQTT_PORT);

#if defined(MQTT_USER) && defined(MQTT_PASS)
    bool ok = g_mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
#else
    bool ok = g_mqtt.connect(MQTT_CLIENT_ID);
#endif

    if (ok) Serial.println("[MQTT] connected");
    else    Serial.printf("[MQTT] fail rc=%d\n", g_mqtt.state());
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    g_frameQueue = xQueueCreate(FRAME_QUEUE_DEPTH, sizeof(sFrame_t));
    configASSERT(g_frameQueue != NULL);

    wifiEnsureConnected();
    g_mqtt.setServer(MQTT_HOST, MQTT_PORT);
    g_mqtt.setBufferSize(MQTT_BUF_SIZE);
    mqttEnsureConnected();

    uart_config_t cfg = {
        .baud_rate           = STM32_UART_BAUDRATE,
        .data_bits           = UART_DATA_8_BITS,
        .parity              = UART_PARITY_DISABLE,
        .stop_bits           = UART_STOP_BITS_1,
        .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk          = UART_SCLK_APB,
    };
    uart_param_config(STM32_UART_PORT, &cfg);
    uart_set_pin(STM32_UART_PORT,
                 STM32_UART_TX_PIN, STM32_UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    esp_err_t ret = uart_driver_install(STM32_UART_PORT,
                                        STM32_UART_RX_BUF, STM32_UART_TX_BUF,
                                        STM32_UART_EVT_SIZE, &g_uartEventQueue, 0);
    Serial.printf("[UART] driver: %s\n", esp_err_to_name(ret));

    xTaskCreatePinnedToCore(uartTask,  "uartTask",  4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(frameTask, "frameTask", 6144, NULL, 4, NULL, 1);

    Serial.println("[SYS] ready");
}

void loop()
{
    wifiEnsureConnected();
    mqttEnsureConnected();
    g_mqtt.loop();
    delay(10);
}
