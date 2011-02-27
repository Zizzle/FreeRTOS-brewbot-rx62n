///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 16 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LEVEL_PROBES_H
#define LEVEL_PROBES_H

#include "types.h"

void level_probe_init();
uint16_t level_probe_heat_adc();
uint16_t level_probe_full_adc();
int8_t  level_hit_heat();
int8_t  level_hit_full();
void level_wait_for_steady_readings();

#endif
