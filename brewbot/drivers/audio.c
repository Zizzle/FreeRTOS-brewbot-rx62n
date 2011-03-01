///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3+.
//
// Authors: Matthew Pratt
//
// Date: 28 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iodefine.h"

static volatile int beep_duration;

void audio_beep(int freq, int duration)
{
    beep_duration = duration * (float)(freq / 1000.0);

    MSTP(MTU8) = 0;           // enable MTU8
    MTU8.TCR.BIT.TPSC = 0x03; // prescale of 64 gives a 750 kHz clock - page 854
    MTU8.TCR.BIT.CCLR = 0x02; // clear counter on TGRB match - page 855
    MTU8.TIOR.BIT.IOA = 0x02; // MTIOCA8 pin initial low, high on match - page 873
    MTU8.TMDR.BIT.MD  = 0x03; // PWM mode 2 - page 859
    MTU8.TIER.BIT.TGIEB = 1;  // interrupt on TGRB (every period)
    IPR(MTU8,TGIB8) = 1;      // interrupt priority to lowest level
    IEN(MTU8,TGIB8) = 1;
    MTU8.TGRB = (750000 / freq);
    MTU8.TGRA = MTU8.TGRB >> 2;
    MTUB.TSTR.BIT.CST2 = 1;
}

// keep track of how long the beep has been running
void INT_Excep_TPU8_TCI8V(void) __attribute__ ((interrupt));
void INT_Excep_TPU8_TCI8V(void)
{
    if (beep_duration-- <= 0)
    {
        MTU8.TGRA = 0;    
        MTUB.TSTR.BIT.CST2 = 0;
    }
}
