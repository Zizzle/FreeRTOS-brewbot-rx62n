#include "iodefine.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "ds1820.h"
#include "buttons.h"
#include "crane.h"
#include "menu.h"
#include "diagnostics.h"

/* Priorities at which the tasks are created. */
#define mainCHECK_TASK_PRIORITY		( configMAX_PRIORITIES - 1 )
#define mainQUEUE_POLL_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainSEM_TEST_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define mainCREATOR_TASK_PRIORITY   ( tskIDLE_PRIORITY + 3 )
#define mainFLASH_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainuIP_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define mainINTEGER_TASK_PRIORITY   ( tskIDLE_PRIORITY )
#define mainGEN_QUEUE_TASK_PRIORITY	( tskIDLE_PRIORITY )
#define mainFLOP_TASK_PRIORITY		( tskIDLE_PRIORITY )

/* The WEB server uses string handling functions, which in turn use a bit more
stack than most of the other tasks. */
#define mainuIP_STACK_SIZE			( configMINIMAL_STACK_SIZE * 3 )

/* The LED toggled by the check task. */
#define mainCHECK_LED				( 5 )


/*
 * vApplicationMallocFailedHook() will only be called if
 * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
 * function that will execute if a call to pvPortMalloc() fails.
 * pvPortMalloc() is called internally by the kernel whenever a task, queue or
 * semaphore is created.  It is also called by various parts of the demo
 * application.
 */
void vApplicationMallocFailedHook( void );

/*
 * vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set to 1
 * in FreeRTOSConfig.h.  It is a hook function that is called on each iteration
 * of the idle task.  It is essential that code added to this hook function
 * never attempts to block in any way (for example, call xQueueReceive() with
 * a block time specified).  If the application makes use of the vTaskDelete()
 * API function (as this demo application does) then it is also important that
 * vApplicationIdleHook() is permitted to return to its calling function because
 * it is the responsibility of the idle task to clean up memory allocated by the
 * kernel to any task that has since been deleted.
 */
void vApplicationIdleHook( void );

/*
 * vApplicationStackOverflowHook() will only be called if
 * configCHECK_FOR_STACK_OVERFLOW is set to a non-zero value.  The handle and
 * name of the offending task should be passed in the function parameters, but
 * it is possible that the stack overflow will have corrupted these - in which
 * case pxCurrentTCB can be inspected to find the same information.
 */
void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName );

/*
 * Contains the implementation of the WEB server.
 */
extern void vuIP_Task( void *pvParameters );

extern void vSetupHighFrequencyTimer( void );

/*-----------------------------------------------------------*/

struct menu main_menu[] =
{
//    {"Settings",          settings_menu, settings_display, NULL},
//    {"Brew Start",        NULL,          brew,             brew_key_handler},
//    {"Brew Resume",       NULL,          brew_resume,      brew_resume_key},
//    {"Clean up",          foo_menu,      NULL,             NULL},
    {"Diagnostics",       diag_menu,     NULL,             NULL},
    {"Test 123",          NULL,          NULL,             NULL},
    {"Blah Blah",         NULL,          NULL,             NULL},
    {NULL, NULL, NULL, NULL}
};


/*-----------------------------------------------------------*/

int main(void)
{
    extern void HardwareSetup( void );

    /* Renesas provided CPU configuration routine.  The clocks are configured in
       here. */
    HardwareSetup();

    lcd_open();
    lcd_set_address(0, 0);

    lcd_string(2,0, "Welcome to brewbot");
    lcd_string(5,0, "IP: ");
    lcd_display_number(configIP_ADDR0);
    lcd_display_char('.');
    lcd_display_number(configIP_ADDR1);
    lcd_display_char('.');
    lcd_display_number(configIP_ADDR2);
    lcd_display_char('.');
    lcd_display_number(configIP_ADDR3);

    /* The web server task. */
    xTaskCreate( vuIP_Task, ( signed char * ) "uIP", mainuIP_STACK_SIZE, NULL, mainuIP_TASK_PRIORITY, NULL );


    menu_set_root(main_menu);

//    startCraneLimitSwitchTask();
    startButtonsTask();

    vSetupHighFrequencyTimer();

    /* Start the tasks running. */
    vTaskStartScheduler();

    /* If all is well we will never reach here as the scheduler will now be
       running.  If we do reach here then it is likely that there was insufficient
       heap available for the idle task to be created. */
    for( ;; );
	
    return 0;
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/* The RX port uses this callback function to configure its tick interrupt.
This allows the application to choose the tick interrupt source. */
void vApplicationSetupTimerInterrupt( void )
{
	/* Enable compare match timer 0. */
	MSTP( CMT0 ) = 0;

	/* Interrupt on compare match. */
	CMT0.CMCR.BIT.CMIE = 1;

	/* Set the compare match value. */
	CMT0.CMCOR = ( unsigned short ) ( ( ( configPERIPHERAL_CLOCK_HZ / configTICK_RATE_HZ ) -1 ) / 8 );

	/* Divide the PCLK by 8. */
	CMT0.CMCR.BIT.CKS = 0;

	/* Enable the interrupt... */
	_IEN( _CMT0_CMI0 ) = 1;

	/* ...and set its priority to the application defined kernel priority. */
	_IPR( _CMT0_CMI0 ) = configKERNEL_INTERRUPT_PRIORITY;

	/* Start the timer. */
	CMT.CMSTR0.BIT.STR0 = 1;
}
/*-----------------------------------------------------------*/

/* This function is explained by the comments above its prototype at the top
of this file. */
void vApplicationMallocFailedHook( void )
{
    lcd_string(0,0, "Malloc failed");
    for( ;; );
}
/*-----------------------------------------------------------*/

/* This function is explained by the comments above its prototype at the top
of this file. */
void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
    lcd_string(0,0, "Stack Overflow");
    for( ;; );
}
/*-----------------------------------------------------------*/

/* This function is explained by the comments above its prototype at the top
of this file. */
void vApplicationIdleHook( void )
{
}
/*-----------------------------------------------------------*/

