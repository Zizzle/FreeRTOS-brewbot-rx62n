#include "FreeRTOS.h"
#include "iodefine.h"
#include "brewbot.h"

#define timerINTERRUPT_FREQUENCY		( 20000UL )
#define timerHIGHEST_PRIORITY			( 15 )

/* Counts the number of high frequency interrupts - used to generate the run
time stats. */
volatile unsigned long ulHighFrequencyTickCount = 0UL;

static int servoPwmCount = 0;

#define NUM_SERVOS 3
static int servoPos[NUM_SERVOS];

void servo_set_pos(short servo, short degrees)
{
    servoPos[servo] = 10 + (degrees * 40) / 180;
}

void vTimer2_ISR_Handler( void ) __attribute__((interrupt));
/*-----------------------------------------------------------*/

void vTimer2_ISR_Handler( void )
{
    /* Used to generate the run time stats. */
    ulHighFrequencyTickCount++;
    /* This is the highest priority interrupt in the system, so there is no
       advantage to re-enabling interrupts here.*/


    servoPwmCount++;

    if (servoPwmCount >= 400)
	servoPwmCount = 0;

    HOP_DROPPER_1 = (servoPos[0] > servoPwmCount);
    HOP_DROPPER_2 = (servoPos[1] > servoPwmCount);
    HOP_DROPPER_3 = (servoPos[2] > servoPwmCount);
}

/*-----------------------------------------------------------*/

void vSetupHighFrequencyTimer( void )
{
    HOP_DROPPER_1_DDR = 1;
    HOP_DROPPER_2_DDR = 1;
    HOP_DROPPER_3_DDR = 1;

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
