///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date: 12 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SPI_H
#define SPI_H

#include "types.h"

// the number of ticks spi_select will block waiting to get the mutex
#define SPI_DEFAULT_LOCK_WAIT 10000

void spi_open();
void spi_write(const uint8_t *out, uint32_t outlen);
void spi_read(uint8_t *in, uint32_t inlen);

enum SpiDevice
{
    SPI_DEVICE_NONE, // use for detecting read/write without locks
    SPI_DEVICE_LCD,
    SPI_DEVICE_P5Q
};

// Take a mutex and Chip select the device.
// Returns 0 on failure, 1 on success
int spi_select(enum SpiDevice device);

void spi_release();

#endif
