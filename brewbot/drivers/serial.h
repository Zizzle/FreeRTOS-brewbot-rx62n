///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date:  4 Apr 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SERIAL_H
#define SERIAL_H

void serial_open(void);
void serial_puts(const char *line);
void serial_write(const char *buf, int len);
void debugf(const char *fmt, ...);

#endif
