/*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: shell.c,v 1.1 2006/06/07 09:43:54 adam Exp $
 *
 */

#include "shell.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "p5q.h"
#include "fatfs/ff.h"
#include "settings.h"
#include "audio.h"
#include "net/uip.h"
#include "menu.h"
#include "buttons.h"
#include "socket_io.h"

#include "FreeRTOS.h"
#include "task.h"

extern FATFS Fatfs;

struct ptentry {
    char *commandstr;
    void (* pfunc)(struct socket_state *ss, char *str);
    int num_args;
};

#define SHELL_PROMPT "brewbot> "

void shell_printf(struct socket_state *ss, const char *fmt, ...)
{
    char message[38];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(message, sizeof(message) - 1, fmt, ap);
    va_end(ap);

    shell_output(ss, message, "");
}

/*---------------------------------------------------------------------------*/
static void parse(struct socket_state *ss, register char *str, struct ptentry *t)
{
    struct ptentry *p;
    for(p = t; p->commandstr != NULL; ++p) {
	if(strncmp(p->commandstr, str, strlen(p->commandstr)) == 0) {
	    break;
	}
    }

    if (p->num_args)
    {
	str = strchr(str, ' ');
	if (str != NULL)
	{
	    str++; // skip the space
	}
	else
	{
	    shell_printf(ss, "Need %d arguments", p->num_args);
	}
    }
    p->pfunc(ss, str);
}
/*---------------------------------------------------------------------------*/
static void help(struct socket_state *ss, char *str)
{
    shell_output(ss, "Available commands:", "");
    shell_output(ss, "stats   - show network statistics", "");
    shell_output(ss, "conn    - show TCP connections", "");
    shell_output(ss, "help, ? - show help", "");
    shell_output(ss, "exit    - exit shell", "");
}
/*---------------------------------------------------------------------------*/
static void unknown(struct socket_state *ss, char *str)
{
    if(strlen(str) > 0) {
	shell_output(ss, "Unknown command: ", str);
    }
}

static void mkfs(struct socket_state *ss, char *str)
{
    FRESULT result  = f_mkfs (0, 1, 1024);
    char message[20];
    sprintf(message,"result %d", result);
    shell_output(ss, message, "");
}

static void ls(struct socket_state *ss, char * str)
{
    DIR dir;
    FILINFO fno;
    char *fn;
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;


#if _USE_LFN
    static char lfn[_MAX_LFN * + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    FRESULT result = f_opendir (&dir, "");
    if (result != FR_OK)
    {
	shell_printf(ss, "Opendir failed %x", result);
	return;
    }

    for (;;) {
	result = f_readdir(&dir, &fno);
	if (result != FR_OK || fno.fname[0] == 0) break;
	if (fno.fname[0] == '.') continue;
#if _USE_LFN
	fn = *fno.lfname ? fno.lfname : fno.fname;
#else
	fn = fno.fname;
#endif
	shell_printf(ss, "%s%s", fn, (fno.fattrib & AM_DIR) ? "/" : "");
    }

    /* Get volume information and free clusters of drive 1 */
    result = f_getfree("0:", &fre_clust, &fs);
    if (result)
    {
	shell_printf(ss, "getfree failed %x", result);
	return;
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    /* Print free space in unit of KB (assuming 512 bytes/sector) */
    shell_printf(ss,
		 "%lu KB total.\n"
		 "%lu KB available.\n",
		 tot_sect / 2, fre_sect / 2);

}

static void mkdir(struct socket_state *ss, char * str)
{
    FRESULT result  = f_mkdir(str);
    if (result != FR_OK)
    {
	shell_printf(ss, "mkdir failed %x", result);
    }
}

static void cd(struct socket_state *ss, char *str)
{
    FRESULT result = f_chdir(str);

    shell_printf(ss, "change to %s", str);

    if (result != FR_OK)
    {
	shell_printf(ss, "chdir failed %x", result);
    }    
}

char buffer[80];

static void pwd(struct socket_state *ss, char *str)
{
    FRESULT result  = f_getcwd(buffer, sizeof(buffer));
    if (result != FR_OK)
    {
	shell_printf(ss, "pwd failed %x", result);
	return;
    }
    shell_printf(ss, "%s", buffer);
}


static void cat(struct socket_state *ss, char *str)
{
    FIL File1;
    FRESULT result;
    char *name = str;
    int flags = FA_READ;

    if (name[0] == '>')
    {
	name++;
	flags = FA_CREATE_ALWAYS | FA_WRITE;
	if (name[0] == '>')
	{
	    name++;
	    flags = FA_OPEN_ALWAYS | FA_WRITE;
	}

	while (*name++ == ' '); // skip the whitespace
    }

    result = f_open(&File1, name, flags);
    if (result != FR_OK)
    {
	shell_printf(ss, "Open failed %x", result);
	return;	
    }
    
    if (flags & FA_OPEN_ALWAYS)
    {
	result = f_lseek(&File1, File1.fsize);
	if (result != FR_OK)
	{
	    shell_printf(ss, "Seek failed %x", result);
	    f_close(&File1);
	    return;
	}
    }

    if (flags & FA_WRITE)
    {

    }
    else
    {
	while (f_gets (buffer, sizeof(buffer), &File1) != NULL)
	{
	    sock_write(ss, buffer, strlen(buffer));
	}
    }

    f_close(&File1);    
}

static void settings(struct socket_state *ss, char *str)
{
    settings_shell_display();
}

static void settings_set(struct socket_state *ss, char *str)
{
    char *second = strchr(str, ' ');
    if (second == NULL)
    {
	shell_printf(ss, "Need 2 arguments");
	return;
    }

    *second = 0; // terminate the first argument
    shell_printf(ss, "%s", settings_update(str, second + 1));
}

static void beep(struct socket_state *ss, char *str)
{
    audio_beep(atoi(str), 300);
}

static void ps(struct socket_state *ss, char *str)
{
    extern void vTaskList( signed char *pcWriteBuffer );
    extern char *pcGetTaskStatusMessage( void );
    vTaskList( uip_appdata );
    uip_send(uip_appdata, strlen(uip_appdata));
    shell_printf(ss, "");
}

static void up(struct socket_state *ss, char *str)
{
    menu_key(KEY_UP & KEY_PRESSED);
    menu_key(KEY_UP);
}
static void left(struct socket_state *ss, char *str)
{
    menu_key(KEY_LEFT & KEY_PRESSED);
    menu_key(KEY_LEFT);
}
static void right(struct socket_state *ss, char *str)
{
    menu_key(KEY_RIGHT & KEY_PRESSED);
    menu_key(KEY_RIGHT);
}
static void down(struct socket_state *ss, char *str)
{
    menu_key(KEY_DOWN & KEY_PRESSED);
//    menu_key(KEY_DOWN);
}

static void rm(struct socket_state *ss, char *str)
{
    FRESULT result = f_unlink(str);
    if (result != FR_OK)
    {
	shell_printf(ss, "Failed %d", result);
    }
}

static void python(struct socket_state *ss, char *str)
{
    xTaskSetStdio(NULL, 0, ss);
    xTaskSetStdio(NULL, 1, ss);
    xTaskSetStdio(NULL, 2, ss);
    py_main();
    xTaskSetStdio(NULL, 0, NULL);
    xTaskSetStdio(NULL, 1, NULL);
    xTaskSetStdio(NULL, 2, NULL);
}

/*---------------------------------------------------------------------------*/
static struct ptentry parsetab[] =
{
    {"stats",    help,         0},
    {"conn",     help,         0},
    {"help",     help,         0},
    {"mkfs",     mkfs,         0},
    {"mkdir",    mkdir,        1},
    {"cd",       cd,           1},
    {"cat",      cat,          1},
    {"pwd",      pwd,          0},
    {"ls",       ls,           0},
    {"rm",       rm,           1},
    {"ps",       ps,           0},
    {"settings", settings,     0},
    {"set",      settings_set, 2},
    {"beep",     beep,         1},
    {"up",       up,           0},
    {"down",     down,         0},
    {"left",     left,         0},
    {"right",    right,        0},
    {"python",   python,       0},
    {"exit",     shell_quit,   0},
    {"?",        help},
    {NULL, unknown}
};
/*---------------------------------------------------------------------------*/
void shell_init(void)
{
}
/*---------------------------------------------------------------------------*/
void shell_start(struct socket_state *ss)
{
    shell_output(ss, "uIP command shell", "");
    shell_output(ss, "Type '?' and return for help", "");
    shell_prompt(ss, SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
void shell_input(struct socket_state *ss, char *cmd)
{
    parse(ss, cmd, parsetab);
    shell_prompt(ss, SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
