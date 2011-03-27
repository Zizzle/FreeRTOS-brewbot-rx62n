///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date: 15 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

//
// The top probe sits at about the 3 gallon mark.
// 2" per gallon. 4.0625" appart.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "semphr.h"
#include "lcd.h"
#include "level_probes.h"
#include "brew_task.h"

#define MAX_FILL_TIME (600 * 1000)

#define PROBE_SEPARATION (4.0625f)
#define FULL_PROBE_VOLUME (3.0f)
#define INCHES_PER_GALLON (2.0f)

static brew_task_t fill_task;
static portTickType time_heat_hit;  // time at which the lower probe was hit
static portTickType time_finish;    // when to stop the fill
static float target = 3.5;

static void display_status();

static void allOff()
{
    outputOff(SOLENOID);
    SOLENOID_DDR = 1;
    LEVEL_PROBE_HEAT_DDR = 0;
    LEVEL_PROBE_FULL_DDR = 0;
}

static void display_status(brew_task_t *bt)
{
    lcd_printf(0, 1, 19, "Probes: %d %d",
	       level_probe_heat_adc(),
	       level_probe_full_adc());
    lcd_printf(0, 2, 19, "Probes: H %d F %d",
	       level_hit_heat(),
	       level_hit_full());
    lcd_printf(0, 3, 19, "Fill time: %d s",
	       (xTaskGetTickCount() - bt->startTick) / 1000);

    if (time_finish == 0)
    {
	if (time_heat_hit != 0)
	    lcd_printf(0, 4, 19, "Since lower: %d s",
		       (xTaskGetTickCount() - time_heat_hit) / 1000);	
    }
    else
    {
	lcd_printf(0, 4, 19, "Left: %d s",
		   (xTaskGetTickCount() - time_finish) / 1000);
    }
}

static void fill_job_start_stop(brew_task_t *bt)
{
    allOff();

    time_heat_hit = 0;
    time_finish = 0;

    // make sure we have consistent readings on the level probes
    level_wait_for_steady_readings();

    // if the lower probe is hit then we can't measure the time between them
    if (level_hit_heat() == 1)
    {
	bt->error = "Water already";
    }
}

static void fill(brew_task_t *bt)
{
    outputOn(SOLENOID);
    display_status(bt);

    if (level_hit_full() == 1)
    {
	if (time_finish == 0)
	{
	    portTickType took    = xTaskGetTickCount() - time_heat_hit;
	    float ticks_per_inch = took / PROBE_SEPARATION;
	    float vol_left       = target - FULL_PROBE_VOLUME;
	    float inches_left    = vol_left * INCHES_PER_GALLON;
	    time_finish          = xTaskGetTickCount() + (inches_left * ticks_per_inch);
	}

	// are we done?
	if (xTaskGetTickCount() >= time_finish)
	    bt->running = 0;
    }

    if (level_hit_heat() == 1 && time_heat_hit == 0)
	time_heat_hit = xTaskGetTickCount();

    vTaskDelay(10);
}

void start_fill_task()
{
    startBrewTask(&fill_task,
		  "Fill", 200, 5, MAX_FILL_TIME,
		  fill_job_start_stop,
		  fill,
		  fill_job_start_stop);
}

uint8_t fill_is_running()
{
    return fill_task.running;
}

void fill_start(void (*taskErrorHandler)(brew_task_t *))
{
    if (fill_is_running())
	return;

    brewTaskStart(&fill_task, taskErrorHandler);
}

void fill_stop()
{
    brewTaskStop(&fill_task);    
}

