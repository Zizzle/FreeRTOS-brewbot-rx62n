#include "iodefine.h"
#include "lcd.h"
#include "spi.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

// lcd is 96 x 64 pixels
// 19 x 8 chars
#define LCD_W 96
#define LCD_H 64

#define CHAR_W 5

// the A0 line is used to determine which mode the byte sent is to be interpreted in
#define LCD_SET_COMMAND_MODE     PORT5.DR.BIT.B1 = 0
#define LCD_SET_DATA_MODE        PORT5.DR.BIT.B1 = 1

void lcd_open()
{
    int i = 0 ;
    
    PORTC.DDR.BIT.B3 = 1 ;   // LCD GPIO for Reset LCD
    PORT5.DDR.BIT.B1 = 1 ;   // LCD RS A0

    PORTC.DR.BIT.B3 = 1 ;    // Reset LCD
    for (i=0;i<1000000;i++)asm("");
    PORTC.DR.BIT.B3 = 0 ;    // clear LCD reset line
    for (i=0;i<1000000;i++)asm("");

    // set up the bias and boost
    lcd_command(LCD_COMMAND_PAGE_ONE);
    lcd_command(0x16); // bias
    lcd_command(LCD_COMMAND_PAGE_THREE);
    lcd_command(0x9B); // boost

    // set VO range
    lcd_command(LCD_COMMAND_PAGE_ZERO);
    lcd_command(0x4);
    lcd_command(LCD_COMMAND_PAGE_ONE);
    lcd_command(0x80 | 96);

    // clear the ram
    lcd_set_x(0);
    lcd_set_y_page(0);
    for (i =0 ; i < 808; i++)
	lcd_write(0x00);

    lcd_command(LCD_COMMAND_PAGE_ZERO);
    lcd_command(0x8 | 0x4);

}

void lcd_set_x(uint8_t xx)
{
    lcd_command(LCD_COMMAND_PAGE_ZERO);
    lcd_command(0x80 | xx);
}

void lcd_set_y_page(uint8_t yy)
{
    lcd_command(LCD_COMMAND_PAGE_ZERO);
    lcd_command(0x40 | yy);	
}

void lcd_set_address(uint8_t yy, uint8_t xx)
{
    lcd_set_y_page(yy);
    lcd_set_x(xx);
}

void lcd_out_byte(uint8_t byte, uint8_t isCommand)
{
    if (!spi_select(SPI_DEVICE_LCD))
	return;
    if (isCommand) LCD_SET_COMMAND_MODE;
    else LCD_SET_DATA_MODE;
    spi_write(&byte, 1);
    spi_release();
}

void lcd_command(int8_t command)
{
    lcd_out_byte(command, 1);
}

void lcd_write(int8_t data)
{
    lcd_out_byte(data, 0);
}

void lcd_display_char(char c)
{
    int ii;
    const uint8_t *data = Fontx5x7_table[(int)c];
    for (ii = 0; ii < 5; ii++)
    {
	lcd_write(data[ii + 2]);
    }
}

void lcd_string(uint8_t yy, uint8_t xx, const char *str)
{
  lcd_set_address(yy, xx);
  while (*str)
  {
	lcd_display_char(*str++);
  }
}

void lcd_display_number(int number)
{
    lcd_display_number_w_decimal(number, -1);
}

void lcd_display_number_w_decimal(int number, int decimal_place)
{
    char result[12];
    int i = 0;
    do
    {
	result[i++] = '0' + (number % 10);
    }
    while ((number /= 10) > 0 && i < sizeof(result) );
    i--;

    while (i >= 0)
    {
        lcd_display_char(result[i]);
	if (decimal_place > 0 && i == decimal_place) lcd_display_char('.');
	i--;
    }
}

void lcd_clear(void)
{
    int i = 0 ;

    // clear the ram
    lcd_set_x(0);
    lcd_set_y_page(0);
    for (i =0 ; i < 808; i++)
	lcd_write(0x00);
}

void lcd_text(uint8_t col, uint8_t row, const char *text)
{
    lcd_string(row, CHAR_W * col, text);
}

void lcd_clear_chars(uint8_t col, uint8_t row, uint8_t ww)
{
    int i = 0 ;
    lcd_set_address(col * CHAR_W, row);
    for (i =0 ; i < ww * CHAR_W; i++)
	lcd_write(0x00);
}

void lcd_set_pixels(uint8_t col, uint8_t row, uint8_t ww)
{
    int i = 0 ;
    lcd_set_address(col * CHAR_W, row);
    for (i =0 ; i < ww * CHAR_W; i++)
	lcd_write(0xff);
}

void lcd_printf(uint8_t col, uint8_t row, uint8_t ww, const char *fmt, ...)
{
    char message[21];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(message, sizeof(message) - 1, fmt, ap);
    va_end(ap);
    lcd_text(col, row, message);

    while (len++ < ww)
    {
	lcd_display_char(' ');
    }
}
