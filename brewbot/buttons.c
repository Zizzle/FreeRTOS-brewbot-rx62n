#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"

static void buttonsCheckTask( void *pvParameters )
{
    for(;;)
    {
	PORTE.DDR.BYTE = 0;
	PORTE.PORT.BYTE = 0xff;

	vTaskDelay(10);
	
	lcd_string(7,0, "bt: ");

	lcd_display_number(BUTTON_UP);
	lcd_display_char(' ');
	lcd_display_number(BUTTON_DOWN);
	lcd_display_char(' ');
	lcd_display_number(BUTTON_LEFT);
	lcd_display_char(' ');
	lcd_display_number(BUTTON_RIGHT);
   }    
}

void startButtonsTask()
{
    xTaskCreate( buttonsCheckTask,
		 ( signed char * ) "Buttons",
		 configMINIMAL_STACK_SIZE, NULL,
		 1, ( xTaskHandle  * ) NULL );

    PORTE.PORT.BIT.B4 = 1;
}
