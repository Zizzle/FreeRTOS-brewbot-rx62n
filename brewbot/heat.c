#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "crane.h"
#include "semphr.h"
#include "ds1820.h"
#include "lcd.h"
#include "level_probes.h"
#include <stdio.h>
#include "brew_task.h"
static void display_status();

#define NUM_READINGS 5

static struct brew_task heat_task;
static volatile float   heat_target = 0.0f;
static volatile int     heat_duty_cycle = 50;
static volatile uint8_t heat_target_reached;
static float heat_readings[NUM_READINGS];
static uint8_t heat_reading_idx;
static uint8_t heat_reading_cnt;

static void allOff()
{
    outputOff(SSR);
    SSR_DDR = 1;

    // turn on power to the ds1820
    PORT4.DR.BIT.B1 = 1;
    PORT4.DDR.BIT.B1 = 1;
}

static void display_status()
{
    lcd_printf(0, 4, 19, "Target %.1f", heat_target);
    lcd_printf(0, 5, 18, "Temp %.2f (%d%)",
	       (double)ds1820_get_temperature(), heat_duty_cycle);
    lcd_printf(0,6, 19, "Probe: %d %d",level_hit_heat(), level_probe_heat_adc());
    lcd_printf(0, 7, 19, "Target hit %d", heat_target_reached);
}

static void heat_keep_temperature()
{
    heat_readings[heat_reading_idx] = ds1820_get_temperature();
    if (++heat_reading_idx >= NUM_READINGS)
    {
	heat_reading_idx  = 0;
    }

    if (heat_reading_cnt < NUM_READINGS)
	heat_reading_cnt++; 
}

static uint8_t test_had_reached_target()
{
    int ii;

    if (heat_target_reached)
	return 1;

    if (heat_reading_cnt < NUM_READINGS)
	return 0;

    for (ii = 0; ii < heat_reading_cnt; ii++)
    {
	if (heat_readings[ii] < heat_target)
	    return 0;
    }
    return 1;
}

static void _heat_start(struct brew_task *bt)
{
    allOff();

    // kick off reading the temp
    vTaskEnterCritical();
    DS1820Init();
    DS1820Skip();
    DS1820Convert();
    vTaskExitCritical();

    vTaskDelay(1000);

    vTaskEnterCritical();
    DS1820ReadTemp();
    vTaskExitCritical();    
}

static void heat_iteration(struct brew_task *bt)
{
    int ii = 0;
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
	    heat_target_reached = test_had_reached_target();
	    allOff();
	}
	vTaskDelay(10); // wait for the conversion to happen
    }

    vTaskEnterCritical();
    DS1820ReadTemp();
    vTaskExitCritical();
    heat_keep_temperature();
}

void _heat_stop(struct brew_task *bt)
{
    allOff();
}

void heat_start_task()
{
    startBrewTask(&heat_task,
		  "Heat", 400, 2, 10000000,
		  _heat_start,
		  heat_iteration,
		  _heat_stop);
}

void heat_start(void (*taskErrorHandler)(brew_task_t *))
{
    brewTaskStart(&heat_task, taskErrorHandler);
}

void heat_stop()
{
    brewTaskStop(&heat_task);
}

char heat_task_is_running()
{
    return heat_task.running;
}

uint8_t heat_has_reached_target()
{
    return heat_target_reached;
}

void heat_set_target_temperature(float target)
{
    heat_target_reached = 0;
    heat_target         = target;
    display_status();
}

void heat_set_dutycycle(int duty_cycle)
{
    heat_duty_cycle = duty_cycle;
    display_status();
}
