#include <stdint.h>
#include <string.h>

#include "err.h"
#include "ff.h"
#include "fat.h"
#include "wrap.h"

#include "diskio.h"

#include <nds.h>
#include <stdio.h>

#include "fatdata_bin.h"

int fat_mount(char drive, const fat_disk_ops *disk_ops);

int ramimg_online(void)
{
	return 1;
}

int ramimg_read(BYTE *buf, DWORD start, UINT count)
{
	memcpy(buf, fatdata_bin + (start * 512), count * 512);
	return 0;
}

int ramimg_write(const BYTE *buf, DWORD start, UINT count)
{
	memcpy(fatdata_bin + (start * 512), buf, count * 512);
	return 0;
}

DWORD ramimg_sectors(void)
{
	return (fatdata_bin_end - fatdata_bin) / 512;
}

static const fat_disk_ops fat_ops = {
	.online = ramimg_online,
	.read = ramimg_read,
	.write = ramimg_write,
	.sectors = ramimg_sectors,
};

int mount_ramimg(char drive)
{
	return fat_mount(drive, &fat_ops);
}
