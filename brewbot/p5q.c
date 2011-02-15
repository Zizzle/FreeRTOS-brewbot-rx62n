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
#include "FreeRTOS.h"
#include "task.h"

#define SFL_CHIP_SELECT_SET      PORTC.DR.BIT.B0 = 0
#define SFL_CHIP_SELECT_CLR      PORTC.DR.BIT.B0 = 1

#define SPI_FLASH_READ_ID        0x9F
#define SPI_FLASH_READ_BYTES     0x03
#define SPI_FLASH_WRITE_ENABLE   0x06
#define SPI_FLASH_WRITE_BYTES    0x22
#define SPI_FLASH_READ_STATUS    0x05

#define P5Q_WIP 0x1
#define P5Q_WEL 0x2
#define PAGE_SIZE 64

uint8_t flash_read_status_register()
{
    uint8_t status, out = SPI_FLASH_READ_STATUS;

    SFL_CHIP_SELECT_SET;
    spi_write(&out, 1);
    spi_read(&status, 1);
    SFL_CHIP_SELECT_CLR;
    return status;
}

uint8_t flash_is_busy()
{
    return flash_read_status_register() & P5Q_WIP;
}


void flash_read_id(uint8_t idbuf[3])
{
    uint8_t out = SPI_FLASH_READ_ID;
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

void flash_write(uint32_t addr, const uint8_t *buffer, uint32_t byteCount)
{
    int ii;
    uint8_t command[4] =
	{
	    SPI_FLASH_WRITE_ENABLE,
	    addr >> 16,
	    addr  >> 8,
	    addr
	};

    // first send write enable
    SFL_CHIP_SELECT_SET;
    spi_write(command, 1);
    SFL_CHIP_SELECT_CLR;    

    command[0] = SPI_FLASH_WRITE_BYTES;

    while (byteCount)
    {
	uint8_t amt = (byteCount < PAGE_SIZE ? byteCount : PAGE_SIZE);

	for (ii = 0;flash_is_busy(); ii++)
	{
	    // TODO check for timeout
	     vTaskDelay(1);
	}

	command[1] = (addr >> 16) & 0xFF;
	command[2] = (addr >> 8) & 0xFF;
	command[3] = (addr) & 0xFF;

	SFL_CHIP_SELECT_SET;
	spi_write(command, 4);
	spi_write(buffer, amt);
	SFL_CHIP_SELECT_CLR;

	byteCount -= amt;
	addr      += amt;
    }
}
