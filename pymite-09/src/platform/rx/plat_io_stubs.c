///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 30 Mar 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

extern int py_main(void);

int main(void)
{
    py_main();
    return 0;
}

int open(const char *pathname, int flags)
{
    return -1;
}

ssize_t read(int fd, void *buf, size_t count)
{
    return -1;
}


int
write (int __fd, const void *__buf, size_t __nbyte )
{
    // DO SERIAL IO HERE?
    return __nbyte;
}


int close(int fd)
{
    return 0;
}

int fstat(int fd, struct stat *buf)
{
    return -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
    return 0;
}

int isatty(int fd)
{
    return 1;
}


