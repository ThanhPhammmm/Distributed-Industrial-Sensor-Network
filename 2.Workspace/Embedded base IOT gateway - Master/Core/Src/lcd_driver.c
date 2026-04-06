#include "lcd_driver.h"
#include "Configuration.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

extern I2C_HandleTypeDef hi2c1;

#define I2C_ADDR   ((uint16_t)(LCD_I2C_ADDR << 1))

#define BIT_RS  0x01U
#define BIT_RW  0x02U
#define BIT_EN  0x04U
#define BIT_BL  0x08U
#define BIT_D4  0x10U
#define BIT_D5  0x20U
#define BIT_D6  0x40U
#define BIT_D7  0x80U

static uint8_t g_bl = BIT_BL;

static void _I2C(uint8_t b){
    HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR, &b, 1U, 10U);
}

static void _Pulse(uint8_t b){
    _I2C((uint8_t)(b | BIT_EN));
    vTaskDelay(pdMS_TO_TICKS(1));
    _I2C((uint8_t)(b & ~BIT_EN));
    vTaskDelay(pdMS_TO_TICKS(1));
}

static void _Nibble(uint8_t byte, bool rs){
    uint8_t b = (uint8_t)((byte & 0xF0U) | g_bl | (rs ? BIT_RS : 0U));
    _Pulse(b);
}

static void _Byte(uint8_t byte, bool rs){
    _Nibble((uint8_t)(byte & 0xF0U), rs);
    _Nibble((uint8_t)((byte << 4) & 0xF0U), rs);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void LCD_Init(void){
	vTaskDelay(pdMS_TO_TICKS(50));
    _Nibble(0x30U, false);
    vTaskDelay(pdMS_TO_TICKS(5));
    _Nibble(0x30U, false);
    vTaskDelay(pdMS_TO_TICKS(1));
    _Nibble(0x30U, false);
    vTaskDelay(pdMS_TO_TICKS(1));
    _Nibble(0x20U, false);
    vTaskDelay(pdMS_TO_TICKS(1));

    _Byte(0x28U, false);
    _Byte(0x0CU, false);
    _Byte(0x01U, false);
    vTaskDelay(pdMS_TO_TICKS(3));
    _Byte(0x06U, false);
}

void LCD_Clear(void){
    _Byte(0x01U, false);
    vTaskDelay(pdMS_TO_TICKS(3));
}

void LCD_SetCursor(uint8_t col, uint8_t row){
    static const uint8_t row_off[2] = { 0x00U, 0x40U };
    _Byte((uint8_t)(0x80U | (row_off[row & 1U] + (col & 0x0FU))), false);
}

void LCD_Print(const char *str){
    while (*str) {
        _Byte((uint8_t)*str++, true);
    }
}

void LCD_SetBacklight(bool on){
    g_bl = on ? BIT_BL : 0x00U;
    _I2C(g_bl);
}

void LCD_Write2Lines(const char *line1, const char *line2){
    char buf[LCD_COLS + 1U];
    size_t n;

    LCD_SetCursor(0, 0);
    n = strlen(line1);
    if (n > LCD_COLS) n = LCD_COLS;
    memcpy(buf, line1, n);
    memset(buf + n, ' ', LCD_COLS - n);
    buf[LCD_COLS] = '\0';
    LCD_Print(buf);

    LCD_SetCursor(0, 1);
    n = strlen(line2);
    if (n > LCD_COLS) n = LCD_COLS;
    memcpy(buf, line2, n);
    memset(buf + n, ' ', LCD_COLS - n);
    buf[LCD_COLS] = '\0';
    LCD_Print(buf);
}
