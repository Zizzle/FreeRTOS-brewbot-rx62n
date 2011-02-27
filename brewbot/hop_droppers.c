#include "FreeRTOS.h"
#include "iodefine.h"
#include "brewbot.h"
#include "brew_task.h"
#include "task.h"
#include "lcd.h"

#define timerINTERRUPT_FREQUENCY		( 20000UL )
#define timerHIGHEST_PRIORITY			( 15 )

/* Counts the number of high frequency interrupts - used to generate the run
time stats. */
volatile unsigned long ulHighFrequencyTickCount = 0UL;
static int servoPwmCount = 0;
static struct brew_task hops_task;
#define NUM_SERVOS 3

static int servoPwm;
static int servo;
static int pos;
static char dirDown;

#define MAX_POS 160
#define MIN_POS 5

/*-----------------------------------------------------------*/
void servo_set_pos(short degrees)
{
    servoPwm = 10 + (degrees * 40) / 180;
}
/*-----------------------------------------------------------*/

static void _hops_start(struct brew_task *bt)
{
    dirDown = 1;
    pos = MAX_POS;
    servo_set_pos(MAX_POS);

    if (servo == 0) HOP_DROPPER_1_DDR = 1; else HOP_DROPPER_1_DDR = 0;
    if (servo == 1) HOP_DROPPER_2_DDR = 1; else HOP_DROPPER_2_DDR = 0;
    if (servo == 2) HOP_DROPPER_3_DDR = 1; else HOP_DROPPER_3_DDR = 0;
}
/*-----------------------------------------------------------*/
static void _hops_stop(struct brew_task *bt)
{
    HOP_DROPPER_1_DDR = 0;
    HOP_DROPPER_2_DDR = 0;
    HOP_DROPPER_3_DDR = 0;
}

static void hops_iteration(struct brew_task *bt)
{
    if (dirDown)
    {
	servo_set_pos(pos--);
	if (pos <= MIN_POS)
	    dirDown = 0;
    }
    else
    {
	servo_set_pos(pos++);

	if (pos >= MAX_POS)
	    bt->running = 0;
    }

    vTaskDelay(10);
}

void hops_start_task()
{
    startBrewTask(&hops_task,
		  "Hops", 200, 2, 10000,
		  _hops_start,
		  hops_iteration,
		  _hops_stop);
}

char hops_are_dropping()
{
    return hops_task.running;
}

void hops_drop(short dropper, void (*taskErrorHandler)(brew_task_t *))
{
    if (servo >= NUM_SERVOS)
	return;

    while (hops_are_dropping())
	vTaskDelay(100); // wait for the current drop to finish to happen

    servo = dropper;

    brewTaskStart(&hops_task, taskErrorHandler);
}

void hops_stop()
{
    brewTaskStop(&hops_task);
}


/*-----------------------------------------------------------*/
void vTimer2_ISR_Handler( void ) __attribute__((interrupt));

void vTimer2_ISR_Handler( void )
{
    /* Used to generate the run time stats. */
    ulHighFrequencyTickCount++;
    /* This is the highest priority interrupt in the system, so there is no
       advantage to re-enabling interrupts here.*/

    servoPwmCount++;

    if (servoPwmCount >= 400)
	servoPwmCount = 0;

    if (hops_task.running && servo == 0)
	HOP_DROPPER_1_DDR = 1; else HOP_DROPPER_1_DDR = 0;
    if (hops_task.running && servo == 1)
	HOP_DROPPER_2_DDR = 1; else HOP_DROPPER_2_DDR = 0;
    if (hops_task.running && servo == 2)
	HOP_DROPPER_3_DDR = 1; else HOP_DROPPER_3_DDR = 0;

    HOP_DROPPER_1 = (servoPwm > servoPwmCount);
    HOP_DROPPER_2 = (servoPwm > servoPwmCount);
    HOP_DROPPER_3 = (servoPwm > servoPwmCount);
}

/*-----------------------------------------------------------*/

void vSetupHighFrequencyTimer( void )
{
	MSTP( CMT2 ) = 0;
	
	/* Interrupt on compare match. */
	CMT2.CMCR.BIT.CMIE = 1;
	
	/* Set the compare match value. */
	CMT2.CMCOR = ( unsigned short ) ( ( ( configPERIPHERAL_CLOCK_HZ / timerINTERRUPT_FREQUENCY ) -1 ) / 8 );
	
	/* Divide the PCLK by 8. */
	CMT2.CMCR.BIT.CKS = 0;
	
	/* Enable the interrupt... */
	_IEN( _CMT2_CMI2 ) = 1;
	
	_IPR( _CMT2_CMI2 ) = timerHIGHEST_PRIORITY;
	
	/* Start the timers. */
	CMT.CMSTR1.BIT.STR2 = 1;
}
