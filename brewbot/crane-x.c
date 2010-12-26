#include "FreeRTOS.h"
#include "task.h"

static void limitSwitchCheckTask( void *pvParameters )
{
    portTickType xLastCheckTime = xTaskGetTickCount();

    /* The parameters are not used. */
    ( void ) pvParameters;

    for(;;)
    {
	vTaskDelayUntil( &xLastCheckTime, 100 );
    }    
}

void startLimitSwitchTask()
{
    xTaskCreate( limitSwitchCheckTask,
		 ( signed char * ) "LimitSwitch",
		 configMINIMAL_STACK_SIZE, NULL,
		 2, ( xTaskHandle  * ) NULL );
}
