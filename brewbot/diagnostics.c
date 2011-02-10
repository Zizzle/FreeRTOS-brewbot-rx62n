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
#include "mash.h"
#include "buttons.h"

static float mash_target = 66.0f;
static int   mash_duty   = 50;

static void diag_mash()
{
    
}

static int diag_mash_key(unsigned char key)
{
    if (key & KEY_PRESSED)
    {
	if (key & KEY_UP)
	{
	    mash_target += 0.5;
	}
	else if (key & KEY_DOWN)
	{
	    mash_target -= 0.5;
	}
	else if (key & KEY_LEFT)
	{
	    if (mash_duty <= 10)
		mash_duty--;
	    else
		mash_duty -= 10;
	}
	else if (key & KEY_RIGHT)
	{
	    if (mash_duty < 10)
		mash_duty++;
	    else
		mash_duty += 10;
	}
	setMashTargetTemperature(mash_target);
	setMashDutyCycle(mash_duty);
    }
    return 1; // signal that we consume the left key, double click needed to exit
}

struct menu diag_menu[] =
 {
     {"Mash",           NULL,                diag_mash,  diag_mash_key},
#if 0
    {"Heat basic",      diag_heat_basic,     NULL,       diag_key},
    {"Level sensors",   NULL,                diag_adc,   diag_key},
    {"Temps",           NULL,                diag_temp,  diag_key},
    {"Solenoid",        NULL,               solenoid_pulse,solenoid_pulse_key},
    {"Hops",            NULL,                diag_hops,  NULL},
#endif
    {NULL, NULL, NULL, NULL}
};


