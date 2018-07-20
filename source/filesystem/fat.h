#ifndef FAT_H__
#define FAT_H__

#include <nds.h>

#include "err.h"
#include "vfs.h"

#define FAT_SECT_SIZE	((off_t)512ULL)
#define FF_MAX_DISK 	(10)

typedef struct {
	int (*init)(void);
	int (*online)(void);
	int (*read)(BYTE *buf, DWORD start, UINT count);
	int (*write)(const BYTE *buf, DWORD start, UINT count);
} fat_disk_ops;

const fat_disk_ops *ff_get_disk_ops(int disk);
int fat_mount(char drive, const fat_disk_ops *disk_ops);

#endif /* FAT_H__ */
