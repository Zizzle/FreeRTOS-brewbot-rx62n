///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 26 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RECIPES_H
#define RECIPES_H

#define RECIPE_PATH "/recipes"

typedef struct recipie
{
    char name[40];
    char date[20];
    
} recipe_t;

int recipie_load(const char *name, recipe_t *rec);

unsigned short httpd_generate_recipe_list( void *arg );
unsigned short httpd_generate_recipe(void *arg);


#endif
