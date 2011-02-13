///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 13 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "lcd.h"
#include "buttons.h"
#include "menu.h"
#include "brew.h"

struct brew_step
{
    const char *name;
    int (*method)();
    int timeout;
};


static struct state
{
    char     in_alarm;
    uint8_t  step;
    uint32_t last_poll;
    uint8_t  graphx;
    uint8_t  target;
    uint8_t  hop_addition;
    uint8_t  last_temp;
    uint8_t  target_temp;
    uint8_t  expected_duration;
} g_state = { 0, 0, 0, 0, 0, 0, 0, 0};

#define BREW_STEPS_TOTAL 10
static struct brew_step g_steps[BREW_STEPS_TOTAL] = 
{
    {"Reset crane",    NULL,   0},
    {"Fill water",     NULL,   0},
    {"Heat water",     NULL,   0},
    {"Crane to mash",  NULL,   0},
    {"Mash",           NULL,   0},
    {"Crane out mash", NULL,   0},
    {"Bring to boil",  NULL,   0},
    {"Boil stabilize", NULL,   0},
    {"Boil & Hops",    NULL,   0},
    {"Finish",         NULL,   0},
};

static const char *brew_step_name(unsigned char step)
{
    if (step < BREW_STEPS_TOTAL)
        return g_steps[step].name;
    return "Unknown";
}

void brew_start(int init)
{

}

int brew_key_handler(unsigned char key)
{
    return 1;
}

void brew_resume(int init)
{
    g_state.in_alarm = 0;
    lcd_printf(0, 2, 19, "Resume at step:");
    lcd_printf(0, 6, 19, "(> to resume)");
    brew_resume_key(KEY_PRESSED);
}

int brew_resume_key(unsigned char key)
{
   if ((key & KEY_PRESSED) == 0)
        return 1;

    if (key & KEY_UP && g_state.step < BREW_STEPS_TOTAL - 1)
        g_state.step++;
    if (key & KEY_DOWN && g_state.step != 0)
        g_state.step--;

    if (key & KEY_RIGHT)
    {
        menu_run_applet(brew_key_handler);
//        run_step();
    }
    else
    {
        lcd_printf(16, 2, 19, "%d", g_state.step + 1);
        lcd_printf(0,  4, 19, " %s", brew_step_name(g_state.step));
    }
    return 1;
}
