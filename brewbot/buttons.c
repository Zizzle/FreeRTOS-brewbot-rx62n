#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "lcd.h"
#include "menu.h"
#include "buttons.h"

static void buttonChanged(unsigned char key)
{
    menu_key(key);
}

static void buttonsCheckTask( void *pvParameters )
{
    uint8_t now  = 0;
    uint8_t last = 0;
    uint8_t changed;
    int ii;

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

#if 0
	MOTOR_DIR_DDR = 1;
	MOTOR_CRANE_X_DDR = 1;
	MOTOR_CRANE_Y_DDR = 1;

	MOTOR_DIR = BUTTON_UP;
	MOTOR_CRANE_X = BUTTON_RIGHT;
	MOTOR_CRANE_Y = BUTTON_LEFT;
#endif

	// get the button inputs from the PORT register. Since they are on
	// pull-ups, invent the result
	now = ~(BUTTON_PORT >> 4);	
	changed = now ^ last; // only interested in the change of state

	for (ii = 0; ii < 4; ii++)
	{
	    uint8_t button = (1 << ii);
	    if (changed & button)
	    {
		buttonChanged(button | (now & button ? KEY_PRESSED : 0));
	    }
	}
	last = now;
   }
}

void startButtonsTask()
{
    xTaskCreate( buttonsCheckTask,
		 (signed char *) "Buttons",
		 configMINIMAL_STACK_SIZE + 300, NULL,
		 1, ( xTaskHandle  * ) NULL );

}
