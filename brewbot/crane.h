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

enum Direction
{
    DIRECTION_UNKNOWN,
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};

void startCraneTask();
void craneMove(int direction);
void craneStop();

#endif
