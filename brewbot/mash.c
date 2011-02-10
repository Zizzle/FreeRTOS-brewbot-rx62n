
#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "crane.h"
#include "semphr.h"
#include "ds1820.h"
#include "lcd.h"
#include <stdio.h>
static void display_status();

static volatile float mash_target = 0.0f;
static volatile int   mash_duty_cycle = 50;

void setMashTargetTemperature(float target)
{
    mash_target = target;
    display_status();
}

void setMashDutyCycle(int duty_cycle)
{
    mash_duty_cycle = duty_cycle;
    display_status();
}

static void allOff()
{
    outputOff(SSR);
    SSR_DDR = 1;
}


static void display_status()
{
//    snprintf(message, sizeof(message), "mash: %d duty %d",
//	     (int) (mash_target * 100), mash_duty_cycle);
//    lcd_string(6,0, message);
    lcd_string(6,0, "mash: ");
    lcd_display_number((int) (mash_target * 100));
    lcd_display_char(' ');
    lcd_display_number(mash_duty_cycle);
    lcd_display_char(' ');    
}

static void mashTask( void *pvParameters )
{
    int ii = 0;

    allOff();

    for (;;)
    {
	display_status();

	for (ii = 0; ii < 100; ii++)
	{
	    if (ds1820_get_temperature() < mash_target && ii <= mash_duty_cycle)
	    {
		outputOn(SSR);
	    }
	    else
	    {
		allOff();
	    }
	    vTaskDelay(10); // wait for the conversion to happen
	}
    }   
}

void startMashTask()
{
    xTaskCreate( mashTask,
		 ( signed char * ) "MashTask",
		 configMINIMAL_STACK_SIZE, NULL,
		 4, ( xTaskHandle  * ) NULL );
}
