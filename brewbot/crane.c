#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "crane.h"
#include "semphr.h"
#include "lcd.h"

static int crane_direction = 0;
static volatile char moving = 1;
static xSemaphoreHandle run;

#define UP_DOWN_TIMEOUT     60
#define LEFT_RIGHT_TIMEOUT (5 * 60)

static void allOff()
{
    moving = 0;
    outputOff(MOTOR_CRANE_X);
    outputOff(MOTOR_CRANE_Y);
    outputOff(MOTOR_DIR);

    MOTOR_DIR_DDR     = 1;
    MOTOR_CRANE_X_DDR = 1;
    MOTOR_CRANE_Y_DDR = 1;
}

static void limitSwitchCheckTask( void *pvParameters )
{
    vSemaphoreCreateBinary(run);
    xSemaphoreTake(run, portMAX_DELAY);

    for (;;)
    {
	allOff();
	
	// wait for another task to tell us to run
	if (xSemaphoreTake(run, portMAX_DELAY) != pdTRUE )
	    continue;


	switch (crane_direction)
	{
	case DIRECTION_UP:
	    outputOff (MOTOR_DIR);
	    outputOn  (MOTOR_CRANE_Y);
	    break;
	case DIRECTION_DOWN:
	    outputOn  (MOTOR_DIR);
	    outputOn  (MOTOR_CRANE_Y);
	    break;
	case DIRECTION_LEFT:
	    outputOn  (MOTOR_DIR);
	    outputOn  (MOTOR_CRANE_X);
	    break;
	case DIRECTION_RIGHT:
	    outputOff (MOTOR_DIR);
	    outputOn  (MOTOR_CRANE_X);
	    break;
	default:
	    continue;
	}

	while(moving)
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
		allOff(); // this will set moving = 0
		break;
	    }

	    vTaskDelay(10);
	}
    }
}

void startCraneTask()
{
    xTaskCreate( limitSwitchCheckTask,
		 ( signed char * ) "Crane",
		 configMINIMAL_STACK_SIZE, NULL,
		 2, NULL );
}


void craneMove(int direction)
{
    crane_direction = direction;
    moving = 1;
    xSemaphoreGive(run);
}

void craneStop()
{
    allOff();  // try an immediate stop
}
