///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, licensed under the GPL3.
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
#include "brew_task.h"

#define MAX_FILL_TIME 100 * 1000

brew_task_t fill_task;

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
    lcd_text(8, 0, "Filling");

    lcd_printf(0, 1, 19, "Probes: %d %d",
	       level_probe_heat_adc(),
	       level_probe_full_adc());
    lcd_printf(0, 2, 19, "Probes: %d %d",
	       level_hit_heat(),
	       level_hit_full());
    lcd_printf(0, 3, 19, "Fill time: %d s",
	       (xTaskGetTickCount() - bt->startTick) / 1000);
}

static void fill_job_start_stop(brew_task_t *bt)
{
    allOff();
}

static void fill(brew_task_t *bt)
{
    SOLENOID = level_hit_full();
    display_status(bt);

    if (level_hit_full())
	bt->running = 0;

    vTaskDelay(10);
}

void start_fill_task()
{
    startBrewTask(&fill_task,
		  "Fill Task", 200, 5, MAX_FILL_TIME,
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

