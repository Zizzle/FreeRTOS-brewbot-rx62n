#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "crane.h"
#include "semphr.h"
#include "lcd.h"

static int crane_direction = 0;
static brew_task_t crane_task;

#define UP_DOWN_TIMEOUT     60
#define LEFT_RIGHT_TIMEOUT (5 * 60)

static void allOff()
{
    outputOff(MOTOR_CRANE_X);
    outputOff(MOTOR_CRANE_Y);
    outputOff(MOTOR_DIR);

    MOTOR_DIR_DDR     = 1;
    MOTOR_CRANE_X_DDR = 1;
    MOTOR_CRANE_Y_DDR = 1;
}


static void crane_start(brew_task_t *bt)
{
    allOff();
    switch (crane_direction)
    {
    case DIRECTION_UP:
	outputOff (MOTOR_DIR);
	outputOn  (MOTOR_CRANE_Y);
	bt->maxRunTime = UP_DOWN_TIMEOUT * configTICK_RATE_HZ;
	break;
    case DIRECTION_DOWN:
	outputOn  (MOTOR_DIR);
	outputOn  (MOTOR_CRANE_Y);
	bt->maxRunTime = UP_DOWN_TIMEOUT * configTICK_RATE_HZ;
	break;
    case DIRECTION_LEFT:
	outputOff  (MOTOR_DIR);
	outputOn  (MOTOR_CRANE_X);
	bt->maxRunTime = LEFT_RIGHT_TIMEOUT * configTICK_RATE_HZ;
	break;
    case DIRECTION_RIGHT:
	outputOn (MOTOR_DIR);
	outputOn  (MOTOR_CRANE_X);
	bt->maxRunTime = LEFT_RIGHT_TIMEOUT * configTICK_RATE_HZ;
	break;
    }
}


static void crane_iteration(brew_task_t *bt)
{
    lcd_string(6,0, "sw: ");

    lcd_display_number(CRANE_END_SW_LEFT);
    lcd_display_char(' ');
    lcd_display_number(CRANE_END_SW_RIGHT);
    lcd_display_char(' ');
    lcd_display_number(CRANE_END_SW_UP);
    lcd_display_char(' ');
    lcd_display_number(CRANE_END_SW_DOWN);

    if ((crane_direction == DIRECTION_UP    && CRANE_END_SW_UP == 0) ||
	(crane_direction == DIRECTION_DOWN  && CRANE_END_SW_DOWN == 0) ||
	(crane_direction == DIRECTION_LEFT  && CRANE_END_SW_LEFT == 0) ||
	(crane_direction == DIRECTION_RIGHT && CRANE_END_SW_RIGHT == 0))
    {
	bt->running = 0;
	allOff();
	return;
    }

    vTaskDelay(10);
}

static void crane_stop_(brew_task_t *bt)
{
    allOff();
}

void start_crane_task()
{
    startBrewTask(&crane_task,
		  "Crane Task", 200, 5, 1000000,
		  crane_start,
		  crane_iteration,
		  crane_stop_);
}

uint8_t crane_is_moving()
{
    return crane_task.running;
}

void crane_move(int direction, void (*taskErrorHandler)(brew_task_t *))
{
    if (crane_is_moving())
	return;

    crane_direction = direction;
    brewTaskStart(&crane_task, taskErrorHandler);
}

void crane_stop()
{
    brewTaskStop(&crane_task);    
}

