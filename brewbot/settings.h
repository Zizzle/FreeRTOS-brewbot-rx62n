///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date: 13 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SETTINGS_H
#define SETTINGS_H

#include "types.h"

#define MAX_HOP_ADDITIONS 5

#define SETTINGS_MAGIC 0xb00b

struct settings
{
    uint16_t magic;
    uint8_t  mash_time;
    float    mash_target_temp;
    uint8_t  boil_time;
    uint8_t  mash_duty_cycle;
    uint8_t  boil_duty_cycle;
    uint8_t  hop_addition[MAX_HOP_ADDITIONS];
};

void settings_load();
void settings_save();
void settings_display(int init);
int settings_key_handler(unsigned char key);


extern struct settings g_settings;

#endif
