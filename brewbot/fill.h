///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 15 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FILL_H
#define FILL_H

#include "brew_task.h"

void start_fill_task();
void fill_start(void (*taskErrorHandler)(brew_task_t *));
void fill_stop();
uint8_t fill_is_running();

#endif
