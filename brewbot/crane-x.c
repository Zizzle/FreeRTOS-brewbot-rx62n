#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"

static void limitSwitchCheckTask( void *pvParameters )
{
    MOTOR_CRANE_X = 1;
    vTaskDelay(3000);
    MOTOR_CRANE_X = 0;

    for(;;)
    {
	vTaskDelay(100);
	
	lcd_string(6,0, "sw: ");

	PORTD.DDR.BIT.B0 = 0;
	PORTD.DDR.BIT.B3 = 0;

	lcd_display_number(PORTD.PORT.BIT.B0);
	lcd_display_char(' ');
	lcd_display_number( PORTD.PORT.BIT.B3);

	vTaskEnterCritical();
	if (PORTD.PORT.BIT.B0 == 0 || PORTD.PORT.BIT.B3 == 0)
	    MOTOR_CRANE_X = 1;
	vTaskExitCritical();
   }    
}

void startLimitSwitchTask()
{
    xTaskCreate( limitSwitchCheckTask,
		 ( signed char * ) "LimitSwitch",
		 configMINIMAL_STACK_SIZE, NULL,
		 2, ( xTaskHandle  * ) NULL );
}
