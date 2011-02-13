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

void spi_open();
void spi_write(uint8_t *out, uint32_t outlen);
void spi_read(uint8_t *in, uint32_t inlen);

#endif
