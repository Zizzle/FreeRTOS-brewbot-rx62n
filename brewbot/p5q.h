///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, licensed under the GPL3.
//
// Authors: Matthew Pratt
//
// Date: 12 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef P5Q_H
#define P5Q_H

void read_flash_id(uint8_t idbuf[3]);
void read_flash(uint32_t addr, uint8_t *buffer, uint32_t byteCount);
void flash_write(uint32_t addr, uint8_t *buffer, uint32_t byteCount);


#endif