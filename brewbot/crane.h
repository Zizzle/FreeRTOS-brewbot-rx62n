///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, All Rights Reserved.
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

void craneMove(int direction);
void startCraneLimitSwitchTask();


#endif
