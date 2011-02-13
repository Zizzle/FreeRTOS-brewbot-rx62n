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
#include "spi.h"
#include "iodefine.h"

#define SFL_CHIP_SELECT_SET      PORTC.DR.BIT.B0 = 0
#define SFL_CHIP_SELECT_CLR      PORTC.DR.BIT.B0 = 1

#define SPI_FLASH_INS_RDID       0x9F
#define SPI_FLASH_READ_BYTES     0x03
#define SPI_FLASH_WRITE_ENABLE   0x06
#define SPI_FLASH_WRITE_BYTES    0x22

void flash_read_id(uint8_t idbuf[3])
{
    uint8_t out = SPI_FLASH_INS_RDID;
    SFL_CHIP_SELECT_SET;
    spi_write(&out, 1);
    spi_read(idbuf, 3);
    SFL_CHIP_SELECT_CLR;
}


void flash_read(uint32_t addr, uint8_t *buffer, uint32_t byteCount)
{
    uint8_t command[4] =
	{
	    SPI_FLASH_READ_BYTES,
	    addr >> 16,
	    addr  >> 8,
	    addr
	};

    SFL_CHIP_SELECT_SET;
    spi_write(command, sizeof(command));
    spi_read(buffer, byteCount);
    SFL_CHIP_SELECT_CLR;    
}

void flash_write(uint32_t addr, uint8_t *buffer, uint32_t byteCount)
{
    uint8_t command[4] =
	{
	    SPI_FLASH_WRITE_ENABLE,
	    addr >> 16,
	    addr  >> 8,
	    addr
	};

    SFL_CHIP_SELECT_SET;
    spi_write(command, 1);
    SFL_CHIP_SELECT_CLR;    

    command[0] = SPI_FLASH_WRITE_BYTES;

    SFL_CHIP_SELECT_SET;
    spi_write(command, 4);

    spi_write(buffer, byteCount);

    SFL_CHIP_SELECT_CLR;    

}
