#include <stddef.h>
#include <string.h>

#include <nds.h>
#include <stdio.h>

#include "err.h"
#include "vfs.h"

static mount_t mounts[VFS_MOUNTPOINTS] = {};
static int mstate[VFS_MOUNTPOINTS];
/* ^ -1 if not mounted, otherwise number of open files on this fs */

#define VFS_MOUNTED(x) (mstate[(x)] >= 0)

static int __vfs_drvlet_to_idx(char drv)
{
	/* convert drive ASCII letter to array index */
	if (drv >= VFS_FIRSTMOUNT && drv <= VFS_LASTMOUNT) return drv - VFS_FIRSTMOUNT;
	return -1;
}

/*static char __vfs_idx_to_drvlet(int idx)
{
	// same as above but backwards
	if (idx >= 0 && idx <= VFS_MOUNTPOINTS) return idx + VFS_FIRSTMOUNT;
	return -1;
}*/

static const char *__vfs_get_localpath(const char *fpath)
{
	/* make sure the first letter corresponds to a mount */
	if (*fpath < VFS_FIRSTMOUNT || *fpath > VFS_LASTMOUNT) return NULL;

	/* verify the next two chars are ':' & '/' */
	if (*(++fpath) != ':') return NULL;
	if (*(++fpath) != '/') return NULL;

	/* skip any trailing '/' */
	while(*fpath == '/') fpath++;
	return fpath;
}

void __attribute__((constructor)) __vfs_ctor(void)
{
	memset(mounts, 0, sizeof(mounts));
	for (int i = 0; i < VFS_MOUNTPOINTS; i++) mstate[i] = -1;
}

int vfs_mount(char drive, const mount_t *mnt_data)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	int idx, res;

	if (mnt_data == NULL) return ERR_MEM;

	/* copy the mount data to the local mounts array
	 * make sure the first path char is valid
	 * make sure it's not already mounted */
	idx = __vfs_drvlet_to_idx(drive);
	if (idx < 0) return ERR_ARG;
	if (VFS_MOUNTED(idx)) return ERR_BUSY;

	mnt = &mounts[idx];
	memcpy(mnt, mnt_data, sizeof(*mnt));
	ops = mnt->ops;

	/* call the mount operation */
	res = ops->mount(mnt);
	if (IS_ERR(res)) {
		memset(mnt, 0, sizeof(*mnt));
	} else {
		mstate[idx] = 0;
	}

	return res;
}

int vfs_unmount(char drive)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	int idx, res;

	/* make sure the index is valid, the fs is mounted and no files are open */
	idx = __vfs_drvlet_to_idx(drive);
	if (idx < 0) return ERR_ARG;
	if (mstate[idx] != 0) return ERR_BUSY;

	mnt = &mounts[idx];
	ops = mnt->ops;

	/* call the unmount operation */
	res = ops->unmount(mnt);
	if (!IS_ERR(res))
		mstate[idx] = -1;

	return res;
}

off_t vfs_size(char drive)
{
	/* sanity checks, return value */
	int idx = __vfs_drvlet_to_idx(drive);
	if (idx < 0) return -1;
	if (!VFS_MOUNTED(idx)) return -1;
	return mounts[idx].info.size;
}

const char *vfs_serial(char drive)
{
	/* same as above */
	int idx = __vfs_drvlet_to_idx(drive);
	if (idx < 0) return NULL;
	if (!VFS_MOUNTED(idx)) return NULL;
	return mounts[idx].info.serial;
}

const char *vfs_label(char drive)
{
	/* ditto */
	int idx = __vfs_drvlet_to_idx(drive);
	if (idx < 0) return NULL;
	if (!VFS_MOUNTED(idx)) return NULL;
	return mounts[idx].info.label;
}

int vfs_open(file_t *file, const char *path, int mode)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	const char *lp;
	int idx, res;

	/* sanity checks */
	if (file == NULL || path == NULL) return ERR_MEM;

	idx = __vfs_drvlet_to_idx(*path);
	if (idx < 0) return ERR_DEV;
	if (!VFS_MOUNTED(idx)) return ERR_NOTREADY;

	/* valid path? */
	lp = __vfs_get_localpath(path);
	if (lp == NULL) return ERR_ARG;

	/* set up the file struct with the corresponding data */
	file->mode = mode;
	file->idx = idx;
	mnt = &mounts[idx];
	ops = mnt->ops;

	/* call the open operation, inc state if successful */
	res = ops->open(mnt, file, lp, mode);
	if (!IS_ERR(res))
		(mstate[idx])++;

	return res;
}

int vfs_close(file_t *file)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	int res;

	if (file == NULL) return ERR_MEM;

	mnt = &mounts[file->idx];
	ops = mnt->ops;

	/* call the close operation, dec state if successful */
	res = ops->close(mnt, file);
	if (!IS_ERR(res))
		(mstate[file->idx])--;

	return res;
}

int vfs_unlink(const char *path)
{
	const vfs_ops_t *ops;
	const char *lp;
	mount_t *mnt;
	int idx;

	if (path == NULL) return ERR_MEM;

	idx = __vfs_drvlet_to_idx(*path);
	if (idx < 0) return ERR_DEV;
	if (!VFS_MOUNTED(idx)) return ERR_NOTREADY;

	lp = __vfs_get_localpath(path);
	if (lp == NULL) return ERR_ARG;

	mnt = &mounts[idx];
	ops = mnt->ops;

	return ops->unlink(mnt, lp);
}

off_t vfs_read(file_t *file, void *buf, off_t size)
{
	const vfs_ops_t *ops;
	mount_t *mnt;

	/* sanity checks */
	if (file == NULL || buf == NULL) return ERR_MEM;
	if (size < 0) return ERR_ARG;

	mnt = &mounts[file->idx];
	ops = mnt->ops;

	/* if the file wasn't opened with read access
	 * don't even try */
	if (!(file->mode & VFS_RO)) return ERR_ARG;
	return ops->read(mnt, file, buf, size);
}

off_t vfs_write(file_t *file, const void *buf, off_t size)
{
	const vfs_ops_t *ops;
	mount_t *mnt;

	/* same as above, but checks write perms and calls write op */
	if (file == NULL || buf == NULL) return ERR_MEM;
	if (size < 0) return ERR_ARG;

	mnt = &mounts[file->idx];
	ops = mnt->ops;

	if (!(file->mode & VFS_WO)) return ERR_ARG;
	return ops->write(mnt, file, buf, size);
}

off_t vfs_seek(file_t *file, off_t off)
{
	const vfs_ops_t *ops;
	mount_t *mnt;

	if (file == NULL) return ERR_MEM;

	mnt = &mounts[file->idx];
	ops = mnt->ops;

	return ops->seek(mnt, file, off);
}

int vfs_mkdir(const char *path)
{
	const vfs_ops_t *ops;
	const char *lp;
	mount_t *mnt;
	int idx;

	if (path == NULL) return ERR_MEM;

	idx = __vfs_drvlet_to_idx(*path);
	if (idx < 0) return ERR_DEV;
	if (!VFS_MOUNTED(idx)) return ERR_NOTREADY;

	lp = __vfs_get_localpath(path);
	if (lp == NULL) return ERR_ARG;

	mnt = &mounts[idx];
	ops = mnt->ops;

	return ops->mkdir(mnt, lp);
}

int vfs_diropen(dir_t *dir, const char *path)
{
	const vfs_ops_t *ops;
	const char *lp;
	mount_t *mnt;
	int idx, res;

	if (dir == NULL || path == NULL) return ERR_MEM;

	idx = __vfs_drvlet_to_idx(*path);
	if (idx < 0) return ERR_DEV;
	if (!VFS_MOUNTED(idx)) return ERR_NOTREADY;

	lp = __vfs_get_localpath(path);
	if (lp == NULL) return ERR_ARG;

	dir->idx = idx;
	mnt = &mounts[idx];
	ops = mnt->ops;

	/* same as open, but with diropen instead */
	res = ops->diropen(mnt, dir, lp);
	if (!IS_ERR(res))
		(mstate[idx])++;

	return res;
}

int vfs_dirclose(dir_t *dir)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	int res;

	if (dir == NULL) return ERR_MEM;

	mnt = &mounts[dir->idx];
	ops = mnt->ops;

	/* ditto, with dirclose */
	res = ops->dirclose(mnt, dir);
	if (!IS_ERR(res))
		(mstate[dir->idx])--;

	return res;
}

int vfs_dirnext_clr(dir_t *dir, dirent_t *next)
{
	const vfs_ops_t *ops;
	mount_t *mnt;

	if (dir == NULL || next == NULL) return ERR_MEM;

	mnt = &mounts[dir->idx];
	ops = mnt->ops;

	/* reset the directory entity
	 * next->path may be dynamically allocated,
	 * so this is pretty fs specific */
	return ops->dirnext_clr(mnt, dir, next);
}

int vfs_dirnext(dir_t *dir, dirent_t *next)
{
	const vfs_ops_t *ops;
	mount_t *mnt;

	if (dir == NULL || next == NULL) return ERR_MEM;

	mnt = &mounts[dir->idx];
	ops = mnt->ops;

	/* 'jump' to the next directory */
	return ops->dirnext(mnt, dir, next);
}
