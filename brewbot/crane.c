#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "crane.h"
#include "semphr.h"
#include "lcd.h"

static int crane_direction = 0;
static xSemaphoreHandle run;
static xSemaphoreHandle finished;

void craneMove(int direction)
{
    crane_direction = direction;
    xSemaphoreGive(run);
    // Block, waiting on the semaphore.
    xSemaphoreTake(finished, portMAX_DELAY);
}

void craneWaitForMoveFinish()
{
    // Block, waiting on the semaphore.
    xSemaphoreTake(finished, portMAX_DELAY);
}

static void allOff()
{
    outputOff(MOTOR_CRANE_X);
    outputOff(MOTOR_CRANE_Y);
    outputOff(MOTOR_DIR);

    MOTOR_DIR_DDR     = 1;
    MOTOR_CRANE_X_DDR = 1;
    MOTOR_CRANE_Y_DDR = 1;
}

static void limitSwitchCheckTask( void *pvParameters )
{
    allOff();

    vSemaphoreCreateBinary(run);
    vSemaphoreCreateBinary(finished);
    xSemaphoreTake(run, portMAX_DELAY);
    xSemaphoreTake(finished, portMAX_DELAY);

    for(;;)
    {
	allOff();
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
	}

	char moving = 1;
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


	    vTaskEnterCritical();
	    if ((crane_direction == DIRECTION_UP    && CRANE_END_SW_UP == 0) ||
		(crane_direction == DIRECTION_DOWN  && CRANE_END_SW_DOWN == 0) ||
		(crane_direction == DIRECTION_LEFT  && CRANE_END_SW_LEFT == 0) ||
		(crane_direction == DIRECTION_RIGHT && CRANE_END_SW_RIGHT == 0))
	    {
		allOff();
		moving = 0;
	    }
	    vTaskExitCritical();

	    vTaskDelay(10);
	}
	xSemaphoreGive(finished);
   }    
}

void startCraneLimitSwitchTask()
{
    xTaskCreate( limitSwitchCheckTask,
		 ( signed char * ) "LimitSwitch",
		 configMINIMAL_STACK_SIZE, NULL,
		 2, ( xTaskHandle  * ) NULL );
}
