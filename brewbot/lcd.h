#ifndef __LCD_H
#define __LCD_H

#include "types.h"

#define LCD_COMMAND_PAGE_ZERO    0x28
#define LCD_COMMAND_PAGE_ONE     0x29
#define LCD_COMMAND_PAGE_TWO     0x2A
#define LCD_COMMAND_PAGE_THREE   0x2B
 

const uint8_t * const Fontx5x7_table[256];

void lcd_open();
void lcd_set_x(uint8_t xx);
void lcd_set_y_page(uint8_t yy);
void lcd_command(int8_t command);
void lcd_write(int8_t data);
void lcd_set_address(uint8_t yy, uint8_t xx);
void lcd_string(uint8_t yy, uint8_t xx, const char *str);
void lcd_display_char(char c);
void lcd_display_number(int number);
void lcd_display_number_w_decimal(int number, int decimal_place);

void lcd_clear(void);
void lcd_text(uint8_t col, uint8_t row, const char *text);
void lcd_clear_chars(uint8_t col, uint8_t row, uint8_t ww);
void lcd_set_pixels (uint8_t col, uint8_t rol, uint8_t ww);

void lcd_printf(uint8_t col, uint8_t row, const char *fmt, ...);

#endif /* __LCD_H */
 
 
