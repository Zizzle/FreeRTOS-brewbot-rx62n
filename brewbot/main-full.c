/* Hardware specific includes. */
#include "iodefine.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard demo includes. */
#include "partest.h"
#include "flash.h"
#include "IntQueue.h"
#include "BlockQ.h"
#include "death.h"
#include "integer.h"
#include "blocktim.h"
#include "semtest.h"
#include "PollQ.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "recmutex.h"
#include "flop.h"

#include "lcd.h"

/* Values that are passed into the reg test tasks using the task parameter.  The
tasks check that the values are passed in correctly. */
#define mainREG_TEST_1_PARAMETER	( 0x12121212UL )
#define mainREG_TEST_2_PARAMETER	( 0x12345678UL )

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

/* The rate at which mainCHECK_LED will toggle when all the tasks are running
without error.  Controlled by the check task as described at the top of this
file. */
#define mainNO_ERROR_CYCLE_TIME		( 5000 / portTICK_RATE_MS )

/* The rate at which mainCHECK_LED will toggle when an error has been reported
by at least one task.  Controlled by the check task as described at the top of
this file. */
#define mainERROR_CYCLE_TIME		( 200 / portTICK_RATE_MS )


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
 * The reg test tasks as described at the top of this file.
 */
static void prvRegTest1Task( void *pvParameters );
static void prvRegTest2Task( void *pvParameters );

/*
 * The actual implementation of the reg test functionality, which, because of
 * the direct register access, have to be in assembly.
 */
static void prvRegTest1Implementation( void ) __attribute__((naked));
static void prvRegTest2Implementation( void ) __attribute__((naked));


/*
 * The check task as described at the top of this file.
 */
static void prvCheckTask( void *pvParameters );

/*
 * Contains the implementation of the WEB server.
 */
extern void vuIP_Task( void *pvParameters );

/*-----------------------------------------------------------*/

/* Variables that are incremented on each iteration of the reg test tasks -
provided the tasks have not reported any errors.  The check task inspects these
variables to ensure they are still incrementing as expected.  If a variable
stops incrementing then it is likely that its associate task has stalled. */
unsigned long ulRegTest1CycleCount = 0UL, ulRegTest2CycleCount = 0UL;

/* The status message that is displayed at the bottom of the "task stats" web
page, which is served by the uIP task.  This will report any errors picked up
by the reg test task. */
static const char *pcStatusMessage = NULL;

/*-----------------------------------------------------------*/

int main(void)
{
extern void HardwareSetup( void );

	/* Renesas provided CPU configuration routine.  The clocks are configured in
	here. */
	HardwareSetup();

	/* Turn all LEDs off. */
	vParTestInitialise();

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

	/* Start the reg test tasks which test the context switching mechanism. */
	xTaskCreate( prvRegTest1Task, ( signed char * ) "RegTst1", configMINIMAL_STACK_SIZE, ( void * ) mainREG_TEST_1_PARAMETER, tskIDLE_PRIORITY, NULL );
	xTaskCreate( prvRegTest2Task, ( signed char * ) "RegTst2", configMINIMAL_STACK_SIZE, ( void * ) mainREG_TEST_2_PARAMETER, tskIDLE_PRIORITY, NULL );

	/* The web server task. */
	xTaskCreate( vuIP_Task, ( signed char * ) "uIP", mainuIP_STACK_SIZE, NULL, mainuIP_TASK_PRIORITY, NULL );

	/* Start the check task as described at the top of this file. */
	xTaskCreate( prvCheckTask, ( signed char * ) "Check", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );

	/* Create the standard demo tasks. */
	vStartBlockingQueueTasks( mainBLOCK_Q_PRIORITY );
	vCreateBlockTimeTasks();
	vStartSemaphoreTasks( mainSEM_TEST_PRIORITY );
	vStartPolledQueueTasks( mainQUEUE_POLL_PRIORITY );
	vStartIntegerMathTasks( mainINTEGER_TASK_PRIORITY );
	vStartGenericQueueTasks( mainGEN_QUEUE_TASK_PRIORITY );
	vStartLEDFlashTasks( mainFLASH_TASK_PRIORITY );
	vStartQueuePeekTasks();
	vStartRecursiveMutexTasks();
	vStartInterruptQueueTasks();
	vStartMathTasks( mainFLOP_TASK_PRIORITY );

	/* The suicide tasks must be created last as they need to know how many
	tasks were running prior to their creation in order to ascertain whether
	or not the correct/expected number of tasks are running at any given time. */
	vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );

	startDS1820Task();

	/* Start the tasks running. */
	vTaskStartScheduler();

	/* If all is well we will never reach here as the scheduler will now be
	running.  If we do reach here then it is likely that there was insufficient
	heap available for the idle task to be created. */
	for( ;; );
	
	return 0;
}
/*-----------------------------------------------------------*/

static void prvCheckTask( void *pvParameters )
{
static volatile unsigned long ulLastRegTest1CycleCount = 0UL, ulLastRegTest2CycleCount = 0UL;
portTickType xNextWakeTime, xCycleFrequency = mainNO_ERROR_CYCLE_TIME;
extern void vSetupHighFrequencyTimer( void );

	/* If this is being executed then the kernel has been started.  Start the high
	frequency timer test as described at the top of this file.  This is only
	included in the optimised build configuration - otherwise it takes up too much
	CPU time. */
	#ifdef INCLUDE_HIGH_FREQUENCY_TIMER_TEST
		vSetupHighFrequencyTimer();
	#endif

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again. */
		vTaskDelayUntil( &xNextWakeTime, xCycleFrequency );

		/* Check the standard demo tasks are running without error. */
		if( xAreGenericQueueTasksStillRunning() != pdTRUE )
		{			
			pcStatusMessage = "Error: GenQueue";
		}
		else if( xAreQueuePeekTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: QueuePeek\r\n";
		}
		else if( xAreBlockingQueuesStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: BlockQueue\r\n";
		}
		else if( xAreBlockTimeTestTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: BlockTime\r\n";
		}
	    else if( xAreSemaphoreTasksStillRunning() != pdTRUE )
	    {
			pcStatusMessage = "Error: SemTest\r\n";
	    }
	    else if( xArePollingQueuesStillRunning() != pdTRUE )
	    {
			pcStatusMessage = "Error: PollQueue\r\n";
	    }
	    else if( xIsCreateTaskStillRunning() != pdTRUE )
	    {
			pcStatusMessage = "Error: Death\r\n";
	    }
	    else if( xAreIntegerMathsTaskStillRunning() != pdTRUE )
	    {
			pcStatusMessage = "Error: IntMath\r\n";
	    }
	    else if( xAreRecursiveMutexTasksStillRunning() != pdTRUE )
	    {
			pcStatusMessage = "Error: RecMutex\r\n";
	    }
		else if( xAreIntQueueTasksStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: IntQueue\r\n";
		}
		else if( xAreMathsTaskStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: Flop\r\n";
		}

		/* Check the reg test tasks are still cycling.  They will stop incrementing
		their loop counters if they encounter an error. */
		if( ulRegTest1CycleCount == ulLastRegTest1CycleCount )
		{
			pcStatusMessage = "Error: RegTest1\r\n";
		}

		if( ulRegTest2CycleCount == ulLastRegTest2CycleCount )
		{
			pcStatusMessage = "Error: RegTest2\r\n";
		}

		ulLastRegTest1CycleCount = ulRegTest1CycleCount;
		ulLastRegTest2CycleCount = ulRegTest2CycleCount;

		/* Toggle the check LED to give an indication of the system status.  If
		the LED toggles every 5 seconds then everything is ok.  A faster toggle
		indicates an error. */
		vParTestToggleLED( mainCHECK_LED );

		/* Ensure the LED toggles at a faster rate if an error has occurred. */
		if( pcStatusMessage != NULL )
		{
			/* Increase the rate at which this task cycles, which will increase the
			rate at which mainCHECK_LED flashes to give visual feedback that an error
			has occurred. */
			xCycleFrequency = mainERROR_CYCLE_TIME;
			lcd_string(3,0, pcStatusMessage);

		}
		else
		{
		    lcd_string(3,0, "ALL OK");
		}
	}
}
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
	for( ;; );
}
/*-----------------------------------------------------------*/

/* This function is explained by the comments above its prototype at the top
of this file. */
void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
	for( ;; );
}
/*-----------------------------------------------------------*/

/* This function is explained by the comments above its prototype at the top
of this file. */
void vApplicationIdleHook( void )
{
}
/*-----------------------------------------------------------*/

/* This function is explained in the comments at the top of this file. */
static void prvRegTest1Task( void *pvParameters )
{
	if( ( ( unsigned long ) pvParameters ) != mainREG_TEST_1_PARAMETER )
	{
		/* The parameter did not contain the expected value. */
		for( ;; )
		{
			/* Stop the tick interrupt so its obvious something has gone wrong. */
			taskDISABLE_INTERRUPTS();
		}
	}

	/* This is an asm function that never returns. */
	prvRegTest1Implementation();
}
/*-----------------------------------------------------------*/

/* This function is explained in the comments at the top of this file. */
static void prvRegTest2Task( void *pvParameters )
{
	if( ( ( unsigned long ) pvParameters ) != mainREG_TEST_2_PARAMETER )
	{
		/* The parameter did not contain the expected value. */
		for( ;; )
		{
			/* Stop the tick interrupt so its obvious something has gone wrong. */
			taskDISABLE_INTERRUPTS();
		}
	}

	/* This is an asm function that never returns. */
	prvRegTest2Implementation();
}
/*-----------------------------------------------------------*/

char *pcGetTaskStatusMessage( void )
{
	/* Not bothered about a critical section here although technically because of
	the task priorities the pointer could change it will be atomic if not near
	atomic and its not critical. */
	if( pcStatusMessage == NULL )
	{
		return "All tasks running without error";
	}
	else
	{
		return ( char * ) pcStatusMessage;
	}
}
/*-----------------------------------------------------------*/

/* This function is explained in the comments at the top of this file. */
static void prvRegTest1Implementation( void )
{
	__asm volatile
	(
			/* Put a known value in each register. */
			"MOV	#1, R1						\n" \
			"MOV	#2, R2						\n" \
			"MOV	#3, R3						\n" \
			"MOV	#4, R4						\n" \
			"MOV	#5, R5						\n" \
			"MOV	#6, R6						\n" \
			"MOV	#7, R7						\n" \
			"MOV	#8, R8						\n" \
			"MOV	#9, R9						\n" \
			"MOV	#10, R10					\n" \
			"MOV	#11, R11					\n" \
			"MOV	#12, R12					\n" \
			"MOV	#13, R13					\n" \
			"MOV	#14, R14					\n" \
			"MOV	#15, R15					\n" \
			
			/* Loop, checking each itteration that each register still contains the
			expected value. */
		"TestLoop1:								\n" \

			/* Push the registers that are going to get clobbered. */
			"PUSHM	R14-R15						\n" \
			
			/* Increment the loop counter to show this task is still getting CPU time. */
			"MOV	#_ulRegTest1CycleCount, R14	\n" \
			"MOV	[ R14 ], R15				\n" \
			"ADD	#1, R15						\n" \
			"MOV	R15, [ R14 ]				\n" \
			
			/* Yield to extend the test coverage.  Set the bit in the ITU SWINTR register. */
			"MOV	#1, R14						\n" \
			"MOV 	#0872E0H, R15				\n" \
			"MOV.B	R14, [R15]					\n" \
			"NOP								\n" \
			"NOP								\n" \
			
			/* Restore the clobbered registers. */
			"POPM	R14-R15						\n" \
			
			/* Now compare each register to ensure it still contains the value that was
			set before this loop was entered. */
			"CMP	#1, R1						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#2, R2						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#3, R3						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#4, R4						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#5, R5						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#6, R6						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#7, R7						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#8, R8						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#9, R9						\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#10, R10					\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#11, R11					\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#12, R12					\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#13, R13					\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#14, R14					\n" \
			"BNE	RegTest1Error				\n" \
			"CMP	#15, R15					\n" \
			"BNE	RegTest1Error				\n" \

			/* All comparisons passed, start a new itteratio of this loop. */
			"BRA		TestLoop1				\n" \
			
		"RegTest1Error:							\n" \
			/* A compare failed, just loop here so the loop counter stops incrementing
			- causing the check task to indicate the error. */
			"BRA RegTest1Error					  "
	);
}
/*-----------------------------------------------------------*/

/* This function is explained in the comments at the top of this file. */
static void prvRegTest2Implementation( void )
{
	__asm volatile
	(
			/* Put a known value in each register. */
			"MOV	#10H, R1					\n" \
			"MOV	#20H, R2					\n" \
			"MOV	#30H, R3					\n" \
			"MOV	#40H, R4					\n" \
			"MOV	#50H, R5					\n" \
			"MOV	#60H, R6					\n" \
			"MOV	#70H, R7					\n" \
			"MOV	#80H, R8					\n" \
			"MOV	#90H, R9					\n" \
			"MOV	#100H, R10					\n" \
			"MOV	#110H, R11					\n" \
			"MOV	#120H, R12					\n" \
			"MOV	#130H, R13					\n" \
			"MOV	#140H, R14					\n" \
			"MOV	#150H, R15					\n" \
			
			/* Loop, checking each itteration that each register still contains the
			expected value. */
		"TestLoop2:								\n" \

			/* Push the registers that are going to get clobbered. */
			"PUSHM	R14-R15						\n" \
			
			/* Increment the loop counter to show this task is still getting CPU time. */
			"MOV	#_ulRegTest2CycleCount, R14	\n" \
			"MOV	[ R14 ], R15				\n" \
			"ADD	#1, R15						\n" \
			"MOV	R15, [ R14 ]				\n" \
			
			/* Restore the clobbered registers. */
			"POPM	R14-R15						\n" \
			
			/* Now compare each register to ensure it still contains the value that was
			set before this loop was entered. */
			"CMP	#10H, R1					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#20H, R2					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#30H, R3					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#40H, R4					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#50H, R5					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#60H, R6					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#70H, R7					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#80H, R8					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#90H, R9					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#100H, R10					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#110H, R11					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#120H, R12					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#130H, R13					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#140H, R14					\n" \
			"BNE	RegTest2Error				\n" \
			"CMP	#150H, R15					\n" \
			"BNE	RegTest2Error				\n" \

			/* All comparisons passed, start a new itteratio of this loop. */
			"BRA	TestLoop2					\n" \
			
		"RegTest2Error:							\n" \
			/* A compare failed, just loop here so the loop counter stops incrementing
			- causing the check task to indicate the error. */
			"BRA RegTest2Error					  "
	);
}

