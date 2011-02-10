///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date:  7 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HEAT_H
#define HEAT_H

void setHeatTargetTemperature(float target);
void setHeatDutyCycle(int duty_cycle);

float getHeatTargetTemperature();
int   getHeatDutyCycle();

void startHeatTask();
void stopHeatTask();

#endif
