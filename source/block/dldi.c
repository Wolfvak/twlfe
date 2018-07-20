#include <nds.h>
#include <nds/arm9/dldi.h>

#include "global.h"
#include "err.h"

#include "vfs.h"

#include "fat.h"

#include "ff.h"

int dldi_init(void)
{
	const DISC_INTERFACE *dldi = dldiGetInternal();
	if (dldi == NULL) return 1;
	return dldi->startup();
}

int dldi_online(void)
{
	const DISC_INTERFACE *dldi = dldiGetInternal();
	if (dldi == NULL) return 1;
	return dldi->isInserted();
}

int dldi_read(BYTE *buf, DWORD start, UINT count)
{
	const DISC_INTERFACE *dldi = dldiGetInternal();
	if (dldi == NULL) return 1;
	return !dldi->readSectors(start, count, buf);
}

int dldi_write(const BYTE *buf, DWORD start, UINT count)
{
	const DISC_INTERFACE *dldi = dldiGetInternal();
	if (dldi == NULL) return 1;
	return !dldi->writeSectors(start, count, buf);
}

static const fat_disk_ops fat_ops = {
	.init = dldi_init,
	.online = dldi_online,
	.read = dldi_read,
	.write = dldi_write,
};

int dldi_mount(char drive)
{
	return fat_mount(drive, &fat_ops);
}
