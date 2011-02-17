///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 13 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "types.h"
#include "lcd.h"
#include "buttons.h"
#include "menu.h"
#include "brew.h"
#include "settings.h"
#include "heat.h"
#include "fill.h"
#include "brew_task.h"
#include "hop_droppers.h"
#include "crane.h"

#define TICKS_PER_MINUTE (60 * configTICK_RATE_HZ)

brew_task_t brew_task;

struct brew_step
{
    const char *name;
    void (*method)(int);
    int timeout;
};
#define BREW_STEPS_TOTAL 10
static struct brew_step g_steps[BREW_STEPS_TOTAL];

static struct state
{
    char         in_alarm;
    uint8_t      step;
    uint8_t      graphx;
    portTickType stepStartTick;
    uint8_t      hop_addition_done[MAX_HOP_ADDITIONS];
} g_state = { 0, 0, 0, 0};

static const char *brew_step_name(unsigned char step)
{
    if (step < BREW_STEPS_TOTAL)
        return g_steps[step].name;
    return "Unknown";
}

static void brew_run_step()
{
    g_state.stepStartTick = xTaskGetTickCount();
    g_steps[g_state.step].method(1);
}

static void brew_next_step()
{
    if (g_state.step >= BREW_STEPS_TOTAL)
    {
	return;
    }

    g_state.step++;
    brew_run_step();
}

void brew_next_step_if(int cond)
{
    if (cond)
	brew_next_step();
}

void brew_error_handler(brew_task_t *bt)
{
    lcd_printf(0, 2, 18, "%s task failed", bt->name);
    lcd_printf(0, 3, 18, "in step %d %s", g_state.step, brew_step_name(g_state.step));
    lcd_printf(0, 4, 18, "Error = %s", bt->error);
    brew_task.error = "Failed";
}

// STEP 1
void brew_reset_crane(int init)
{
}

// STEP 2
void brew_fill_and_heat(int init)
{
    if (init)
    {
	setHeatTargetTemperature(g_settings.mash_target_temp);
	setHeatDutyCycle(g_settings.mash_duty_cycle);
	fill_start(brew_error_handler);
    }
    else brew_next_step_if(!fill_is_running() && heat_has_reached_target());
}

// STEP 3
void brew_crane_to_mash(int init)
{
    if (init)   crane_move(DIRECTION_RIGHT, brew_error_handler);
    else brew_next_step_if(!crane_is_moving());
}

// STEP 4
void brew_mash_in(int init)
{
    if (init)	crane_move(DIRECTION_DOWN, brew_error_handler);
    else brew_next_step_if(!crane_is_moving());
}

// STEP 5
void brew_mash(int init)
{
    brew_next_step_if (xTaskGetTickCount() - g_state.stepStartTick > g_settings.mash_time * TICKS_PER_MINUTE);
}

// STEP 6
void brew_mash_out(int init)
{
    if (init)	crane_move(DIRECTION_UP, brew_error_handler);
    else brew_next_step_if (!crane_is_moving() &&
			    xTaskGetTickCount() - g_state.stepStartTick > 5 * TICKS_PER_MINUTE);
}

// STEP 7
void brew_to_boil(int init)
{
    if (init)
    {
	setHeatTargetTemperature(90.0f);
	setHeatDutyCycle(g_settings.boil_duty_cycle);
    }
    else brew_next_step_if (heat_has_reached_target());
}

// STEP 8
void brew_boil_stabilise(int init)
{
    if (init)
    {
	setHeatTargetTemperature(101.0f);
	setHeatDutyCycle(g_settings.boil_duty_cycle);
	crane_move(DIRECTION_LEFT, brew_error_handler);
    }
    else brew_next_step_if (!crane_is_moving());
}

// STEP 9
void brew_boil_hops(int init)
{
    int ii;
    int minute_from_start = (xTaskGetTickCount() - g_state.stepStartTick) / TICKS_PER_MINUTE;
    int minute_from_end   = g_settings.boil_time - minute_from_start;

    for (ii = 0; ii < MAX_HOP_ADDITIONS; ii++)
    {
	if (init)
	{
	    g_state.hop_addition_done[ii] = 0;
	}
	else if (minute_from_end <= g_settings.hop_addition[ii] &&
		 !g_state.hop_addition_done[ii])
	{
	    g_state.hop_addition_done[ii] = 1;

	    hop_drop(ii);
	}
    }

    brew_next_step_if (xTaskGetTickCount() - g_state.stepStartTick > g_settings.boil_time * TICKS_PER_MINUTE);
}

// STEP 10
void brew_finish(int init)
{
    if (init)
    {
	stopHeatTask();
    }
    else
    {

    }
}

static struct brew_step g_steps[BREW_STEPS_TOTAL] = 
{
    {"Reset crane",        brew_reset_crane,     0},
    {"Fill & Heat water",  brew_fill_and_heat,   0},
    {"Crane to mash",      brew_crane_to_mash,   0},
    {"Mash In",            brew_mash_in,         0},
    {"Mash",               brew_mash,            0},
    {"Crane out mash",     brew_mash_out,        0},
    {"Bring to boil",      brew_to_boil,         0},
    {"Boil stabilize",     brew_boil_stabilise,  0},
    {"Boil & Hops",        brew_boil_hops,       0},
    {"Finish",             brew_finish,          0},
};

void brew_start_cb(brew_task_t *bt)
{
    brew_run_step();
}

void brew_iterate_cb(brew_task_t *bt)
{
    g_steps[g_state.step].method(0);
    vTaskDelay(50);
}

void brew_stop_cb(brew_task_t *bt)
{
    // all off
    fill_stop();
    stopHeatTask();
    crane_stop();
}

void brew_start(int init)
{
    if (init)
    {
	g_state.step = 0;
	brewTaskStart(&brew_task, NULL);
    }
}

void brew_start_task()
{
    startBrewTask(&brew_task,
		  "Brew", 700, 3, portMAX_DELAY,
		  brew_start_cb,
		  brew_iterate_cb,
		  brew_stop_cb);
}

int brew_key_handler(unsigned char key)
{
    return 1;
}

void brew_resume(int init)
{
    g_state.in_alarm = 0;
    lcd_printf(0, 2, 19, "Resume at step:");
    lcd_printf(0, 6, 19, "(> to resume)");
    brew_resume_key(KEY_PRESSED);
}

int brew_resume_key(unsigned char key)
{
   if ((key & KEY_PRESSED) == 0)
        return 1;

    if (key & KEY_UP && g_state.step < BREW_STEPS_TOTAL - 1)
        g_state.step++;
    if (key & KEY_DOWN && g_state.step != 0)
        g_state.step--;

    if (key & KEY_RIGHT)
    {
        menu_run_applet(brew_key_handler);
	brewTaskStart(&brew_task, NULL);
    }
    else
    {
        lcd_printf(16, 2, 19, "%d", g_state.step + 1);
        lcd_printf(0,  4, 19, " %s", brew_step_name(g_state.step));
    }
    return 1;
}
