///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date: 12 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include "iodefine.h"
#include "types.h"

#define SPI_WAIT_FOR_IDLE()      while (RSPI0.SPSR.BIT.IDLNF);  // ensure transmit register is empty

void spi_open()
{    
    MSTP(RSPI0) = 0 ;             // enable module
    IOPORT.PFGSPI.BIT.RSPIS  = 0; // Select proper bank of pins for SPI0 
    IOPORT.PFGSPI.BIT.RSPCKE = 1; // SCK (PC.5) is active
    IOPORT.PFGSPI.BIT.SSL3E  = 0; // SSL3 (PC.2) is inactive (toggled as GPIO instead)
    IOPORT.PFGSPI.BIT.SSL0E  = 0; // SSL0 (PC.0) is inactive (toggled as GPIO instead)
    IOPORT.PFGSPI.BIT.MOSIE  = 1; // MOSI (PC.6) is active
    IOPORT.PFGSPI.BIT.MISOE  = 1; // MISO (PC.5) is active

    PORTC.DDR.BIT.B0 = 1;     // Set up SFL chip select pin. Make it an output
    PORTC.DR.BIT.B0  = 1;     // Set level to inactive

    PORTC.DDR.BIT.B2 = 1;     // Set up LCD chip select pin. Make it an output
    PORTC.DR.BIT.B2  = 1;     // Set level to inactive

    PORTC.DDR.BIT.B7 = 0;     // MISO as an input
    PORTC.DR.BIT.B7  = 0;
    PORTC.ICR.BIT.B7 = 1; 

    PORTC.DDR.BIT.B6 = 1;     // MOSI as an output
    PORTC.DR.BIT.B6  = 1;     // Enable input buffer for peripheral
    PORTC.DDR.BIT.B5 = 1;     // SCK as an output
    PORTC.DR.BIT.B5  = 1;     // Set level to inactive
    
    /* Initialize SPI (per flowchart in hardware manual) */
    RSPI0.SPPCR.BYTE = 0x00;  // No loopback, CMOS output
    RSPI0.SPBR.BYTE  = 0x05;  // Full speed
    RSPI0.SPDCR.BYTE = 0x00;  // 16-bit data 1 frame 1 chip select
    RSPI0.SPCKD.BYTE = 0x00;  // 2 clock delay before next access to SPI device
    RSPI0.SSLND.BYTE = 0x00;  // 2 clock delay after de-asserting SSL
    RSPI0.SPND.BYTE  = 0x00;  // 2 clock delay before next access to SPI device
    RSPI0.SPCR2.BYTE = 0x00;  // No parity no idle interrupts
    RSPI0.SPCMD0.WORD = 0x0700; // MSB first 8-bit data, keep SSL low
    //RSPI0.SPCR.BYTE  = 0x6B;  // Enable RSPI 3wire in master mode with RSPI Enable Transmit Only and Interupt 
    RSPI0.SPCR.BYTE  = 0xE9;  // Enable RSPI 3wire in master mode with RSPI Enable Transmit Only and Interupt 
    RSPI0.SSLP.BYTE  = 0x09;  // SSL3A Polarity
    RSPI0.SPSCR.BYTE = 0x00;  // One frame
}

void spi_write(uint8_t *out, uint32_t outlen)
{
    while (outlen--)
    {
	RSPI0.SPDR.WORD.L = 0x0;
	RSPI0.SPDR.WORD.H = 0x00ff & *out++;
	SPI_WAIT_FOR_IDLE();
	(void)RSPI0.SPDR.WORD.L;
	(void)RSPI0.SPDR.WORD.H;
    }    
}

void spi_read(uint8_t *in, uint32_t inlen)
{
    SPI_WAIT_FOR_IDLE();
    while (inlen--)
    {
	RSPI0.SPDR.WORD.H = 0x00ff; // write some dummy data
	SPI_WAIT_FOR_IDLE();
	*in++ = RSPI0.SPDR.WORD.H;
    }
}
