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
#include <stdarg.h>

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
#include "fatfs/ff.h"
#include "brewbot.h"

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
    portTickType brew_start_tick;
    portTickType step_start_tick;
    long         total_runtime;  // seconds since the start of the brew
    long         step_runtime;   // seconds since the start of the current step
    uint8_t      hop_addition_done[MAX_HOP_ADDITIONS];
} g_state = { 0, 0, 0, 0};

static void brew_log(char *fmt, ...)
{
    char message[40];
    UINT written;
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(message, sizeof(message) - 1, fmt, ap);
    va_end(ap);

//    f_write(&g_state.file, message, len, &written);
}

static const char *brew_step_name(unsigned char step)
{
    if (step < BREW_STEPS_TOTAL)
        return g_steps[step].name;
    return "Unknown";
}

static void brew_run_step()
{
    g_state.step_start_tick = xTaskGetTickCount();
    g_steps[g_state.step].method(1);

    lcd_clear();

    lcd_printf(0, 0, 14, "%d.%s", g_state.step, g_steps[g_state.step].name);
}

static void brew_next_step()
{
    if (g_state.step >= BREW_STEPS_TOTAL)
    {
	return;
    }

    STIRRER_DDR = 0;

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
    if (init)
    {
	if (!crane_is_at_top())
	    crane_move(DIRECTION_UP, brew_error_handler);
    }
    
    if (!crane_is_moving() && !crane_is_at_left() && crane_is_at_top())
	crane_move(DIRECTION_LEFT, brew_error_handler);

    brew_next_step_if (crane_is_at_left() && crane_is_at_top());
}

// STEP 2
void brew_fill_and_heat(int init)
{
    if (init)
    {
	heat_start(brew_error_handler);
	heat_set_target_temperature(g_settings.mash_target_temp);
	heat_set_dutycycle(70);
	fill_start(brew_error_handler);
    }
    else brew_next_step_if(!fill_is_running() && heat_has_reached_target());
}

// STEP 3
void brew_crane_to_mash(int init)
{
    if (init)
    {
	crane_move(DIRECTION_RIGHT, brew_error_handler);

	heat_start(brew_error_handler);
	heat_set_target_temperature(g_settings.mash_target_temp);
	heat_set_dutycycle(g_settings.mash_duty_cycle);
    }
    else brew_next_step_if(!crane_is_moving());
}

// STEP 4
void brew_mash_in(int init)
{
    if (init)
    {
	crane_move(DIRECTION_DOWN, brew_error_handler);

	heat_start(brew_error_handler);
	heat_set_target_temperature(g_settings.mash_target_temp);
	heat_set_dutycycle(g_settings.mash_duty_cycle);
    }
    else brew_next_step_if(!crane_is_moving());
}

// STEP 5
void brew_mash(int init)
{
    long remain = g_settings.mash_time * 60 - g_state.step_runtime;
    if (init)
    {
	STIRRER_DDR = 1;
	outputOn(STIRRER);

	heat_start(brew_error_handler);
	heat_set_target_temperature(g_settings.mash_target_temp);
	heat_set_dutycycle(g_settings.mash_duty_cycle);
    }

    lcd_printf(0, 1, 19, "%.2d:%.2d Elapsed", g_state.step_runtime / 60,
	       g_state.step_runtime % 60);
    lcd_printf(0, 2, 19, "%.2d:%.2d Remaining", remain / 60, remain % 60);

    brew_next_step_if (g_state.step_runtime > g_settings.mash_time * 60);
}

// STEP 6
void brew_mash_out(int init)
{
    long remain = g_settings.mash_out_time * 60 - g_state.step_runtime;

    if (init)
    {
	heat_start(brew_error_handler);
	heat_set_target_temperature(90.0f);
	heat_set_dutycycle(g_settings.boil_duty_cycle);
    }
    else brew_next_step_if (!crane_is_moving() &&
			    g_state.step_runtime > g_settings.mash_out_time * 60);


    if (g_state.step_runtime < 50)
    {
	if ((g_state.step_runtime % 5) == 0)
	    crane_move(DIRECTION_UP, brew_error_handler);
	else
	    crane_stop();
    }
    else if (!crane_is_at_top() && !crane_is_moving())
    {
	crane_move(DIRECTION_UP, brew_error_handler);
    }

    lcd_printf(0, 1, 19, "%.2d:%.2d Elapsed", g_state.step_runtime / 60,
	       g_state.step_runtime % 60);
    lcd_printf(0, 2, 19, "%.2d:%.2d Remaining", remain / 60, remain % 60);

}

// STEP 7
void brew_to_boil(int init)
{
    if (init)
    {
	heat_start(brew_error_handler);
	heat_set_target_temperature(90.0f);
	heat_set_dutycycle(g_settings.boil_duty_cycle);
    }
    else brew_next_step_if (heat_has_reached_target());
    brew_next_step();
}

// STEP 8
void brew_boil_stabilise(int init)
{
    if (init)
    {
	crane_move(DIRECTION_LEFT, brew_error_handler);
	heat_start(brew_error_handler);
	heat_set_target_temperature(101.0f);
	heat_set_dutycycle(g_settings.boil_duty_cycle);
    }
    else brew_next_step_if (!crane_is_moving());
}

// STEP 9
void brew_boil_hops(int init)
{
    int ii;
    long remain = g_settings.boil_time * 60 - g_state.step_runtime;

    for (ii = 0; ii < HOP_DROPPER_NUM; ii++)
    {
	long drop_second = g_settings.hop_addition[ii] * 60; 

	if (g_state.hop_addition_done[ii] == 0)
	    lcd_printf(0, 1 + ii, 19, "Hops %d in %.2d:%.2d", ii + 1,
		       (remain - drop_second) / 60,
		       (remain - drop_second) % 60);

	if (init)
	{
	    g_state.hop_addition_done[ii] = 0;
	}
	else if (remain < drop_second && !g_state.hop_addition_done[ii])
	{
	    g_state.hop_addition_done[ii] = 1;
	    hops_drop(ii, brew_error_handler);
	}
    }

    brew_next_step_if (g_state.step_runtime / 60 > g_settings.boil_time);
}

// STEP 10
void brew_finish(int init)
{
    if (init)
    {
	heat_stop();
    }
    else
    {

    }
}

static struct brew_step g_steps[BREW_STEPS_TOTAL] = 
{
    {"Reset crane",        brew_reset_crane,     0},
    {"Fill & Heat",        brew_fill_and_heat,   0},
    {"Crane to mash",      brew_crane_to_mash,   0},
    {"Mash In",            brew_mash_in,         0},
    {"Mash",               brew_mash,            0},
    {"Crane out",          brew_mash_out,        0},
    {"Bring to boil",      brew_to_boil,         0},
    {"Boil stable",        brew_boil_stabilise,  0},
    {"Boil & Hops",        brew_boil_hops,       0},
    {"Finish",             brew_finish,          0},
};

void brew_start_cb(brew_task_t *bt)
{
    brew_run_step();
}

void brew_iterate_cb(brew_task_t *bt)
{
    // caclulate the runtimes
    g_state.total_runtime = (xTaskGetTickCount() - g_state.brew_start_tick) / configTICK_RATE_HZ;
    g_state.step_runtime  = (xTaskGetTickCount() - g_state.step_start_tick) / configTICK_RATE_HZ;

    // run the current step
    g_steps[g_state.step].method(0);
    vTaskDelay(100);

    // display the total run time
    lcd_printf(14, 0, 5, "%.2d:%.2d", g_state.total_runtime / 60, g_state.total_runtime % 60);
}

void brew_stop_cb(brew_task_t *bt)
{
    // all off
    fill_stop();
    heat_stop();
    crane_stop();
    STIRRER_DDR = 0;
}

void brew_start(int init)
{
    if (init)
    {
	g_state.brew_start_tick = xTaskGetTickCount();
	g_state.step = 0;
	brewTaskStart(&brew_task, NULL);
    }
    else
    {
	brewTaskStop(&brew_task);	
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
    if (init)
    {
	g_state.in_alarm = 0;
	lcd_printf(0, 2, 19, "Resume at step:");
	lcd_printf(0, 6, 19, "(> to resume)");
	brew_resume_key(KEY_PRESSED);
    }
    else
    {
	brewTaskStop(&brew_task);
    }
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
