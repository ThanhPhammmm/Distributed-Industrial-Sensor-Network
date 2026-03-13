#ifndef INC_LCD_DRIVER_H_
#define INC_LCD_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t col, uint8_t row);
void LCD_Print(const char *str);
void LCD_SetBacklight(bool on);

void LCD_Write2Lines(const char *line1, const char *line2);

#endif /* INC_LCD_DRIVER_H_ */
