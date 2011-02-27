///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3+.
//
// Authors: Matthew Pratt
//
// Date: 26 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fatfs/ff.h"
#include "recipes.h"
#include "net/uip.h"

static FIL file;

static void parse_recipe_line(char *line, recipe_t *rec)
{
    if (strncmp(line, "name=", 5) == 0)
	strncpy(rec->name, line + 5, sizeof(rec->name));
    else if (strncmp(line, "date=", 5) == 0)
	strncpy(rec->date, line + 5, sizeof(rec->date));
}

int recipe_load(const char *name, recipe_t *rec)
{
    FRESULT result;
    char line[50];
    snprintf(line, sizeof(line), "%s/%s/recipe.txt", RECIPE_PATH, name);

    result = f_open(&file, line, FA_READ);
    if (result != FR_OK)
    {
	return 0;	
    }

    while (f_gets (line, sizeof(line), &file) != NULL)
    {
	parse_recipe_line(line, rec);
    }
    return 1;
}

unsigned short httpd_generate_recipe_list( void *arg )
{
    unsigned short len = 0;

#if 0

    DIR dir;
    FILINFO fno;
    recipe_t recipe;
    FRESULT result = f_opendir (&dir, RECIPE_PATH);

    if (result != FR_OK)
    {
	return sprintf((char *) uip_appdata + len, "Error opening brew dir %d\n", result);	
    }

    for (;;) {
	result = f_readdir(&dir, &fno);
	if (result != FR_OK || fno.fname[0] == 0) break;
	if (fno.fname[0] == '.') continue;
	if ( ! (fno.fattrib & AM_DIR)) continue; // only care about directories

	recipe_load(fno.fname, &recipe);
	len += sprintf((char *) uip_appdata + len, "<li><a href=\"\">%s</a></li>\n", recipe.name);
    }	
#else
	len += sprintf((char *) uip_appdata + len, "<li><a href=\"\">%s</a></li>\n", "blah");
#endif

    return len;
}
