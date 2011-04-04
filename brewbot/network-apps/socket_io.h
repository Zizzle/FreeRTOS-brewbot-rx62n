///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date:  2 Apr 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SOCKET_IO_H
#define SOCKET_IO_H

#include "webserver.h"
#include <stdio.h>

void socket_appcall(void);

int sock_read(struct socket_state *ss, void *buf, size_t count);
int sock_write(struct socket_state *ss, const void *buf, size_t count);
int sock_flush(struct socket_state *ss);


#endif
