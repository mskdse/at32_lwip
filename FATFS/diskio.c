/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */
#include "msc_diskio.h"

/* Example: Mapping of physical drive number for each drive */
#define DEV_USB	    0	/* Map FTL to physical drive 0 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	switch (pdrv) {

	case DEV_USB :
		return 0;
	
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	switch (pdrv) {

	case DEV_USB :
		w25x80_init();
	return 0;
	
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	switch (pdrv) {

	case DEV_USB :
		msc_disk_read(DEV_USB,sector*4096,buff,count*4096);
		return RES_OK;
	
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	switch (pdrv) {
	
	case DEV_USB :
		msc_disk_write(DEV_USB,sector*4096,(uint8_t*)buff,count*4096);
		return RES_OK;
	
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch (pdrv) {

	case DEV_USB :
	{
		if(GET_SECTOR_COUNT==cmd)      *((LBA_t*)buff)=256;
		else if(GET_SECTOR_SIZE==cmd)  *((WORD*)buff)=4096;
		else if(GET_BLOCK_SIZE==cmd)   *((DWORD*)buff)=1;
	}return RES_OK;

	}

	return RES_PARERR;
}

DWORD get_fattime (void)
{
    return (DWORD)(2026 - 80) << 25 |
           (DWORD)(8 + 1) << 21 |
           (DWORD) 7 << 16 |
           (DWORD) 17 << 11 |
           (DWORD) 6 << 5 |
           (DWORD) 30 >> 1;
}
