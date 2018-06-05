#ifndef FAT_H__
#define FAT_H__

#define FAT_SECT_SIZE	((off_t)512ULL)
#define FF_MAX_DISK 	(10)

typedef struct {
	int (*online)(void);
	int (*init)(void);
	int (*read)(BYTE *buf, DWORD start, UINT count);
	int (*write)(const BYTE *buf, DWORD start, UINT count);
	DWORD (*sectors)(void);
} fat_disk_ops;

const fat_disk_ops *ff_get_disk_ops(int disk);

#endif /* FAT_H__ */
