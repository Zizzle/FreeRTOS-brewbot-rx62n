///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2007, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 12 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include "spi.h"
#include "iodefine.h"

#define SFL_CHIP_SELECT_SET      PORTC.DR.BIT.B0 = 0
#define SFL_CHIP_SELECT_CLR      PORTC.DR.BIT.B0 = 1

#define SPI_FLASH_INS_RDID       0x9F


void read_flash_id2(uint8_t idbuf[3])
{
    uint8_t out = SPI_FLASH_INS_RDID;
    SFL_CHIP_SELECT_SET;
    spi_write(&out, 1);
    spi_read(idbuf, 3);
    SFL_CHIP_SELECT_CLR;
}


