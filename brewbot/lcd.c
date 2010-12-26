#include "iodefine.h"
#include "lcd.h"

// the A0 line is used to determine which mode the byte sent is to be interpreted in
#define LCD_SET_COMMAND_MODE     PORT5.DR.BIT.B1 = 0
#define LCD_SET_DATA_MODE        PORT5.DR.BIT.B1 = 1
// we need to assert the chip select line before sending any data to the LCD
#define LCD_CHIP_SELECT_SET      PORTC.DR.BIT.B2 = 0
#define LCD_CHIP_SELECT_CLR      PORTC.DR.BIT.B2 = 1

void lcd_open()
{
    int i = 0 ;
    
    PORTC.DDR.BIT.B3 = 1 ;   // LCD GPIO for Reset LCD
    PORT5.DDR.BIT.B1 = 1 ;   // LCD RS A0

    PORTC.DR.BIT.B3 = 1 ;    // Reset LCD
    for (i=0;i<1000000;i++)asm("");
    PORTC.DR.BIT.B3 = 0 ;    // clear LCD reset line
    for (i=0;i<1000000;i++)asm("");

    MSTP(RSPI0) = 0 ;             // enable module
    IOPORT.PFGSPI.BIT.RSPIS  = 0; // Select proper bank of pins for SPI0 
    IOPORT.PFGSPI.BIT.RSPCKE = 1; // SCK (PC.5) is active
    IOPORT.PFGSPI.BIT.SSL3E  = 0; // SSL3 (PC.2) is inactive (toggled as GPIO instead)
    IOPORT.PFGSPI.BIT.MOSIE  = 1; // MOSI (PC.6) is active
    PORTC.DDR.BIT.B2 = 1;     // Set up chip select pin. Make it an output
    PORTC.DR.BIT.B2  = 1;     // Set level to inactive
    PORTC.DDR.BIT.B7 = 1;     // MISO as an input
    PORTC.DR.BIT.B7  = 1;     // Enable input buffer for peripheral
    PORTC.DDR.BIT.B6 = 1;     // MOSI as an output
    PORTC.DR.BIT.B6  = 1;     // Enable input buffer for peripheral
    PORTC.DDR.BIT.B5 = 1;     // SCK as an output
    PORTC.DR.BIT.B5  = 1;     // Set level to inactive
    
    /* Initialize SPI (per flowchart in hardware manual) */
    RSPI0.SPPCR.BYTE = 0x00;  // No loopback, CMOS output
    RSPI0.SPBR.BYTE  = 0x00;  // Full speed
    RSPI0.SPDCR.BYTE = 0x00;  // 16-bit data 1 frame 1 chip select
    RSPI0.SPCKD.BYTE = 0x00;  // 2 clock delay before next access to SPI device
    RSPI0.SSLND.BYTE = 0x00;  // 2 clock delay after de-asserting SSL
    RSPI0.SPND.BYTE  = 0x00;  // 2 clock delay before next access to SPI device
    RSPI0.SPCR2.BYTE = 0x00;  // No parity no idle interrupts
    RSPI0.SPCMD0.WORD = 0x0700; // MSB first 8-bit data, keep SSL low
    RSPI0.SPCR.BYTE  = 0x6B;  // Enable RSPI 3wire in master mode with RSPI Enable Transmit Only and Interupt 
    RSPI0.SSLP.BYTE  = 0x08;  // SSL3A Polarity
    RSPI0.SPSCR.BYTE = 0x00;  // One frame

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

void lcd_out_byte(int16_t sHighWord, uint8_t isCommand)
{
    LCD_CHIP_SELECT_SET;

    if (isCommand) LCD_SET_COMMAND_MODE;
    else LCD_SET_DATA_MODE;

    while (RSPI0.SPSR.BIT.IDLNF)  // ensure transmit register is empty
	;

    RSPI0.SPDR.WORD.L = 0 ;
    RSPI0.SPDR.WORD.H = sHighWord;

    while (RSPI0.SPSR.BIT.IDLNF) // wait for transfer to complete
	;

    (void)RSPI0.SPDR.WORD.L;
    (void)RSPI0.SPDR.WORD.H;

    LCD_CHIP_SELECT_CLR;
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


