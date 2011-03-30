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

static int fds = 10;
#include "shell.h"
#include "fatfs/ff.h"

static FIL File1;
static char File1Name[100];

int open(const char *pathname, int flags)
{
    FRESULT result;
    flags = FA_READ;
    result = f_open(&File1, pathname, flags);
    if (result != FR_OK)
    {
	shell_printf("Open failed %x - %s", result, pathname);
	return -1;	
    }    

    strncpy(File1Name, pathname, 100);
    shell_printf("open %s", pathname);
    return (int)&File1;
}

ssize_t read(int fd, void *buf, size_t count)
{
    UINT nread = 0;
    shell_printf("read %d len %d", fd, count);
    f_read(&File1, buf, count, &nread); 
    shell_printf("n %d", nread);
    return nread;
}


int
write (int __fd, const void *__buf, size_t __nbyte )
{
    ((char *)__buf)[__nbyte] = 0;
    shell_printf("%s", __buf);
    return __nbyte;
}


int close(int fd)
{
    return 0;
}

int fstat(int fd, struct stat *buf)
{
    FILINFO file_info;
    if (f_stat (File1Name, &file_info) != FR_OK)
    {
	return -1;
    }

    shell_printf("stat %d", fd);
    buf->st_dev = 0;     /* ID of device containing file */
    buf->st_ino = 1;     /* inode number */
    buf->st_mode  = S_IFREG | S_IRUSR | S_IWUSR;    /* protection */
    buf->st_nlink = 1;   /* number of hard links */
    buf->st_uid = 0;     /* user ID of owner */
    buf->st_gid = 0;     /* group ID of owner */
    buf->st_rdev = 0;    /* device ID (if special file) */
    buf->st_size = file_info.fsize;    /* total size, in bytes */
    buf->st_blksize = 512; /* blocksize for file system I/O */
    buf->st_blocks = 100;  /* number of 512B blocks allocated */
    buf->st_atime =0;   /* time of last access */
    buf->st_mtime = 0;   /* time of last modification */
    buf->st_ctime = 0;   /* time of last status change */

    return 0;
}

off_t lseek(int fd, off_t offset, int whence)
{
    shell_printf("lseek %d", fd);
    return 0;
}

int isatty(int fd)
{
    return 1;
}

int unlink(const char *pathname)
{
    fprintf(stderr, "unlink\n");
}

clock_t times(struct tms *buf)
{
    fprintf(stderr, "times\n");
}

int link(const char *oldpath, const char *newpath)
{
    fprintf(stderr, "link\n");
}

int kill(pid_t pid, int sig)
{
    fprintf(stderr, "kill\n");
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    fprintf(stderr, "gettimeofday\n");
}

pid_t getpid(void)
{
    fprintf(stderr, "getpid\n");
}

