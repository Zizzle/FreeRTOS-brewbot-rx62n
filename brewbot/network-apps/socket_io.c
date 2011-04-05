///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date:  1 Apr 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "net/uip.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "telnetd.h"

#include "lcd.h"
#include "serial.h"

extern int uIP_request_poll(struct uip_conn *uip_conn);

int sock_read(struct socket_state *ss, void *buf, size_t count)
{
    if (ss->status == CLOSED)
	return -1;

    if (count == 0) return 0;

    while (ss->recv_count < count && ss->status == OPEN)
	xSemaphoreTake(ss->semaphore, 100000);

    if (ss->recv_count >= count)
    {
	taskENTER_CRITICAL();
	memcpy(buf, ss->recv_buf, count);
	memmove(ss->recv_buf, ss->recv_buf + count, ss->recv_count - count);
	ss->recv_count -= count;
	taskEXIT_CRITICAL();
	return count;
    }
    return -1;
}

int sock_flush(struct socket_state *ss)
{
    // bail if nothing to flush
    if (ss->tx_count == 0)
	return 0;

    debugf("flush %d\r\n", ss->tx_count);

    ss->txing = 1;
    uIP_request_poll(ss->uip_conn);

    debugf("flushed %d\r\n", 0); //ss->uip_conn);

    if (ss->status == CLOSED ||
	xSemaphoreTake(ss->semaphore, 100000) != pdTRUE ||
	ss->status == CLOSED)
    {
	debugf("sock closed\r\n");
	return -1;
    }

    debugf("acked\r\n");
    ss->txing    = 0;
    ss->tx_count = 0;
    return 0;
}

int sock_write(struct socket_state *ss, const void *buf, size_t count)
{
    size_t left = count;
    size_t amt;
    int free;

    debugf("write %ld ", count);
    serial_write(buf, count);
    debugf("\r\n");

    if (ss->status == CLOSED)
    {
	return -1;
    }

    while (left > 0)
    {
	free = uip_mss() - ss->tx_count;
	amt  = left < free ? left : free;

	memcpy(ss->tx_buf + ss->tx_count, buf, amt);
	ss->tx_count += amt;

	buf  += amt;
	left -= amt;

	if (ss->tx_count == uip_mss())
	{
	    if (sock_flush(ss) == -1)
		return -1;
	}
    }
    return count;
}


static void opened(struct socket_state *ss)
{
    vSemaphoreCreateBinary( ss->semaphore );
    xSemaphoreTake(ss->semaphore, 10);
    ss->status    = OPEN;
    ss->uip_conn  = uip_conn;
    ss->tx_buf    = malloc(uip_mss());
    ss->tx_count  = 0;
    ss->txing     = 0;
    ss->recv_callback = NULL;
}

static void closed(struct socket_state *ss)
{
    ss->status = CLOSED;
    xSemaphoreGive(ss->semaphore);
    vQueueDelete(ss->semaphore);
    ss->semaphore = NULL;
    if (ss->user_state != NULL)
    {
	free(ss->user_state);
	ss->user_state = NULL;
    }
}

static void acked(struct socket_state *ss)
{
    ss->txing    = 0;
    xSemaphoreGive(ss->semaphore);
}

static void newdata(struct socket_state *ss)
{
    if (ss->recv_callback != NULL)
    {
	ss->recv_callback(ss);
	return;
    }

    if (uip_datalen() <= SOCKET_RECV_BUFF_SIZE - ss->recv_count)
    {
	taskENTER_CRITICAL();
	memcpy(ss->recv_buf + ss->recv_count, uip_appdata, uip_datalen());
	ss->recv_count += uip_datalen();
	taskEXIT_CRITICAL();
	xSemaphoreGive(ss->semaphore);
    }
    else
    {
	// dropped some data
    }
}

static void senddata(struct socket_state *ss)
{
    if (ss->txing)
    {
	uip_send(ss->tx_buf, ss->tx_count);
    }
}

/*---------------------------------------------------------------------------*/
void socket_appcall(void)
{
    struct socket_state *ss = (&uip_conn->appstate.socket_state);

//    debugf("%ld appcall %x\r\n", xTaskGetTickCount(), uip_flags); 

  if (uip_connected())
  {
      opened(ss);

      switch(uip_conn->lport)
      {
      case HTONS(23):  telnetd_new_connection(ss); break;
      }
  }
  
  if (uip_closed() ||
      uip_aborted() ||
      uip_timedout())
  {
      closed(ss);
  }
  
  if(uip_acked())
  {
      acked(ss);
  }
  
  if(uip_newdata())
  {
      newdata(ss);
  }
  
  if (uip_rexmit() ||
      uip_newdata() ||
      uip_acked() ||
      uip_connected() ||
      uip_poll())
  {
      senddata(ss);
  }
}
/*---------------------------------------------------------------------------*/
