/*
 * Copyright (c) 2002, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
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
 * $Id: webserver.h,v 1.2 2006/06/11 21:46:38 adam Exp $
 *
 */
#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include "apps/httpd/httpd.h"

void uip_tcp_appcall(void);

struct uIP_message
{
    struct uip_conn	*uip_conn;
};

enum SockStatus
{
    CLOSED,
    OPEN
};

#define SOCKET_RECV_BUFF_SIZE 2048

struct ftpd_state
{
    int count;
    int pos;
    char IsCmdWD;    
    unsigned char Status;
    unsigned char RecvCmd;
    unsigned char AnsToCmd;
    unsigned char ftpMode;
    unsigned char ftpType;
    unsigned char ftpStru;
    struct uip_conn *uip_conn;
    struct ftpd_state *other;
    char cwd[128];
};

struct socket_state
{
    struct uip_conn	*uip_conn;
    void                *semaphore;
    int                  status;
    unsigned char       *recv_buf;
    int                  recv_count;
    void               (*recv_callback)(struct socket_state *ss);
    void                *user_state;

    unsigned char       *tx_buf;
    int                  tx_count;
    char                 txing;
};

union _app_state
{
    struct socket_state  socket_state;
    struct httpd_state   httpd_appstate;
    struct ftpd_state    ftpd_state;
};

typedef union _app_state uip_tcp_appstate_t;
/* UIP_APPCALL: the name of the application function. This function
   must return void and take no arguments (i.e., C type "void
   appfunc(void)"). */
#define UIP_APPCALL     uip_tcp_appcall 


#endif /* __WEBSERVER_H__ */
