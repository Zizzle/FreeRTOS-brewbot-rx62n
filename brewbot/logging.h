///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date:  2 Mar 2011
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOGGING_H
#define LOGGING_H

int log_find_max_number(char *path);
int log_open(const char *dir, int number, char *name, FIL *file);
void log_brew(FIL *file, char *fmt, ...);
void log_close(FIL *file);

#endif
