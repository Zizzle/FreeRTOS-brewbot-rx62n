#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "crane.h"
#include "mash.h"

static char lastU = 1;
static char lastD = 1;
static char lastL = 1;
static char lastR = 1;

static float target = 66.0f;
static int duty = 50;

static void buttonUp()
{
//    craneMove(DIRECTION_UP);
    target += 0.5;
    setMashTargetTemperature(target);
}

static void buttonDown()
{
    target -= 0.5;
    setMashTargetTemperature(target);
//    craneMove(DIRECTION_DOWN);
}

static void buttonLeft()
{
//    craneMove(DIRECTION_LEFT);
    if (duty <= 10)
	duty--;
    else
	duty -= 10;
    setMashDutyCycle(duty);
}

static void buttonRight()
{
//    craneMove(DIRECTION_RIGHT);
    if (duty < 10)
	duty++;
    else
	duty += 10;
    setMashDutyCycle(duty);
}

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

#if 0
	MOTOR_DIR_DDR = 1;
	MOTOR_CRANE_X_DDR = 1;
	MOTOR_CRANE_Y_DDR = 1;

	MOTOR_DIR = BUTTON_UP;
	MOTOR_CRANE_X = BUTTON_RIGHT;
	MOTOR_CRANE_Y = BUTTON_LEFT;
#endif
	if (BUTTON_UP    == 0 && BUTTON_UP    != lastU)
	    buttonUp();
	if (BUTTON_DOWN  == 0 && BUTTON_DOWN  != lastD)
	    buttonDown();
	if (BUTTON_LEFT  == 0 && BUTTON_LEFT  != lastL)
	    buttonLeft();
	if (BUTTON_RIGHT == 0 && BUTTON_RIGHT != lastR)
	    buttonRight();

	lastU = BUTTON_UP;
	lastD = BUTTON_DOWN;
	lastL = BUTTON_LEFT;
	lastR = BUTTON_RIGHT;
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
