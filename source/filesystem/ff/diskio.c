/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
#include <nds.h>

#include "diskio.h"		/* FatFs lower layer API */

#include "fat.h"

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	const fat_disk_ops *ops = ff_get_disk_ops(pdrv);
	if (ops != NULL && ops->online()) return RES_OK;
	return RES_NOTRDY;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	const fat_disk_ops *ops = ff_get_disk_ops(pdrv);
	if (ops != NULL && ops->init()) return RES_OK;
	return RES_NOTRDY;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	const fat_disk_ops *ops = ff_get_disk_ops(pdrv);
	if (ops == NULL) return RES_NOTRDY;
	if (ops->read(buff, sector, count) == 0) return RES_OK;
	return RES_NOTRDY;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	const fat_disk_ops *ops = ff_get_disk_ops(pdrv);
	if (ops == NULL) return RES_NOTRDY;
	if (ops->write(buff, sector, count) == 0) return RES_OK;
	return RES_NOTRDY;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	const fat_disk_ops *ops = ff_get_disk_ops(pdrv);
	if (ops == NULL) return RES_PARERR;

	switch(cmd) {
		case CTRL_SYNC:
			break;
		case GET_SECTOR_SIZE:
			*(DWORD*)buff = 512;
			break;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = 32768;
			break;
		default:
			return RES_PARERR;
	}

	return RES_OK;
}
