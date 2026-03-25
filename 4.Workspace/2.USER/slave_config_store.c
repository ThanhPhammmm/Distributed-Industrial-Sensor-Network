#include "slave_config_store.h"
#include "slave_config.h"
#include "slave_sensor.h"
#include "stm32f10x_flash.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

extern uint8_t k_cnt;
extern SensorEntry_t g_sensors[];

typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  hash;
    uint8_t  sensorCount;
    uint8_t  _pad;
    uint32_t crc32;
} FlashCfg_t;

static FlashCfg_t g_cfg;

uint8_t Slave_Sensors_GetTableHash(void) {
    uint8_t crc = 0xFF;
    const uint8_t *p = (const uint8_t *)g_sensors;
    size_t total_bytes = k_cnt * sizeof(SensorEntry_t);

    for (size_t i = 0; i < total_bytes; i++) {
        crc ^= p[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
        }
    }
    return crc ^ k_cnt;
}

static uint32_t _Crc32(const uint8_t *b, size_t n) {
    uint32_t c = 0xFFFFFFFFUL;
    for (size_t i = 0; i < n; i++) {
        c ^= b[i];
        for (int j = 0; j < 8; j++)
            c = (c & 1UL) ? ((c >> 1) ^ 0xEDB88320UL) : (c >> 1);
    }
    return ~c;
}

static void _Save(void) {
    uint32_t addr = SLAVE_CFG_FLASH_ADDR;
    uint32_t *src = (uint32_t *)&g_cfg;

    g_cfg.crc32 = _Crc32((const uint8_t *)&g_cfg, sizeof(g_cfg) - 4U);

    __disable_irq();
    taskENTER_CRITICAL();

    FLASH_Unlock();
    
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    if (FLASH_ErasePage(SLAVE_CFG_FLASH_ADDR) == FLASH_COMPLETE) {
        for (size_t i = 0; i < (sizeof(FlashCfg_t) / 4U); i++) {
            FLASH_ProgramWord(addr, src[i]);
            addr += 4U;
        }
    }

    FLASH_Lock();

    taskEXIT_CRITICAL();
    __enable_irq();
}

void SlaveConfig_Load(void) {
    uint8_t currentHash = Slave_Sensors_GetTableHash();
    uint8_t currentCount = k_cnt;

    memcpy(&g_cfg, (const void *)SLAVE_CFG_FLASH_ADDR, sizeof(g_cfg));

    uint8_t valid = (g_cfg.version != 0xFFU) &&
                    (_Crc32((const uint8_t *)&g_cfg, sizeof(g_cfg) - 4U) == g_cfg.crc32);

    if (!valid) {
        g_cfg.version     = 1U;
        g_cfg.hash        = currentHash;
        g_cfg.sensorCount = currentCount;
        _Save();
        return;
    }

    if (g_cfg.hash != currentHash || g_cfg.sensorCount != currentCount) {
        g_cfg.version = (g_cfg.version == 1U) ? 2U : 1U;
        g_cfg.hash    = currentHash;
        g_cfg.sensorCount = currentCount;
        _Save();
    }
}

uint8_t SlaveConfig_GetVersion(void)     { return g_cfg.version; }
uint8_t SlaveConfig_GetSensorCount(void) { return g_cfg.sensorCount; }
