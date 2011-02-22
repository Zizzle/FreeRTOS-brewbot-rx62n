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

#include "p5q.h"

#include "fatfs/ff.h"
FATFS Fatfs;

struct ptentry {
    char *commandstr;
    void (* pfunc)(char *str);
    int num_args;
};

#define SHELL_PROMPT "brewbot> "

void shell_printf(const char *fmt, ...)
{
    char message[38];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(message, sizeof(message) - 1, fmt, ap);
    va_end(ap);

    shell_output(message, "");
}

/*---------------------------------------------------------------------------*/
static void parse(register char *str, struct ptentry *t)
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
	  shell_printf("Need %d arguments", p->num_args);
      }
  }

  p->pfunc(str);
}
/*---------------------------------------------------------------------------*/
static void help(char *str)
{
  shell_output("Available commands:", "");
  shell_output("stats   - show network statistics", "");
  shell_output("conn    - show TCP connections", "");
  shell_output("help, ? - show help", "");
  shell_output("exit    - exit shell", "");
}
/*---------------------------------------------------------------------------*/
static void unknown(char *str)
{
  if(strlen(str) > 0) {
    shell_output("Unknown command: ", str);
  }
}
static void mount(char *str)
{
    FRESULT result  = f_mount (0, &Fatfs);
    char message[20];
    sprintf(message,"result %d", result);
    shell_output(message, "");
}
static void mkfs(char *str)
{
    FRESULT result  = f_mkfs (0, 1, 1024);
    char message[20];
    sprintf(message,"result %d", result);
    shell_output(message, "");
}


static void _write(char *str)
{
FIL File1;
    UINT ByteWritten;
    FRESULT result  = f_open(&File1, "0:test.txt", FA_OPEN_ALWAYS | FA_WRITE);
    char message[20];
    sprintf(message,"result %d", result);
    shell_output(message, "");

    result = f_write(&File1, "hello world", 12, &ByteWritten);
    sprintf(message,"result %d %d", result, ByteWritten);
    shell_output(message, "");

    f_close(&File1);
}

static void ls(char * str)
{
    DIR dir;
    FILINFO fno;
    char *fn;
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;


#if _USE_LFN
    static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    FRESULT result = f_opendir (&dir, "");
    if (result != FR_OK)
    {
	shell_printf("Opendir failed %x", result);
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
	shell_printf("%s%s", fn, (fno.fattrib & AM_DIR) ? "/" : "");
    }

    /* Get volume information and free clusters of drive 1 */
    result = f_getfree("0:", &fre_clust, &fs);
    if (result)
    {
	shell_printf("getfree failed %x", result);
	return;
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    /* Print free space in unit of KB (assuming 512 bytes/sector) */
    shell_printf("%lu KB total drive space.\n"
		 "%lu KB available.\n",
		 fre_sect / 2, tot_sect / 2);

}

static void mkdir(char * str)
{
    FRESULT result  = f_mkdir(str);
    if (result != FR_OK)
    {
	shell_printf("mkdir failed %x", result);
    }
}

static void cd(char *str)
{
    FRESULT result = f_chdir(str);

    shell_printf("change to %s", str);

    if (result != FR_OK)
    {
	shell_printf("chdir failed %x", result);
    }    
}

char buffer[80];

static void pwd(char *str)
{
    FRESULT result  = f_getcwd(buffer, sizeof(buffer));
    if (result != FR_OK)
    {
	shell_printf("pwd failed %x", result);
	return;
    }
    shell_printf("%s", buffer);
}


static void cat(char *str)
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
	shell_printf("Open failed %x", result);
	return;	
    }
    
    if (flags & FA_OPEN_ALWAYS)
    {
	result = f_lseek(&File1, File1.fsize);
	if (result != FR_OK)
	{
	    shell_printf("Seek failed %x", result);
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
	    shell_printf("%s\n", buffer);
	}
    }

    f_close(&File1);    
}

/*---------------------------------------------------------------------------*/
static struct ptentry parsetab[] =
    {{"stats", help, 0},
     {"conn", help, 0},
     {"help", help, 0},
     {"mount", mount, 0},
     {"mkfs", mkfs, 0},
     {"mkdir", mkdir, 1},
     {"cd",    cd, 1},
     {"cat",   cat, 1},
     {"pwd",   pwd, 0},
     {"exit", shell_quit, 0},
     {"ls",   ls, 0},
     {"?", help},
     {NULL, unknown}
};
/*---------------------------------------------------------------------------*/
void shell_init(void)
{
}
/*---------------------------------------------------------------------------*/
void shell_start(void)
{
  shell_output("uIP command shell", "");
  shell_output("Type '?' and return for help", "");
  shell_prompt(SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
void shell_input(char *cmd)
{
  parse(cmd, parsetab);
  shell_prompt(SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
