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

#include "shell.h"

#define SPI_FLASH_READ_ID        0x9F
#define SPI_FLASH_READ_BYTES     0x03
#define SPI_FLASH_WRITE_ENABLE   0x06
#define SPI_FLASH_WRITE_BYTES    0x22
#define SPI_FLASH_READ_STATUS    0x05

#define P5Q_WIP 0x1
#define P5Q_WEL 0x2
#define PAGE_SIZE 64

#define spi_select_p5q() if (!spi_select(SPI_DEVICE_P5Q)) return;

uint8_t flash_read_status_register()
{
    uint8_t status, out = SPI_FLASH_READ_STATUS;

    // if we fail to get the bus, then fake a Write-In-Progress
    if (!spi_select(SPI_DEVICE_P5Q))
    {
	return P5Q_WIP;
    }
    spi_write(&out, 1);
    spi_read(&status, 1);
    spi_release();

//    shell_printf("st %x", status);
    return status;
}

uint8_t flash_is_busy()
{
    return flash_read_status_register() & P5Q_WIP;
}


void flash_read_id(uint8_t idbuf[3])
{
    uint8_t out = SPI_FLASH_READ_ID;
    spi_select_p5q();
    spi_write(&out, 1);
    spi_read(idbuf, 3);
    spi_release();
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

    spi_select_p5q();
    spi_write(command, sizeof(command));
    spi_read(buffer, byteCount);
    spi_release();
}

void flash_write_enable()
{
    int ii;
    uint8_t command = SPI_FLASH_WRITE_ENABLE;
    spi_select_p5q();
    spi_write(&command, 1);
    spi_release();

    // wait for write enable bit
    for (ii = 0;
	 (flash_read_status_register() & P5Q_WEL) == 0; ii++)
    {
	// TODO check for timeout
	vTaskDelay(1);
    }
}

void flash_wait_for_write_complete()
{
    int ii;
    for (ii = 0;flash_is_busy(); ii++)
    {
	// TODO check for timeout
	vTaskDelay(1);
    }
    //shell_printf("waited %d", ii);
}

void flash_write(uint32_t addr, const uint8_t *buffer, uint32_t byteCount)
{
    uint8_t command[4];

    //shell_printf("writing to %x, %d bytes", addr, byteCount);

    while (byteCount)
    {
	uint8_t amt = (byteCount < PAGE_SIZE ? byteCount : PAGE_SIZE);

	// first send write enable
	flash_write_enable();

	command[0] = SPI_FLASH_WRITE_BYTES;
	command[1] = (addr >> 16) & 0xFF;
	command[2] = (addr >> 8) & 0xFF;
	command[3] = (addr) & 0xFF;

	//shell_printf("writing %x %x %x = %d %x", command[1], command[2], command[3], amt, buffer);

	spi_select_p5q();
	spi_write(command, 4);
	spi_write(buffer, amt);
	spi_release();

	byteCount -= amt;
	addr      += amt;
	buffer    += amt;

	flash_wait_for_write_complete();
    }
}
