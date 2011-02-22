///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3+.
//
// Authors: Matthew Pratt
//
// Date: 16 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "iodefine.h"

//
// Ok so the level probes sit in water in the pot.
// Effective resistance of the water seems to be about 30k. So I'm
// using a 100k pullup.
//
// But I noticed an interesting effect. Over a few seconds the resistance
// goes up to much higher. And it seems affected by the voltage.
//
// Anyway, we can use a threshold and the ADC to good affect.
//
// Here the ADC runs in continuous scan mode on the level probes and the
// application samples it occasionally.
//

#define THRESHOLD 3000

void level_probe_init()
{
    MSTP_S12AD               = 0;   // Enable 12-bit ADC 
    S12AD.ADCSR.BIT.ADST     = 0;   // Stop ADC
    S12AD.ADCSR.BIT.ADCS     = 1;   // Continuous-scan mode

    // which ports?
    S12AD.ADANS.BIT.ANS      = (1 << 2) | (1 << 3);  // AN3 & 4
    PORT4.ICR.BIT.B2         = 1;   // input routed to peripheral
    PORT4.ICR.BIT.B3         = 1;   // input routed to peripheral

    S12AD.ADCSR.BIT.ADST     = 1;   // start the ADC

}

uint16_t level_probe_heat_adc()
{
    return S12AD.ADDRC;
}

uint16_t level_probe_full_adc()
{
    return S12AD.ADDRD;
}

uint8_t level_hit_heat()
{
    return level_probe_heat_adc() < THRESHOLD;
}

uint8_t level_hit_full()
{
    return level_probe_full_adc() < THRESHOLD;
}
