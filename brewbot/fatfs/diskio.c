///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, Matthew Pratt, All Rights Reserved.
//
// Authors: Matthew Pratt
//
// Date: 13 Feb 2011
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "diskio.h"
#include "p5q.h"

#define FAT_SECTOR_SIZE 512

/*-----------------------------------------------------------------------*/
/* Initialize disk drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (BYTE drv /* Physical drive number (0) */)
{
    if (drv) return STA_NOINIT;			/* Supports only drive 0 */
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE drv /* Physical drive number (0) */)
{
    if (drv) return STA_NOINIT;		/* Supports only drive 0 */
    return 0;	/* Return disk status */
}

/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE drv,		/* Physical drive number (0) */
    BYTE *buff,		/* Pointer to the data buffer to store read data */
    DWORD sector,	/* Start sector number (LBA) */
    BYTE count		/* Number of sectors to read (1..128) */
    )
{
    if (drv || !count) return RES_PARERR;		/* Check parameter */

    flash_read(FLASH_ADDR_FAT_FILESYSTEM + FAT_SECTOR_SIZE * sector,
	       buff,
	       count * FAT_SECTOR_SIZE);

    return RES_OK;	/* Return result */
}



/*-----------------------------------------------------------------------*/
/* Write sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
    BYTE drv,			/* Physical drive number (0) */
    const BYTE *buff,	/* Ponter to the data to write */
    DWORD sector,		/* Start sector number (LBA) */
    BYTE count			/* Number of sectors to write (1..128) */
    )
{
    if (drv || !count) return RES_PARERR;		/* Check parameter */

    flash_write(FLASH_ADDR_FAT_FILESYSTEM + FAT_SECTOR_SIZE * sector,
	       buff,
	       count * FAT_SECTOR_SIZE);
    return RES_OK;	/* Return result */
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous drive controls other than data read/write               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE drv,		/* Physical drive number (0) */
    BYTE ctrl,		/* Control command code */
    void *buff		/* Pointer to the conrtol data */
    )
{
    DRESULT res = RES_ERROR;

    if (drv) return RES_PARERR;	/* Check parameter */

    switch (ctrl)
    {
    case CTRL_SYNC: /* Wait for end of internal write process of the drive */
	res = RES_OK;
	break;

    case GET_SECTOR_COUNT: /* Get drive capacity in unit of sector (DWORD) */
	*(DWORD*)buff = FLASH_SIZE / FAT_SECTOR_SIZE;
	res = RES_OK;
	break;

    case GET_SECTOR_SIZE:	/* Get sector size in unit of byte (WORD) */
	*(WORD*)buff = FAT_SECTOR_SIZE;
	res = RES_OK;
	break;

    case GET_BLOCK_SIZE:  /* Get erase block size in unit of sector (DWORD) */
	*(DWORD*)buff = 64;
	res = RES_OK;
	break;

    case CTRL_ERASE_SECTOR: /* Erase a block of sectors (used when _USE_ERASE == 1) */
	res = RES_OK;
	break;

    default:
	res = RES_PARERR;
    }
    return res;
}

DWORD get_fattime (void)
{
	/* No RTC feature provided. Return a fixed value 2011/1/29 0:00:00 */
	return	  ((DWORD)(2011 - 1980) << 25)	/* Y */
			| ((DWORD)1  << 21)				/* M */
			| ((DWORD)29 << 16)				/* D */
			| ((DWORD)0  << 11)				/* H */
			| ((DWORD)0  << 5)				/* M */
			| ((DWORD)0  >> 1);				/* S */
}
