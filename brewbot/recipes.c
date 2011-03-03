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

#include "shell.h"

extern char reqParams[];

static FIL file;

static int len;

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
    char line[80];
    snprintf(line, sizeof(line), "%s/%s/recipe.txt", RECIPE_PATH, name);

    strcpy(rec->name, "Unknown");


    result = f_open(&file, line, FA_READ);
    if (result != FR_OK)
    {
	shell_printf("%s", line);
	return 0;	
    }

    while (f_gets (line, sizeof(line), &file) != NULL)
    {
	len += sprintf((char *) uip_appdata + len, "%s", line);
	parse_recipe_line(line, rec);
    }
    f_close(&file);
    return 1;
}

static void http_create_recipe()
{
    char *input  = reqParams;

    char *token;

    while ((token = strtok(input, "&")))
    {
	input = NULL;

	
    }
}

unsigned short httpd_generate_recipe_list( void *arg )
{
    unsigned short len = 0;
    DIR dir;
    FILINFO fno;
    recipe_t recipe;
    FRESULT result = f_opendir (&dir, RECIPE_PATH);

    if (reqParams[0] != 0)
    {
	http_create_recipe();
    }

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
	len += sprintf((char *) uip_appdata + len, "<li><a href=\"/html/recipe.sht?name=%s\">%s</a></li>\n", fno.fname, recipe.name);
    }	

//    len += sprintf((char *) uip_appdata + len, "params = %s", reqParams);
    
    return len;
}

unsigned short httpd_generate_recipe(void *arg)
{
    recipe_t recipe;

    char *name = strstr(reqParams, "name=");

    if (name == NULL)
    {
	return sprintf((char *) uip_appdata + len, "Please include recipe name in URL\n");	
    }

    len = 0;
    len += sprintf((char *) uip_appdata + len, "<pre>\n");
    if (!recipe_load(name +5, &recipe))
    {
	return sprintf((char *) uip_appdata + len, "Recipe not found\n");	
    }

    //len += sprintf((char *) uip_appdata + len, "<h1>%s</h1>\n", recipe.name);

    len += sprintf((char *) uip_appdata + len, "</pre><p><a href=\"/html/start.sht?%s\">Brew this now</a>\n", name);
    return len;
}
