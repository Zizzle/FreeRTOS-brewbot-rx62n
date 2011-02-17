#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "crane.h"
#include "semphr.h"
#include "ds1820.h"
#include "lcd.h"
#include "level_probes.h"
#include <stdio.h>
static void display_status();

static volatile float   heat_target = 0.0f;
static volatile int     heat_duty_cycle = 50;
static xTaskHandle      heatTaskHandle;
static volatile uint8_t heat_target_reached;

static void allOff()
{
    outputOff(SSR);
    SSR_DDR = 1;
}

static void display_status()
{
    lcd_printf(0, 4, 19, "Target temp %.1f (%d)", heat_target, heat_target_reached);
    lcd_printf(0, 5, 18, "Temp %.2f (%d%%)",
	       (double)ds1820_get_temperature(), heat_duty_cycle);
}

static void heatTask( void *pvParameters )
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
	    if (ds1820_get_temperature() < heat_target &&
		ii <= heat_duty_cycle &&
		level_hit_heat()) // check the element is covered
	    {
		outputOn(SSR);
	    }
	    else
	    {
		if (!heat_target_reached)
		    heat_target_reached = (ds1820_get_temperature() >= heat_target);
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
    xTaskCreate( heatTask,
		 ( signed char * ) "HeatTask",
		 configMINIMAL_STACK_SIZE + 600, NULL,
		 4, ( xTaskHandle  * ) &heatTaskHandle );
}

void stopHeatTask()
{
    vTaskDelete( heatTaskHandle );
    heatTaskHandle = NULL;
    allOff();
}

uint8_t isHeatTaskRunning()
{
    return heatTaskHandle != NULL;
}

uint8_t heat_has_reached_target()
{
    return heat_target_reached;
}

void setHeatTargetTemperature(float target)
{
    heat_target_reached = 0;
    heat_target         = target;
    display_status();

    if (!isHeatTaskRunning())
    {
	startHeatTask();
    }
}

void setHeatDutyCycle(int duty_cycle)
{
    heat_duty_cycle = duty_cycle;
    display_status();
}
