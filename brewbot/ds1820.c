
#include "FreeRTOS.h"
#include "task.h"
#include "iodefine.h"
#include "yrdkrx62ndef.h"
#include "lcd.h"
#include <stdio.h>

#define DQ1        PORT4.DR.BIT.B0 = 1
#define DQ0        PORT4.DR.BIT.B0 = 0
#define DQ_READ    PORT4.PORT.BIT.B0
#define DQ_DIR_IN  PORT4.DDR.BIT.B0 = 0
#define DQ_DIR_OUT PORT4.DDR.BIT.B0 = 1

static int     ds1820_temperature=0;
static int     ds1820_fahrenheit =0;
static unsigned char ds1820_error = 0;
static portTickType ds1820_last_read_time;

float temper;

float ds1820_get_temperature()
{
//    char foo[200];
//    temper = 1.0;

//    snprintf(foo, sizeof(foo), "Temp %.2f");
    return ((float)ds1820_temperature) / 100;
}

#define delayUs(x) delay((x) * 18)

void delay(int i)
{
  for (; i; --i)
    asm("");
}

void Write_18B20(unsigned char n)
{
    unsigned char i;

    for(i = 0; i < 8; i++)
    {
	DQ0;
	delayUs(10);//13us
	if((n & 0x01) == 0x01) DQ1;
	else DQ0;
	n = n >> 1;
	delayUs(50);//50us
	DQ1;
    }
}
//------------------------------------
unsigned char Read_18B20(void)
{
    unsigned char i;
    unsigned char temp = 0;

    for(i=0;i<8;i++)
    {
	temp = temp >> 1;
	DQ0;
	delayUs(2);//1us
//	DQ1;
//        delayUs(5); //5us
	DQ_DIR_IN;
	delayUs(10);
	if(DQ_READ == 0)
	    temp = temp & 0x7F;
	else
	    temp = temp | 0x80;
	delayUs(40);//40us

	DQ_DIR_OUT;
	DQ1;
    }
    return temp;
}

//----------------------------------
void DS1820Skip(void)
{
    Write_18B20(0xcc);
}
//----------------------------------
void DS1820Convert (void)
{
    Write_18B20(0x44);
}
//----------------------------------
void DS1820ReadDo (void)
{
    Write_18B20(0xbe);
}
//----------------------------------
void DS1820Init (void)
{
    DQ_DIR_OUT;
    DQ0;
    delayUs(500);
    DQ1;
    DQ_DIR_IN;

    delayUs(90);

    if( DQ_READ )        //0001 1111b=1f
    {
	ds1820_error = 1;    //1
	DQ_DIR_OUT;
    }
    else
    {
	ds1820_error = 0;//
        DQ_DIR_OUT;
	DQ1;
    }
    delayUs(500);
}
//-----------------------------------
void DS1820ReadTemp(void)
{
    unsigned char temp_low,temp_high; 

    DS1820Init();
    DS1820Skip();
    DS1820ReadDo();

    temp_low=Read_18B20(); 
    temp_high=Read_18B20();

	Read_18B20(); // TH
	Read_18B20(); // TL
	Read_18B20(); // reserved
	Read_18B20(); // reserved
    unsigned char remain = Read_18B20(); // remaining count

    ds1820_temperature = temp_high & 0x0f;
    ds1820_temperature <<= 8;
    ds1820_temperature |= temp_low;

    ds1820_temperature >>= 1;
    ds1820_temperature = (ds1820_temperature * 100) -  25  + (100 * 16 - remain * 100) / (16);

    ds1820_fahrenheit = 3200 + (ds1820_temperature * 9) / 5;

    ds1820_last_read_time = xTaskGetTickCount();

}

static void ds1820Task( void *pvParameters )
{
    int ii =0;
    for(;;)
    {
	PORT4.DR.BIT.B1 = 1;
	PORT4.DR.BIT.B2 = 1;
	PORT4.DR.BIT.B3 = 1;
	PORT4.DDR.BIT.B1 = 1;
	PORT4.DDR.BIT.B1 = 1;
	PORT4.DDR.BIT.B1 = 1;

	vTaskEnterCritical();
	DS1820Init();
        DS1820Skip();
        DS1820Convert();
	vTaskExitCritical();

	vTaskDelay(800); // wait for the conversion to happen
	
	vTaskEnterCritical();
	DS1820ReadTemp();
	vTaskExitCritical();

	{
	    char foo[20];
	    snprintf(foo, sizeof(foo), "Temp %d . %d  ", ds1820_temperature, ii++);
	    lcd_string(0, 6, foo);
	}
    }    
}

void startDS1820Task()
{
    xTaskCreate( ds1820Task,
		 ( signed char * ) "DS1820",
		 configMINIMAL_STACK_SIZE + 100, NULL,
		 4, ( xTaskHandle  * ) NULL );
}
