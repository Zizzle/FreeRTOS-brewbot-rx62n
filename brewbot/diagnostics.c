///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date:  9 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include "menu.h"
#include "heat.h"
#include "buttons.h"

static float heat_target = 66.0f;
static int   heat_duty   = 50;

static void diag_heat()
{
    setHeatTargetTemperature(heat_target);
    setHeatDutyCycle(heat_duty);
    startHeatTask();
}

static int diag_heat_key(unsigned char key)
{
    if (key & KEY_PRESSED)
    {
	if (key & KEY_UP)
	{
	    heat_target += 0.5;
	}
	else if (key & KEY_DOWN)
	{
	    heat_target -= 0.5;
	}
	else if (key & KEY_LEFT)
	{
	    if (heat_duty <= 10)
		heat_duty--;
	    else
		heat_duty -= 10;
	}
	else if (key & KEY_RIGHT)
	{
	    if (heat_duty < 10)
		heat_duty++;
	    else
		heat_duty += 10;
	}
	setHeatTargetTemperature(heat_target);
	setHeatDutyCycle(heat_duty);
    }
    return 1; // signal that we consume the left key, double click needed to exit
}

struct menu diag_menu[] =
 {
     {"Heat",           NULL,                diag_heat,  diag_heat_key},
#if 0
    {"Heat basic",      diag_heat_basic,     NULL,       diag_key},
    {"Level sensors",   NULL,                diag_adc,   diag_key},
    {"Temps",           NULL,                diag_temp,  diag_key},
    {"Solenoid",        NULL,               solenoid_pulse,solenoid_pulse_key},
    {"Hops",            NULL,                diag_hops,  NULL},
#endif
    {NULL, NULL, NULL, NULL}
};


