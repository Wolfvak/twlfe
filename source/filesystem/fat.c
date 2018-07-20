#include <stdio.h>
#include <nds.h>

#include "global.h"

#include "err.h"
#include "vfs.h"

#include "fat.h"

#include "ff.h"

#define FF_LOG_PATH(x)	((char[]){'0' + (x), ':', '\0'})

typedef struct {
	const fat_disk_ops *dops;
	unsigned int drvn;
	char label[12];

	FATFS fs;
} fat_state;

static fat_state *states[FF_MAX_DISK] = {NULL};

const fat_disk_ops *ff_get_disk_ops(int disk)
{
	if (states[disk]) {
		return states[disk]->dops;
	}
	return NULL;
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
	static const u8 ff_err_ttbl[] = {
		[FR_OK] = 0,
		[FR_DISK_ERR] = ERR_IO,
		[FR_INT_ERR] = ERR_DEV,
		[FR_NOT_READY] = ERR_NOTREADY,
		[FR_NO_FILE] = ERR_NOTFOUND,
		[FR_NO_PATH] = ERR_NOTFOUND,
		[FR_INVALID_NAME] = ERR_ARG,
		[FR_DENIED] = ERR_ARG,
		[FR_EXIST] = ERR_ARG,
		[FR_INVALID_OBJECT] = ERR_MEM,
		[FR_WRITE_PROTECTED] = ERR_NOTREADY,
		[FR_INVALID_DRIVE] = ERR_ARG,
		[FR_NOT_ENABLED] = ERR_NOTREADY,
		[FR_NO_FILESYSTEM] = ERR_IO,
		[FR_NOT_ENOUGH_CORE] = ERR_MEM,
		[FR_TOO_MANY_OPEN_FILES] = ERR_MEM,
		[FR_INVALID_PARAMETER] = ERR_ARG,
	};
	return -ff_err_ttbl[err];
}

int fat_vfs_mount(mount_t *mnt)
{
	int res;
	fat_state *state = GET_PRIVDATA(mnt, fat_state*);

	states[state->drvn] = state;
	res = f_mount(&state->fs, FF_LOG_PATH(state->drvn), 1);

	if (res != FR_OK) {
		states[state->drvn] = NULL;
	} else {
		f_getlabel(FF_LOG_PATH(state->drvn), state->label, NULL);
	}

	return _ff_err(res);
}

int fat_vfs_unmount(mount_t *mnt)
{
	fat_state *state = GET_PRIVDATA(mnt, fat_state*);
	int res = f_mount(NULL, FF_LOG_PATH(state->drvn), 0);
	if (res == FR_OK) {
		states[state->drvn] = NULL;
		free(state);
		free(mnt);
	}

	return _ff_err(res);
}

int fat_vfs_ioctl(mount_t *mnt, int ctl, vfs_ioctl_t *data)
{
	fat_state *state = GET_PRIVDATA(mnt, fat_state*);
	FATFS *fs = &state->fs;

	switch(ctl) {
		default:
			return -ERR_ARG;

		case VFS_IOCTL_SIZE:
			data->size = (off_t)(fs->n_fatent - 2) * (off_t)(fs->csize) * FAT_SECT_SIZE;
			break;

		case VFS_IOCTL_LABEL:
			data->string = state->label;
			break;
	}
	return 0;
}

int fat_vfs_open(mount_t *mnt, vf_t *file, const char *path, int mode)
{
	int res;
	FIL *ff_file;
	char ff_lpath[MAX_PATH + 1];
	fat_state *state = GET_PRIVDATA(mnt, fat_state*);

	ff_file = malloc(sizeof(*ff_file));
	if (ff_file == NULL) return -ERR_MEM;

	snprintf(ff_lpath, MAX_PATH, "%s/%s", FF_LOG_PATH(state->drvn), path);
	res = f_open(ff_file, ff_lpath, _ff_vfs_mode(mode));
	if (res == FR_OK) {
		SET_PRIVDATA(file, ff_file);
	} else {
		free(ff_file);
	}

	return _ff_err(res);
}

int fat_vfs_close(mount_t *mnt, vf_t *file)
{
	int res;
	FIL *ff_file = GET_PRIVDATA(file, FIL*);

	res = f_close(ff_file);
	if (res == FR_OK) {
		SET_PRIVDATA(file, NULL);
		free(ff_file);
	}

	return _ff_err(res);
}

int fat_vfs_unlink(mount_t *mnt, const char *path)
{
	int res;
	size_t pathlen;
	char ff_lpath[MAX_PATH + 1];
	fat_state *state = GET_PRIVDATA(mnt, fat_state*);

	pathlen = snprintf(ff_lpath, MAX_PATH, "%s/%s", FF_LOG_PATH(state->drvn), path);
	if (ff_lpath[pathlen-1] == '/') ff_lpath[pathlen-1] = '\0';

	res = f_unlink(ff_lpath);
	return _ff_err(res);
}

int fat_vfs_rename(mount_t *mnt, const char *oldp, const char *newp)
{
	int res;
	size_t pathlen;
	char ff_lop[MAX_PATH + 1], ff_lnp[MAX_PATH + 1];
	fat_state *state = GET_PRIVDATA(mnt, fat_state*);

	pathlen = snprintf(ff_lop, MAX_PATH, "%s/%s", FF_LOG_PATH(state->drvn), oldp);
	if (ff_lop[pathlen-1] == '/') ff_lop[pathlen-1] = '\0';

	pathlen = snprintf(ff_lnp, MAX_PATH, "%s/%s", FF_LOG_PATH(state->drvn), newp);
	if (ff_lnp[pathlen-1] == '/') ff_lnp[pathlen-1] = '\0';

	res = f_rename(ff_lop, ff_lnp);
	return _ff_err(res);
}

off_t fat_vfs_read(mount_t *mnt, vf_t *file, void *buf, off_t size)
{
	int res;
	size_t br;
	FIL *ff_file = GET_PRIVDATA(file, FIL*);

	res = f_lseek(ff_file, file->pos);
	if (res != FR_OK) return _ff_err(res);

	f_read(ff_file, buf, size, &br);
	return br;
}

off_t fat_vfs_write(mount_t *mnt, vf_t *file, const void *buf, off_t size)
{
	int res;
	size_t wb;
	FIL *ff_file = GET_PRIVDATA(file, FIL*);

	res = f_lseek(ff_file, file->pos);
	if (res != FR_OK) return _ff_err(res);

	f_write(ff_file, buf, size, &wb);
	return wb;
}

off_t fat_vfs_size(mount_t *mnt, vf_t *file)
{
	FIL *ff_file = GET_PRIVDATA(file, FIL*);
	return f_size(ff_file);
}

int fat_vfs_mkdir(mount_t *mnt, const char *path)
{
	int res;
	size_t pathlen;
	char ff_lpath[MAX_PATH + 1];
	fat_state *state = GET_PRIVDATA(mnt, fat_state*);

	pathlen = snprintf(ff_lpath, MAX_PATH, "%s/%s", FF_LOG_PATH(state->drvn), path);
	if (ff_lpath[pathlen-1] == '/') ff_lpath[pathlen-1] = '\0';

	res = f_mkdir(ff_lpath);
	return _ff_err(res);
}

int fat_vfs_diropen(mount_t *mnt, vf_t *dir, const char *path)
{
	DIR *dp;
	int res;
	size_t pathlen;
	char ff_lpath[MAX_PATH + 1];
	fat_state *state = GET_PRIVDATA(mnt, fat_state*);

	dp = malloc(sizeof(*dp));
	if (dp == NULL) return -ERR_MEM;

	pathlen = snprintf(ff_lpath, MAX_PATH, "%s/%s", FF_LOG_PATH(state->drvn), path);
	if (ff_lpath[pathlen-1] == '/') ff_lpath[pathlen-1] = '\0';

	res = f_opendir(dp, ff_lpath);
	if (res == FR_OK) {
		SET_PRIVDATA(dir, dp);
	} else {
		free(dp);
	}

	return _ff_err(res);
}

int fat_vfs_dirclose(mount_t *mnt, vf_t *dir)
{
	FRESULT res;
	DIR *dp = GET_PRIVDATA(dir, DIR*);

	res = f_closedir(dp);
	if (res == FR_OK) {
		free(dp);
	}

	return _ff_err(res);
}

int fat_vfs_dirnext(mount_t *mnt, vf_t *dir, dirinf_t *next)
{
	int res;
	FILINFO fno;
	int flags = 0;
	DIR *dp = GET_PRIVDATA(dir, DIR*);

	res = f_readdir(dp, &fno);
	if (res != FR_OK || fno.fname[0] == '\0') {
		next->path[0] = '\0';
		next->flags = 0;
		return -ERR_NOTFOUND;
	}

	strcpy(next->path, fno.fname);
	if (fno.fattrib & AM_DIR) {
		strcat(next->path, "/");
	}

	flags |= (fno.fattrib & AM_DIR) ? VFS_DIR : VFS_FILE;
	flags |= (fno.fattrib & AM_RDO) ? VFS_RO : VFS_RW;

	next->flags = flags;
	return 0;
}

static const vfs_ops_t fat_ops = {
	.mount = fat_vfs_mount,
	.unmount = fat_vfs_unmount,
	.ioctl = fat_vfs_ioctl,

	.open = fat_vfs_open,
	.close = fat_vfs_close,

	.unlink = fat_vfs_unlink,
	.rename = fat_vfs_rename,

	.read = fat_vfs_read,
	.write = fat_vfs_write,
	.size = fat_vfs_size,

	.mkdir = fat_vfs_mkdir,
	.diropen = fat_vfs_diropen,
	.dirclose = fat_vfs_dirclose,
	.dirnext = fat_vfs_dirnext,
};

int fat_mount(char drive, const fat_disk_ops *disk_ops)
{
	int res;
	size_t idx;
	mount_t *mnt;
	fat_state *state;

	/* find a free spot */
	for (idx = 0; idx < FF_MAX_DISK; idx++) {
		if (states[idx] == NULL) break;
	}
	if (idx == FF_MAX_DISK) return -ERR_DEV;

	mnt = malloc(sizeof(*mnt));
	if (mnt == NULL) return -ERR_MEM;

	state = malloc(sizeof(*state));
	if (state == NULL) {
		free(mnt);
		return -ERR_MEM;
	}

	mnt->ops = &fat_ops;
	mnt->caps = VFS_RW | VFS_CREATE;
	SET_PRIVDATA(mnt, state);

	memset(state->label, 0, sizeof(state->label));
	state->drvn = idx;
	state->dops = disk_ops;

	res = vfs_mount(drive, mnt);
	if (IS_ERR(res)) {
		free(state);
		free(mnt);
	}

	return res;
}
