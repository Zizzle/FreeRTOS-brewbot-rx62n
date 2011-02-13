///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date: 13 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"
#include "buttons.h"
#include "p5q.h"
#include "lcd.h"

#define SETTINGS_FLASH_ADDR 0
#define SETTINGS_DISPLAY_SIZE 7

struct settings g_settings;

#define FLOAT  1
#define UINT8  2

struct settings_display
{
    const char * fmt;
    int type;
    void  *value;
};

static struct settings_display settings_display_list[] = 
{
    {" Mash target %.1f C", FLOAT, &g_settings.mash_target_temp},
    {" Mash time   %d min", UINT8, &g_settings.mash_time},
    {" Mash duty   %d %%",  UINT8, &g_settings.mash_duty_cycle},
    {" Boil time   %d min", UINT8, &g_settings.boil_time},
    {" Boil duty   %d %%",  UINT8, &g_settings.boil_duty_cycle},
    {" Hop time 1  %d min", UINT8, &g_settings.hop_addition[0]},
    {" Hop time 2  %d min", UINT8, &g_settings.hop_addition[1]},
    {" Hop time 3  %d min", UINT8, &g_settings.hop_addition[2]},
    {NULL, 0, NULL}
};

#define SETTINGS_MENU_LEN 8

int settings_offset = 0;
int settings_cursor = 0;

void settings_load()
{
    flash_read(SETTINGS_FLASH_ADDR, (uint8_t *)&g_settings, sizeof(g_settings));

    if (g_settings.magic != SETTINGS_MAGIC)
    {
	g_settings.magic             = SETTINGS_MAGIC;
	g_settings.mash_time         = 60;
	g_settings.mash_target_temp  = 66.00f;
	g_settings.boil_time         = 60;
	g_settings.mash_duty_cycle   = 10;
	g_settings.boil_duty_cycle   = 50;
	g_settings.hop_addition[0]   = 60;
        g_settings.hop_addition[1]   = 40;
        g_settings.hop_addition[2]   = 0;
        g_settings.hop_addition[3]   = 0;
        g_settings.hop_addition[4]   = 0;
    }
}

void settings_save()
{
    flash_write(SETTINGS_FLASH_ADDR,  (uint8_t *)&g_settings, sizeof(g_settings));
}


static void settings_display_menu()
{
    int ii;
    for (ii = 0;
	 ii < SETTINGS_DISPLAY_SIZE && settings_display_list[ii + settings_offset].fmt != NULL;
	 ii++)
    {
	struct settings_display *disp = &settings_display_list[ii + settings_offset];
	if (disp->type == FLOAT)
	{
	    lcd_printf(0, 1 + ii, disp->fmt, *((float *)disp->value));
	}
	else if (disp->type == UINT8)
	{
	    lcd_printf(0, 1 + ii, disp->fmt, *((uint8_t *)disp->value));
	}
	if (ii + settings_offset == settings_cursor)
	{
	    lcd_printf(0, 1 + ii, ">");
	}
    }
}

void settings_display(int init)
{
    if (init)
    {
	lcd_printf(0, 0, "  Settings");
	settings_display_menu();
    }
    else
    {
	settings_save();
    }
}
int settings_key_handler(unsigned char key)
{

    if (upKeyPressed(key))
    {
	if (settings_offset != 0 && settings_offset == settings_cursor)
	    settings_offset--;
	if (settings_cursor > 0)
	    settings_cursor--;
    }
    else if (downKeyPressed(key))
    {
	if (settings_offset < SETTINGS_MENU_LEN - SETTINGS_DISPLAY_SIZE &&
	    settings_cursor == settings_offset + SETTINGS_DISPLAY_SIZE - 1)
	    settings_offset++;
	if (settings_cursor < SETTINGS_MENU_LEN)
	    settings_cursor++;
    }
    else if (leftKeyPressed(key))
    {
	struct settings_display *disp = &settings_display_list[settings_cursor];
	if (disp->type == FLOAT)
	{
	    *((float *)disp->value) -= 0.5;
	}
	else if (disp->type == UINT8)
	{
	    *((uint8_t *)disp->value) -= 1;
	}
    }
    else if (rightKeyPressed(key))
    {
	struct settings_display *disp = &settings_display_list[settings_cursor];
	if (disp->type == FLOAT)
	{
	    *((float *)disp->value) += 0.5;
	}
	else if (disp->type == UINT8)
	{
	    *((uint8_t *)disp->value) += 1;
	}
    }
    settings_display_menu();
    return 1;
}
