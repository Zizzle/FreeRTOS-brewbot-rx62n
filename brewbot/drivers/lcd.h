#ifndef __LCD_H
#define __LCD_H

#include "types.h"

#define LCD_COMMAND_PAGE_ZERO    0x28
#define LCD_COMMAND_PAGE_ONE     0x29
#define LCD_COMMAND_PAGE_TWO     0x2A
#define LCD_COMMAND_PAGE_THREE   0x2B
 

const uint8_t * const Fontx5x7_table[256];

void lcd_open();

void lcd_clear(void);
void lcd_text(uint8_t col, uint8_t row, const char *text);
void lcd_clear_chars(uint8_t col, uint8_t row, uint8_t ww);
void lcd_set_pixels (uint8_t col, uint8_t rol, uint8_t ww);

void lcd_printf(uint8_t col, uint8_t row, uint8_t ww, const char *fmt, ...);

#endif /* __LCD_H */
 
 
