///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date:  4 Apr 2011
//
///////////////////////////////////////////////////////////////////////////////

#include "iodefine.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void serial_open(void) 
{
    int i;

    MSTP(SCI2)=0;    

    //initialize serial port on pins 42 and 44.
    //this is RxD2-B and TxD2B
    //See section 28.3.4 of the RX hardware user manual
    SCI2.SCR.BYTE = 0;  //clear TIE, RIE, TE, RE, TEIE
    PORT5.ICR.BIT.B2 = 1;  //enable input buffer of RxD2B
    SCI2.SMR.BYTE = 0; //asynchronous, 8bit data, no parity, 1 stop bit, multi-processor disabled, PCLK clock (n=0 in BRR)
    SCI2.SCMR.BYTE = 0; //242; //0b11110010: disable smart card interface and use SCI mode

    //See 16.1.2.22
    IOPORT.PFFSCI.BIT.SCI2S = 1;  //use the 'b' pins

    /* div-8 mode in BRR */
    SCI2.SEMR.BYTE = 0x10;

    /* Calculate baud rate. */
    SCI2.BRR = 25;

    for (i=2200; i; --i)
	asm("");

    /* Enable transmitter, receiver, rx interrupts */
    SCI2.SCR.BYTE = 0x70;
}

// can use printf instead
void serial_puts(const char *line)
{
    while (*line)
    {
        //Wait for the Tx buffer to be empty
        while (SCI2.SSR.BIT.TEND == 0){}
        //Send the Tx char
        SCI2.TDR = *line++;
    }
}

void serial_write(const char *buf, int len)
{
    while (len--)
    {
        //Wait for the Tx buffer to be empty
        while (SCI2.SSR.BIT.TEND == 0){}
        //Send the Tx char
        SCI2.TDR = *buf++;
    }
}

void debugf(const char *fmt, ...)
{
    char message[81];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(message, sizeof(message) - 1, fmt, ap);
    va_end(ap);

    serial_write(message, len);
}
