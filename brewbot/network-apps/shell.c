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

FIL File1, File2;

static void _write(char *str)
{
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

static void _read(char *str)
{
    UINT BytesRead;
    char buffer[30];
    FRESULT result  = f_open(&File1, "0:test.txt", FA_READ);
    char message[20];
    sprintf(message,"result %d", result);
    shell_output(message, "");

    result = f_read(&File1, buffer, 12, &BytesRead);
    sprintf(message,"result %d %d", result, BytesRead);
    shell_output(message, "");
    shell_output(buffer, "");

    f_close(&File1);
}

static void test(char * str)
{
    uint8_t buffer[138];
    int ii;

    for (ii = 0; ii < sizeof(buffer); ii++)
    {
	buffer[ii] = ii;
    }

    flash_write(0x1000, buffer, sizeof(buffer));
    flash_read(0x1000, buffer, sizeof(buffer));

    for (ii = 0; ii < sizeof(buffer) / 10; ii++)
    {
	shell_printf("%d %d %d %d %d %d %d %d %d %d",
		     buffer[ii * 10 + 0], buffer[ii * 10 + 1], buffer[ii * 10 + 2], buffer[ii * 10 + 3], buffer[ii * 10 + 4],
		     buffer[ii * 10 + 5], buffer[ii * 10 + 6], buffer[ii * 10 + 7], buffer[ii * 10 + 8], buffer[ii * 10 + 9]);

    }
}

/*---------------------------------------------------------------------------*/
static struct ptentry parsetab[] =
  {{"stats", help},
   {"conn", help},
   {"help", help},
   {"mount", mount},
   {"mkfs", mkfs},
   {"read", _read},
   {"write", _write},
   {"test", test},
   {"exit", shell_quit},
   {"?", help},

   /* Default action */
   {NULL, unknown}};
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
