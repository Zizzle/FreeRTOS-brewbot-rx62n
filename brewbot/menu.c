///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt
//
// Licensed under the GNU General Public License v3 or greater
//
// Date: 21 Jun 2007
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "buttons.h"
#include "menu.h"
#include "FreeRTOS.h"
#include "task.h"

#define HEIGHT 7
#define HILIGHT_W 19
#define HILIGHT_Y(row) (row)

static struct menu  *g_menu[MAX_DEPTH];
static unsigned char g_crumbs[MAX_DEPTH];
static unsigned char g_item = 0;
static unsigned char g_index = 0;
static int (*g_menu_applet)(unsigned char key) = NULL;

static void menu_update_hilight(void)
{
#if 1
    unsigned char ii;
    for (ii = 0; ii < HEIGHT && g_menu[g_index][ii].text; ii++)
    {
        lcd_text(0,  ii + 1, " ");
	lcd_text(18, ii + 1, " ");
    }
    lcd_text(0,  g_item + 1, ">");
    lcd_text(18, g_item + 1, "<");
#else
    static short old = 0; // keep track of the old hilight so we can clear it

    lcd_clear_chars(0, old, HILIGHT_W);
    lcd_set_pixels (0, g_item + 1, HILIGHT_W);
    old = g_item + 1;
#endif
}

static void menu_run_callback(void)
{
    if (g_index > 0)
    {
        void (*callback)(int) = g_menu[g_index - 1][g_crumbs[g_index - 1]].activate;
        if (callback)
            callback(1);
    }
}

static void menu_update(void)
{
    unsigned char ii;
    menu_clear();


    for (ii = 0; ii < HEIGHT && g_menu[g_index][ii].text; ii++)
    {
        lcd_text(1, ii + 1, g_menu[g_index][ii].text);
    }
    menu_update_hilight();


    char crumbs[40] = "Brewbot";
/*
    for (ii = 1; ii <= g_index; ii++)
    {
        strcat(crumbs, ":");
        strcat(crumbs, g_menu[ii-1][g_crumbs[ii-1]].text);
    }
*/
    lcd_text(0,0, crumbs);

    
}

void menu_set_root(struct menu *root_menu)
{
    g_menu[0] = root_menu;
    menu_update();
}

//
// Go back in the menu hierachy after the left key has been pressed
// in an applet (or double clicked)
//
static void menu_back_after_applet()
{
    void (*callback)(int) = g_menu[g_index][g_item].activate;
    if (callback)
	callback(0); // deactivate

    menu_clear();
    menu_update();
    g_menu_applet = NULL;
    menu_run_callback();
}

//
// When the user has select an applet all the keys come here.
// We offer the key to the applet. If the back key is pressed then
// we disable keys coming here.
//
static void menu_applet_key(unsigned char key)
{
    static portTickType lastLeft = 0;

    if (leftKeyPressed(key) && xTaskGetTickCount() - lastLeft < 500) // double click?)
    {
	menu_back_after_applet();
	return; // don't let the applet know about this key
    }

    // offer the key to the active applet
    int consumed = g_menu_applet(key);

    // if not consumed then got back to the menu
    if (leftKeyPressed(key))
    {
	if (consumed == 0)	    
	{
	    menu_back_after_applet();
	}
	else
	{
	    lastLeft = xTaskGetTickCount();
	}
    }
}

void menu_key(unsigned char key)
{
    if (g_menu_applet) {
        menu_applet_key(key);
        return;
    }

    if ((key & KEY_PRESSED) == 0)
        return;

    if (key & KEY_UP)
    {
        if (g_item > 0)
            g_item--;
        menu_update_hilight();
    }
    else if (key & KEY_DOWN)
    {
        if (g_menu[g_index][g_item+1].text != NULL)
            g_item++;
        menu_update_hilight();
    }
    else if (key & KEY_LEFT)
    {
        if (g_index > 0)
            g_index--;
        g_item = 0;
        menu_update();
        menu_run_callback();
    }
    else if (key & KEY_RIGHT)
    {
        void (*callback)(int) = g_menu[g_index][g_item].activate;
        g_crumbs[g_index] = g_item;

        if (g_menu[g_index][g_item].next && g_index < MAX_DEPTH)
        {
            g_index++;
            g_menu[g_index] = g_menu[g_index-1][g_item].next;
            g_item = 0;
            menu_update();
        }
        else if (g_menu[g_index][g_item].key_handler)
        {
            g_menu_applet = g_menu[g_index][g_item].key_handler;
            menu_clear();
        }

        // run the callback which should start the applet or update the display
        if (callback)
        {
            callback(1);
        }
    }
}

void menu_clear(void)
{
    lcd_clear();
//    lcd_clear_pixels(0, HILIGHT_Y(g_item + 1), HILIGHT_W, HILIGHT_H);
}

void menu_run_applet(int (*applet_key_handler)(unsigned char))
{
    g_menu_applet = applet_key_handler;
    menu_clear();
}
