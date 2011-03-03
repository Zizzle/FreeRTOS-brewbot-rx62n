///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date:  2 Mar 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "task.h"
#include "brewbot.h"
#include "semphr.h"
#include "lcd.h"
#include "brew_task.h"
#include "fatfs/ff.h"

static brew_task_t log_task;


static void log_start(brew_task_t *bt)
{
}

static void log_iteration(brew_task_t *bt)
{
    vTaskDelay(10);
}

static void log_stop_(brew_task_t *bt)
{

}

void start_log_task()
{
    startBrewTask(&log_task,
		  "Log", 500, 5, 1000000,
		  log_start,
		  log_iteration,
		  log_stop_);
}

void log_stop()
{
    brewTaskStop(&log_task);    
}

int log_find_max_number(char *path)
{
    DIR dir;
    FILINFO fno;
    FRESULT result = f_opendir (&dir, path);
    if (result != FR_OK)
    {
	return -1;
    }

    int max = 0;

    for (;;) {
	result = f_readdir(&dir, &fno);
	if (result != FR_OK || fno.fname[0] == 0) break;
	if (fno.fname[0] == '.') continue;
	if (! (fno.fattrib & AM_DIR)) continue; // only care about directories

	if (atoi(fno.fname) > max)
	    max = atoi(fno.fname);
    }
    return max;
}

int log_open(const char *dir, int number, char *name, FIL *file)
{
    FRESULT result;
    char filename[40];
    snprintf(filename, sizeof(filename), "%s/%d", dir, number);
    f_mkdir(filename);

    snprintf(filename, sizeof(filename), "%s/%d/%s", dir, number, name);
    result  = f_open(file, filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (result == FR_OK)
    {
	f_lseek(file, file->fsize);
    }
    return result;

}

void log_brew(FIL *file, char *fmt, ...)
{
    if (file->fs)
    {
	char message[40];
	UINT written;
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(message, sizeof(message) - 1, fmt, ap);
	va_end(ap);

	f_write(file, message, len, &written);
	f_sync(file);
    }
}

void log_close(FIL *file)
{
    if (file->fs)
	f_close(file);
}
