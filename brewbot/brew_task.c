///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 16 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "semphr.h"
#include "lcd.h"
#include "brew_task.h"

#define START 2
#define STOP  3
typedef unsigned char start_stop_t;

static void brewTask( void *pvParameters )
{
    brew_task_t *bt = pvParameters;

    for (;;)
    {
	// wait for someone to send us a start message
	start_stop_t ss = 0;

	if (xQueueReceive( bt->startStopQueue, &ss, portMAX_DELAY) != pdTRUE || ss != START)
	    continue;

	bt->error     = NULL; // clear error from last task
	bt->running   = 1;
	bt->startTick = xTaskGetTickCount(); // keep track of when we started
	if (bt->taskStart)
	    bt->taskStart(bt);

	while (bt->running && bt->error == NULL)
	{
	    bt->taskIteration(bt);

	    // check to see if we have received a stop message
	    if (pdTRUE == xQueueReceive(bt->startStopQueue, &ss, 1) && ss == STOP) // wait 1 tick
		break;

	    // check to see if the job has run too long
	    if (xTaskGetTickCount() - bt->startTick > bt->maxRunTime)
	    {
		bt->error = "Timeout filling";
		break;
	    }
	}

	if (bt->taskErrorHandler && bt->error)
	    bt->taskErrorHandler(bt);

	if (bt->taskStop)
	    bt->taskStop(bt);

	bt->running = 0;
    }
}

void startBrewTask(brew_task_t *task,
		   const char *name, int stackSize, int priority, int maxRunTime,
		   void (*taskStartCallback)(brew_task_t *),
		   void (*taskIterationCallback)(brew_task_t *),
		   void (*taskStopCallback)(brew_task_t *))
{
    task->name           = name;
    task->startStopQueue = xQueueCreate( 2, sizeof(start_stop_t));
    task->taskStart      = taskStartCallback;
    task->taskIteration  = taskIterationCallback;
    task->taskStop       = taskStopCallback;

    xTaskCreate( brewTask,
		 (const signed char *) name,
		 configMINIMAL_STACK_SIZE + stackSize,
		 task,
		 priority,
		 NULL );
}


portBASE_TYPE brewTaskStart(brew_task_t *task, void (*taskErrorHandler)(brew_task_t *))
{
    start_stop_t ss = START;
    return xQueueSend( task->startStopQueue, &ss, 1);
}

portBASE_TYPE brewTaskStop(brew_task_t *task)
{
    start_stop_t ss = STOP;
    return xQueueSend( task->startStopQueue, &ss, 1);
}
