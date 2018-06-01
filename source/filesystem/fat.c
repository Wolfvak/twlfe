#include <stdint.h>

#include "err.h"
#include "ff.h"
#include "fat.h"
#include "vfs.h"
#include "wrap.h"

#include "diskio.h"

#include <nds.h>
#include <stdio.h>

static FATFS fat_filesystem[FF_MAX_DISK];

#define FF_LOG_PATH(x) ((char[]){'0' + (x), ':', '\0'})

static int __ff_vfs_mode(int vfs_mode)
{
	int ret = 0;
	if (vfs_mode & VFS_RO) ret |= FA_READ;
	if (vfs_mode & VFS_WO) ret |= FA_WRITE;
	if (vfs_mode & VFS_CREATE) ret |= FA_CREATE_ALWAYS;
	return ret;
}

int fat_vfs_mount(mount_t *mnt)
{
	FATFS *fs;
	FRESULT res;
	size_t ff_idx;
	u32 serial_int;
	char *label, *serial;

	ff_idx = (size_t)mnt->priv;
	if (ff_idx >= FF_MAX_DISK) return -ERR_DEV;

	fs = &fat_filesystem[ff_idx];
	res = f_mount(fs, FF_LOG_PATH(ff_idx), 1);
	if (res != FR_OK) return -ERR_DEV;

	label = zmalloc_abrt(FAT_LABEL_LENGTH);
	serial = zmalloc_abrt(FAT_SERIAL_LENGTH);

	mnt->info.size = (off_t)((fs->n_fatent - 2) * fs->csize) * FAT_SECT_SIZE;
	mnt->info.label = label;
	mnt->info.serial = serial;

	res = f_getlabel(FF_LOG_PATH(ff_idx), label, &serial_int);
	if (res != FR_OK) {
		const char *unk = "Unknown";
		strcpy(label, unk);
		strcpy(serial, unk);
	} else {
		snprintf(serial, FAT_SERIAL_LENGTH-1, "%08lX", serial_int);
	}

	return 0;
}

int fat_vfs_unmount(mount_t *mnt)
{
	FRESULT res;
	size_t ff_idx = (size_t)mnt->priv;
	FATFS *fs = &fat_filesystem[ff_idx];

	res = f_mount(NULL, FF_LOG_PATH(ff_idx), 0);
	if (res != FR_OK) return -ERR_DEV;

	free(mnt->info.label);
	free(mnt->info.serial);

	memset(fs, 0, sizeof(*fs));
	disk_remove(ff_idx);
	return 0;
}

int fat_vfs_open(mount_t *mnt, vf_t *file, const char *path, int mode)
{
	FRESULT res;
	FIL *ff_file;
	size_t ff_idx = (size_t)mnt->priv;
	char ff_local_path[MAX_PATH + 1];

	snprintf(ff_local_path, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), path);

	ff_file = zmalloc_abrt(sizeof(*ff_file));
	res = f_open(ff_file, ff_local_path, __ff_vfs_mode(mode));
	if (res != FR_OK) {
		free(ff_file);
		return -ERR_NOTFOUND;
	}

	file->priv = ff_file;
	return 0;
}

int fat_vfs_close(mount_t *mnt, vf_t *file)
{
	FIL *ff_file = file->priv;
	if (f_close(ff_file) != FR_OK)
		return -ERR_ARG;
	free(ff_file);
	return 0;
}

int fat_vfs_unlink(mount_t *mnt, const char *path)
{
	size_t ff_idx = (size_t)mnt->priv;
	char ff_local_path[MAX_PATH + 1];

	snprintf(ff_local_path, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), path);
	if (f_unlink(ff_local_path) != FR_OK)
		return -ERR_ARG;
	return 0;
}

off_t fat_vfs_read(mount_t *mnt, vf_t *file, void *buf, off_t size)
{
	size_t br;
	FIL *ff_file = file->priv;
	f_read(ff_file, buf, size, &br);
	return br;
}

off_t fat_vfs_write(mount_t *mnt, vf_t *file, const void *buf, off_t size)
{
	size_t br;
	FIL *ff_file = file->priv;
	f_write(ff_file, buf, size, &br);
	return br;
}

off_t fat_vfs_seek(mount_t *mnt, vf_t *file, off_t off)
{
	FIL *ff_file = file->priv;
	off_t fpos = f_tell(ff_file) + off;

	if (fpos < 0) fpos = 0;
	f_lseek(ff_file, fpos);
	return f_tell(ff_file);
}

off_t fat_vfs_size(mount_t *mnt, vf_t *file)
{
	FIL *ff_file = file->priv;
	return f_size(ff_file);
}

int fat_vfs_mkdir(mount_t *mnt, const char *path)
{
	FRESULT res;
	char ff_local_path[MAX_PATH + 1];
	size_t ff_idx = (size_t)mnt->priv;

	snprintf(ff_local_path, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), path);
	res = f_mkdir(ff_local_path);
	if (res == FR_OK)
		return 0;
	return -ERR_ARG;
}

int fat_vfs_diropen(mount_t *mnt, vf_t *dir, const char *path)
{
	FRESULT res;
	DIR *dp;
	char ff_local_path[MAX_PATH + 1];
	size_t ff_idx = (size_t)mnt->priv;

	dp = zmalloc_abrt(sizeof(*dp));

	snprintf(ff_local_path, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), path);
	res = f_opendir(dp, ff_local_path);
	if (res != FR_OK) {
		free(dp);
		return -ERR_NOTFOUND;
	}

	dir->priv = dp;
	return 0;
}

int fat_vfs_dirclose(mount_t *mnt, vf_t *dir)
{
	DIR *dp = dir->priv;
	if (f_closedir(dp) != FR_OK) return -ERR_ARG;
	free(dp);
	return 0;
}

int fat_vfs_dirnext(mount_t *mnt, vf_t *dir, dirent_t *next)
{
	FRESULT res;
	FILINFO *fno;
	DIR *dp = dir->priv;

	if (next->priv == NULL) {
		fno = zmalloc_abrt(sizeof(*fno));
		next->priv = fno;
	} else {
		fno = next->priv;
	}

	res = f_readdir(dp, fno);
	if (res != FR_OK || fno->fname[0] == '\0') {
		next->path = NULL;
		return -ERR_NOTFOUND;
	}
	next->path = fno->fname;

	if (fno->fattrib & AM_DIR) {
		next->flags = VFS_DIR;
	} else {
		next->flags = VFS_FILE;
	}
	return 0;
}

static const vfs_ops_t fat_ops = {
	.mount = fat_vfs_mount,
	.unmount = fat_vfs_unmount,

	.open = fat_vfs_open,
	.close = fat_vfs_close,
	.unlink = fat_vfs_unlink,

	.read = fat_vfs_read,
	.write = fat_vfs_write,
	.seek = fat_vfs_seek,
	.size = fat_vfs_size,

	.mkdir = fat_vfs_mkdir,
	.diropen = fat_vfs_diropen,
	.dirclose = fat_vfs_dirclose,
	.dirnext = fat_vfs_dirnext,
};

int fat_mount(char drive, const fat_disk_ops *disk_ops)
{
	int ff_idx = 0;
	const mount_t mount = {
		.ops = &fat_ops,
		.caps = VFS_RW | VFS_CREATE,
		.priv = (void*)ff_idx,
	};

	disk_register(ff_idx, disk_ops);
	return vfs_mount(drive, &mount);
}
