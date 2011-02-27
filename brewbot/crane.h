///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date:  6 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CRANE_H
#define CRANE_H

#include "brew_task.h"
#include "types.h"

enum Direction
{
    DIRECTION_UNKNOWN,
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};

void start_crane_task();
uint8_t crane_is_moving();
void crane_move(int direction, void (*taskErrorHandler)(brew_task_t *));
void crane_stop();

uint8_t crane_is_at_left();
uint8_t crane_is_at_right();
uint8_t crane_is_at_top();
uint8_t crane_is_at_bottom();

#endif
