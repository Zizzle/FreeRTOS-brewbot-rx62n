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
 * This file is part of the uIP TCP/IP stack
 *
 * $Id: telnetd.c,v 1.2 2006/06/07 09:43:54 adam Exp $
 *
 */

#include "net/uip.h"
#include "telnetd.h"
#include "memb.h"
#include "shell.h"
#include "lcd.h"
#include "socket_io.h"
#include "serial.h"

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "types.h"

#define ISO_nl       0x0a
#define ISO_cr       0x0d

#define STATE_NORMAL 0
#define STATE_IAC    1
#define STATE_WILL   2
#define STATE_WONT   3
#define STATE_DO     4
#define STATE_DONT   5
#define STATE_CLOSE  6

//static struct telnetd_state s;

#define TELNET_IAC   255
#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254

xQueueHandle rx_queue;

struct rx_item 
{
    struct socket_state *ss;
    const void          *data;
    int                  data_len;
};

/*---------------------------------------------------------------------------*/
void shell_quit(struct socket_state *ss, char *str)
{
//  s.state = STATE_CLOSE;
}
/*---------------------------------------------------------------------------*/
static void sendline(struct socket_state *ss, char *line)
{
//    lcd_puts("send line %d %d", strlen(line), line[1]);
    sock_write(ss, line, strlen(line));
}
/*---------------------------------------------------------------------------*/
static void get_char(struct socket_state *ss, u8_t c)
{
    struct telnetd_state *s = ss->user_state;

    if(c == ISO_cr) {
	return;
    }
  
    s->buf[(int)s->bufptr] = c;
    if(s->buf[(int)s->bufptr] == ISO_nl ||
       s->bufptr == sizeof(s->buf) - 1) {
	if(s->bufptr > 0) {
	    s->buf[(int)s->bufptr] = 0;
	    /*      petsciiconv_topetscii(s.buf, TELNETD_CONF_LINELEN);*/
	}
	shell_input(ss, s->buf);
	s->bufptr = 0;
    } else {
	++s->bufptr;
    }
}
/*---------------------------------------------------------------------------*/
static void sendopt(struct socket_state *ss, u8_t option, u8_t value)
{
    char line[4];
    line[0] = TELNET_IAC;
    line[1] = option;
    line[2] = value;
    line[3] = 0;
    sendline(ss, line);
}
/*---------------------------------------------------------------------------*/
void shell_prompt(struct socket_state *ss, char *str)
{
    sendline(ss, str);
    sock_flush(ss);
}
/*---------------------------------------------------------------------------*/
void shell_output(struct socket_state *ss, char *str1, char *str2)
{
    char nl[3] = {ISO_cr, ISO_nl, 0};
    sendline(ss, str1);
    if (strlen(str2))
	sendline(ss, str2);
    sendline(ss, nl);
}

void new_data(struct socket_state *ss, const char *dataptr, uint16_t len)
{
    struct telnetd_state *s = ss->user_state;
    u8_t c;
  
    if (dataptr == NULL)
    {
	shell_start(ss);
	sock_flush(ss);
	return;
    }

    while(len > 0 && s->bufptr < sizeof(s->buf)) {
	c = *dataptr;
	++dataptr;
	--len;
	switch(s->state) {
	case STATE_IAC:
	    if(c == TELNET_IAC) {
		get_char(ss, c);
		s->state = STATE_NORMAL;
	    } else {
		switch(c) {
		case TELNET_WILL:
		    s->state = STATE_WILL;
		    break;
		case TELNET_WONT:
		    s->state = STATE_WONT;
		    break;
		case TELNET_DO:
		    s->state = STATE_DO;
		    break;
		case TELNET_DONT:
		    s->state = STATE_DONT;
		    break;
		default:
		    s->state = STATE_NORMAL;
		    break;
		}
	    }
	    break;
	case STATE_WILL:
	    /* Reply with a DONT */
	    sendopt(ss, TELNET_DONT, c);
	    s->state = STATE_NORMAL;
	    break;
      
	case STATE_WONT:
	    /* Reply with a DONT */
	    sendopt(ss, TELNET_DONT, c);
	    s->state = STATE_NORMAL;
	    break;
	case STATE_DO:
	    /* Reply with a WONT */
	    sendopt(ss, TELNET_WONT, c);
	    s->state = STATE_NORMAL;
	    break;
	case STATE_DONT:
	    /* Reply with a WONT */
	    sendopt(ss, TELNET_WONT, c);
	    s->state = STATE_NORMAL;
	    break;
	case STATE_NORMAL:
	    if(c == TELNET_IAC) {
		s->state = STATE_IAC;
	    } else {
		get_char(ss, c);
	    }
	    break;
	} 
    }    
    sock_flush(ss);
}

/*---------------------------------------------------------------------------*/
static void telnet_mainloop(void *pvParameters )
{
    while (1)
    {
	struct rx_item item;
	if (xQueueReceive( rx_queue, &item, 10000 ) == pdTRUE)
	{
	    debugf("RX %d\n", item.data_len);
	    new_data(item.ss, item.data, item.data_len);
	    if (item.data) free((void *)item.data);
	}
    }
}

/*---------------------------------------------------------------------------*/
void telnetd_init(void)
{
    uip_listen(HTONS(23));
    shell_init();

    rx_queue = xQueueCreate(10,  sizeof(struct rx_item));
    xTaskCreate( telnet_mainloop,
		 (const signed char *)"telnetd",
		 configMINIMAL_STACK_SIZE + 4096,
		 NULL,
		 1,
		 NULL );

}

static void telnet_send_message(struct socket_state *ss, const void *data, int len)
{
    struct rx_item item;
    item.ss             = ss;
    item.data           = data;
    item.data_len       = len;
    if (pdTRUE != xQueueSend( rx_queue, &item, 200))
    {
	serial_puts("Overflow telnet");
    }
    debugf("telnet %d\r\n", len);
}

/*---------------------------------------------------------------------------*/
// Reminder: this method cannot block or call socket_write
void telnetd_recv_callback(struct socket_state *ss)
{
    void *data = malloc(uip_datalen());
    memcpy(data, uip_appdata, uip_datalen());
    telnet_send_message(ss, data, uip_datalen());
}

/*---------------------------------------------------------------------------*/
// Reminder: this method cannot block or call socket_write
void telnetd_new_connection(struct socket_state *ss)
{
    struct telnetd_state *s = malloc(sizeof(struct telnetd_state));
    s->bufptr = 0;
    s->state = STATE_NORMAL;

    ss->recv_callback = telnetd_recv_callback;
    ss->user_state    = s;

    // let other thread know of new connection
    telnet_send_message(ss, NULL, 0);
}
