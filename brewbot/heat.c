
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
static xTaskHandle    mashTaskHandle;

void setHeatTargetTemperature(float target)
{
    mash_target = target;
    display_status();
}

void setHeatDutyCycle(int duty_cycle)
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
    lcd_text(8, 0, "Heating");
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

	// kick off reading the temp
	vTaskEnterCritical();
	DS1820Init();
        DS1820Skip();
        DS1820Convert();
	vTaskExitCritical();

	// 1 second delay, run heat according to the duty cycle
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

	vTaskEnterCritical();
	DS1820ReadTemp();
	vTaskExitCritical();
    }   
}

void startHeatTask()
{
    xTaskCreate( mashTask,
		 ( signed char * ) "HeatTask",
		 configMINIMAL_STACK_SIZE + 600, NULL,
		 4, ( xTaskHandle  * ) &mashTaskHandle );
}

void stopHeatTask()
{
    vTaskDelete( mashTaskHandle );
}
