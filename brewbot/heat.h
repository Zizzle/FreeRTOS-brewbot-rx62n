///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date:  7 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HEAT_H
#define HEAT_H

#include "types.h"

void setHeatTargetTemperature(float target);
void setHeatDutyCycle(int duty_cycle);

float getHeatTargetTemperature();
int   getHeatDutyCycle();

void startHeatTask();
void stopHeatTask();

uint8_t isHeatTaskRunning();
uint8_t heat_has_reached_target();

#endif
