#ifndef SLAVE_CONFIG_STORE_H
#define SLAVE_CONFIG_STORE_H

#include <stdint.h>

#define SLAVE_CFG_FLASH_ADDR    ((uint32_t)0x0800FC00)

void    SlaveConfig_Load(void);
uint8_t SlaveConfig_GetVersion(void);

#endif
