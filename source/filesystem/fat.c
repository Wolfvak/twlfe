#include <nds.h>
#include <stdio.h>

#include "err.h"
#include "vfs.h"

#include "ff.h"
#include "fat.h"

struct {
	FATFS fs;
	const fat_disk_ops *disk_ops;
	int mounted;
} fat_info[FF_MAX_DISK];

#define FF_LOG_PATH(x)	((char[]){'0' + (x), ':', '\0'})
#define FF_GET_IDX(x)	((size_t)((x)->priv))

const fat_disk_ops *ff_get_disk_ops(int disk)
{
	if (disk < 0 || disk >= FF_MAX_DISK ||
		!fat_info[disk].mounted) return NULL;
	return fat_info[disk].disk_ops;
}

static int _ff_vfs_mode(int vfs_mode)
{
	int ret = 0;
	if (vfs_mode & VFS_RO) ret |= FA_READ;
	if (vfs_mode & VFS_WO) ret |= FA_WRITE;
	if (vfs_mode & VFS_CREATE) ret |= FA_CREATE_ALWAYS;
	return ret;
}

static int _ff_err(int err)
{
	switch(err) {
		case FR_OK:
			return 0;
			break;

		case FR_NOT_READY:
		case FR_NO_FILESYSTEM:
		case FR_WRITE_PROTECTED:
			return -ERR_NOTREADY;
			break;

		case FR_NO_FILE:
		case FR_NO_PATH:
			return -ERR_NOTFOUND;
			break;

		case FR_INVALID_NAME:
		case FR_INVALID_DRIVE:
			return -ERR_ARG;
			break;

		case FR_NOT_ENOUGH_CORE:
			return -ERR_MEM;
			break;

		default:
			return -ERR_DEV;
			break;
	}
}

int fat_vfs_mount(mount_t *mnt)
{
	FATFS *fs;
	FRESULT res;
	size_t ff_idx = FF_GET_IDX(mnt);

	if (ff_idx >= FF_MAX_DISK) return -ERR_DEV;
	fat_info[ff_idx].mounted = 1;

	fs = &fat_info[ff_idx].fs;
	res = f_mount(fs, FF_LOG_PATH(ff_idx), 1);
	if (res != FR_OK)
		fat_info[ff_idx].mounted = 0;

	return _ff_err(res);
}

int fat_vfs_unmount(mount_t *mnt)
{
	FRESULT res;
	size_t ff_idx = FF_GET_IDX(mnt);
	FATFS *fs = &fat_info[ff_idx].fs;

	res = f_mount(NULL, FF_LOG_PATH(ff_idx), 0);
	if (res == FR_OK) {
		memset(fs, 0, sizeof(*fs));
		fat_info[ff_idx].mounted = 0;
	}

	return _ff_err(res);
}

int fat_vfs_open(mount_t *mnt, vf_t *file, const char *path, int mode)
{
	FRESULT res;
	FIL *ff_file;
	size_t ff_idx = FF_GET_IDX(mnt);
	char ff_local_path[MAX_PATH + 1];

	snprintf(ff_local_path, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), path);

	ff_file = malloc(sizeof(*ff_file));
	if (ff_file == NULL) return -ERR_MEM;

	res = f_open(ff_file, ff_local_path, _ff_vfs_mode(mode));
	if (res == FR_OK)
		file->priv = ff_file;
	else
		free(ff_file);

	return _ff_err(res);
}

int fat_vfs_close(mount_t *mnt, vf_t *file)
{
	FRESULT res;
	FIL *ff_file = file->priv;

	res = f_close(ff_file);
	if (res == FR_OK)
		free(ff_file);

	return _ff_err(res);
}

int fat_vfs_unlink(mount_t *mnt, const char *path)
{
	FRESULT res;
	size_t ff_idx = FF_GET_IDX(mnt);
	char ff_local_path[MAX_PATH + 1];

	snprintf(ff_local_path, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), path);
	res = f_unlink(ff_local_path);
	return _ff_err(res);
}

int fat_vfs_rename(mount_t *mnt, const char *oldp, const char *newp)
{
	FRESULT res;
	size_t ff_idx = FF_GET_IDX(mnt);
	char ff_local_oldp[MAX_PATH + 1], ff_local_newp[MAX_PATH + 1];

	snprintf(ff_local_oldp, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), oldp);
	snprintf(ff_local_newp, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), newp);
	res = f_rename(oldp, newp);
	return _ff_err(res);
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
	size_t ff_idx = FF_GET_IDX(mnt);

	snprintf(ff_local_path, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), path);
	res = f_mkdir(ff_local_path);
	return _ff_err(res);
}

int fat_vfs_diropen(mount_t *mnt, vf_t *dir, const char *path)
{
	FRESULT res;
	DIR *dp;
	char ff_local_path[MAX_PATH + 1];
	size_t ff_idx = FF_GET_IDX(mnt);

	dp = malloc(sizeof(*dp));
	if (dp == NULL) return -ERR_MEM;

	snprintf(ff_local_path, MAX_PATH, "%s/%s", FF_LOG_PATH(ff_idx), path);
	res = f_opendir(dp, ff_local_path);

	if (res == FR_OK)
		dir->priv = dp;
	else
		free(dp);

	return _ff_err(res);
}

int fat_vfs_dirclose(mount_t *mnt, vf_t *dir)
{
	FRESULT res;
	DIR *dp = dir->priv;

	res = f_closedir(dp);

	if (res == FR_OK)
		free(dp);

	return _ff_err(res);
}

int fat_vfs_dirnext(mount_t *mnt, vf_t *dir, dirinf_t *next)
{
	FRESULT res;
	FILINFO fno;
	DIR *dp = dir->priv;

	res = f_readdir(dp, &fno);
	if (res != FR_OK || *fno.fname == '\0') {
		*next->path = '\0';
		return -ERR_NOTFOUND;
	}

	strcpy(next->path, fno.fname);

	if (fno.fattrib & AM_DIR)
		next->flags |= VFS_DIR;
	else
		next->flags |= VFS_FILE;

	if (fno.fattrib & AM_RDO)
		next->flags |= VFS_RO;
	else
		next->flags |= VFS_RW;

	return 0;
}

static const vfs_ops_t fat_ops = {
	.mount = fat_vfs_mount,
	.unmount = fat_vfs_unmount,

	.open = fat_vfs_open,
	.close = fat_vfs_close,
	.unlink = fat_vfs_unlink,
	.rename = fat_vfs_rename,

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
	int ff_idx = -1;
	mount_t mount = {
		.ops = &fat_ops,
		.caps = VFS_RW | VFS_CREATE,
	};

	for (int i = 0; i < FF_MAX_DISK; i++) {
		if (!fat_info[i].mounted) {
			ff_idx = i;
			break;
		}
	}

	if (ff_idx == -1) return -ERR_DEV;

	mount.priv = (void*)ff_idx;
	fat_info[ff_idx].disk_ops = disk_ops;
	return vfs_mount(drive, &mount);
}
