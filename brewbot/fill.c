///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 15 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "semphr.h"
#include "lcd.h"
#include "level_probes.h"

static xSemaphoreHandle run;
static volatile uint8_t filling;

static void display_status();

static void allOff()
{
    outputOff(SOLENOID);
    SOLENOID_DDR = 1;
}

static void display_status()
{
    lcd_text(8, 0, "Filling");

    lcd_printf(0, 3, 19, "Probes: %d %d",
	       level_probe_heat_adc(),
	       level_probe_full_adc());
    lcd_printf(0, 4, 19, "Probes: %d %d",
	       level_hit_heat(),
	       level_hit_full());
}

static void fillTask( void *pvParameters )
{
    vSemaphoreCreateBinary(run);
    xSemaphoreTake(run, portMAX_DELAY);

    for (;;)
    {
	allOff();
	
	// wait for another task to tell us to run
	if (xSemaphoreTake(run, portMAX_DELAY) != pdTRUE )
	    continue;

	LEVEL_PROBE_HEAT_DDR = 0;
	LEVEL_PROBE_FULL_DDR = 0;
	level_probe_init();
	for (;;)
	{

	    SOLENOID = level_hit_heat();



	    display_status();
	}
    }
    xSemaphoreGive(run);
    allOff();
}

void startFillTask()
{
    xTaskCreate( fillTask,
		 ( signed char * ) "FillTask",
		 configMINIMAL_STACK_SIZE + 600, NULL,
		 4, ( xTaskHandle  * ) NULL );
}

void fillStart()
{
    xSemaphoreGive(run);
}

void fillStop()
{

}
