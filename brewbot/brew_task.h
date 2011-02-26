///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date: 16 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BREW_TASK_H
#define BREW_TASK_H

#include "queue.h"

typedef struct brew_task brew_task_t;

struct brew_task
{
    const char *name;
    volatile char running;
    xQueueHandle  startStopQueue;
    portTickType  startTick;
    const char *  error;
    portTickType  maxRunTime;

    void (*taskStart)(brew_task_t *);
    void (*taskIteration)(brew_task_t *);
    void (*taskStop)(brew_task_t *);
    void (*taskErrorHandler)(brew_task_t*);
};

void startBrewTask(brew_task_t *task,
		   const char *name, int stackSize,
		   int priority, portTickType maxRunTime,
		   void (*taskStartCallback)(brew_task_t *),
		   void (*taskIterationCallback)(brew_task_t *),
		   void (*taskStopCallback)(brew_task_t *));
portBASE_TYPE brewTaskStart(brew_task_t *task, void (*taskErrorHandler)(brew_task_t *));
portBASE_TYPE brewTaskStop (brew_task_t *task);

#endif
