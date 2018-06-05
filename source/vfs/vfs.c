#include "err.h"
#include "vfs.h"
#include "vfs_help.h"

#include <string.h>

static mount_t mounts[VFS_MOUNTPOINTS];
static vf_t vfds[MAX_FILES];
static int vfds_c;

static mount_t *_vfs_mount(char drv)
{
	int idx = _vfs_drvlet_to_idx(drv);
	return (idx < 0) ? NULL : &mounts[idx];
}

static int _vfs_mounted(char drv)
{
	int idx = _vfs_drvlet_to_idx(drv);
	return (idx < 0) ? 0 : (mounts[idx].acts >= 0);
}

static int _vfs_actives(char drv)
{
	int idx = _vfs_drvlet_to_idx(drv);
	return (idx < 0) ? -1 : mounts[idx].acts;
}

static void _vfs_reset_mount(char drv)
{
	mount_t *mnt = _vfs_mount(drv);
	memset(mnt, 0, sizeof(*mnt));
	mnt->acts = -1;
}

/* TODO: better fetch method */
static vf_t *_vfs_open_vfd(int *fd)
{
	int i;
	if (vfds_c == MAX_FILES) return NULL;
	for (i = 0; i < MAX_FILES; i++) {
		if (!_vfs_ent_opened(&vfds[i])) break;
	}
	vfds_c++;
	*fd = i;
	return &vfds[i];
}

static void _vfs_close_vfd(int fd)
{
	memset(&vfds[fd], 0, sizeof(*vfds));
	vfds_c--;
}

static inline int _vfs_valid_vfd(int fd)
{
	return ((fd >= 0) && (fd < MAX_FILES));
}

void __attribute__((constructor)) __vfs_ctor(void)
{
	memset(vfds, 0, sizeof(vfds));
	vfds_c = 0;
	for (int i = VFS_FIRSTMOUNT; i < VFS_LASTMOUNT; i++)
		_vfs_reset_mount(i);
}

int vfs_state(char drive)
{
	if (!_vfs_mounted(drive)) return -ERR_ARG;
	return _vfs_actives(drive);
}

int vfs_mount(char drive, const mount_t *mnt_data)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	int res;

	if (mnt_data == NULL || mnt_data->ops == NULL) return -ERR_MEM;

	/* copy the mount data to the local mounts array
	 * make sure the first path char is valid
	 * make sure it's not already mounted */
	if (_vfs_mounted(drive)) return -ERR_BUSY;
	mnt = _vfs_mount(drive);

	memcpy(mnt, mnt_data, sizeof(*mnt));
	ops = mnt->ops;

	/* call the mount operation */
	res = ops->mount(mnt);
	if (IS_ERR(res))
		_vfs_reset_mount(drive);
	else
		mnt->acts = 0;

	return res;
}

int vfs_unmount(char drive)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	int res;

	/* make sure the index is valid, the fs is mounted and no files are open */
	if (!_vfs_mounted(drive)) return -ERR_NOTREADY;
	if (_vfs_actives(drive) > 0) return -ERR_BUSY;
	mnt = _vfs_mount(drive);
	ops = mnt->ops;

	/* call the unmount operation */
	if (ops->unmount == NULL) return -ERR_UNSUPP;
	res = ops->unmount(mnt);
	if (!IS_ERR(res))
		_vfs_reset_mount(drive);

	return res;
}

int vfs_open(const char *path, int mode)
{
	const vfs_ops_t *ops;
	const char *lp;
	mount_t *mnt;
	vf_t *file;
	int res, fd;

	/* sanity checks */
	if (path == NULL) return -ERR_MEM;
	if (!_vfs_mounted(*path)) return -ERR_NOTREADY;

	/* valid path? */
	lp = _vfs_get_lpath(path);
	if (!_vfs_check_lpath(lp)) return -ERR_ARG;

	mnt = _vfs_mount(*path);
	ops = mnt->ops;
	if (ops->open == NULL) return -ERR_UNSUPP;
	if ((mode & mnt->caps) != mode) return -ERR_ARG;

	/* get a free file descriptor */
	file = _vfs_open_vfd(&fd);
	if (file == NULL) return -ERR_MEM;

	/* set up the file structure with the corresponding data */
	file->flags = VFS_OPEN | VFS_FILE | mode;
	file->mnt = mnt;

	/* call the open operation, inc actives if successful */
	res = ops->open(mnt, file, lp, mode);
	if (IS_ERR(res)) {
		_vfs_close_vfd(fd);
	} else {
		(mnt->acts)++;
		res = fd;
	}

	return res;
}

int vfs_close(int fd)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	vf_t *file;
	int res;

	if (!_vfs_valid_vfd(fd)) return -ERR_ARG;

	file = &vfds[fd];
	if (!_vfs_ent_opened(file) || !_vfs_ent_file(file)) return -ERR_NOTREADY;

	mnt = file->mnt;
	ops = mnt->ops;
	if (ops->close == NULL) return -ERR_UNSUPP;

	/* call the close operation, dec state if successful */
	res = ops->close(mnt, file);
	if (!IS_ERR(res)) {
		_vfs_close_vfd(fd);
		(mnt->acts)--;
	}

	return res;
}

int vfs_unlink(const char *path)
{
	const vfs_ops_t *ops;
	const char *lp;
	mount_t *mnt;

	if (path == NULL) return -ERR_MEM;
	if (!_vfs_mounted(*path)) return -ERR_NOTREADY;

	lp = _vfs_get_lpath(path);
	if (!_vfs_check_lpath(lp)) return -ERR_ARG;

	mnt = _vfs_mount(*path);
	ops = mnt->ops;
	if (ops->unlink == NULL) return -ERR_UNSUPP;

	return ops->unlink(mnt, lp);
}

int vfs_rename(const char *oldp, const char *newp)
{
	const vfs_ops_t *ops;
	const char *lop, *lnp;
	mount_t *mnt;

	if (oldp == NULL || newp == NULL) return -ERR_MEM;
	if (*oldp != *newp) return -ERR_UNSUPP;

	if (!_vfs_mounted(*oldp)) return -ERR_NOTREADY;

	lop = _vfs_get_lpath(oldp);
	lnp = _vfs_get_lpath(newp);
	if (!_vfs_check_lpath(lop) || !_vfs_check_lpath(lnp)) return -ERR_ARG;

	mnt = _vfs_mount(*oldp);
	ops = mnt->ops;
	if (ops->rename == NULL) return -ERR_UNSUPP;

	return ops->rename(mnt, lop, lnp);
}

off_t vfs_read(int fd, void *buf, off_t size)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	vf_t *file;

	/* sanity checks */
	if (!_vfs_valid_vfd(fd) || size <= 0) return -ERR_ARG;
	if (buf == NULL) return -ERR_MEM;

	file = &vfds[fd];
	if (!_vfs_ent_opened(file) || !_vfs_ent_file(file)) return -ERR_NOTREADY;

	mnt = file->mnt;
	ops = mnt->ops;
	if (ops->read == NULL) return -ERR_UNSUPP;

	/* if the file wasn't opened with read access
	 * don't even try */
	if (!_vfs_ent_readable(file)) return -ERR_ARG;
	return ops->read(mnt, file, buf, size);
}

off_t vfs_write(int fd, const void *buf, off_t size)
{
	/* same as above, but with write */
	const vfs_ops_t *ops;
	mount_t *mnt;
	vf_t *file;

	if (buf == NULL) return -ERR_MEM;
	if (!_vfs_valid_vfd(fd) || size <= 0) return -ERR_ARG;

	file = &vfds[fd];
	if (!_vfs_ent_opened(file) || !_vfs_ent_file(file)) return -ERR_NOTREADY;

	mnt = file->mnt;
	ops = mnt->ops;
	if (ops->write == NULL) return -ERR_UNSUPP;

	if (!_vfs_ent_writable(file)) return -ERR_ARG;
	return ops->write(mnt, file, buf, size);
}

off_t vfs_seek(int fd, off_t off)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	vf_t *file;

	if (!_vfs_valid_vfd(fd)) return -ERR_MEM;

	file = &vfds[fd];
	if (!_vfs_ent_opened(file) || !_vfs_ent_file(file)) return -ERR_NOTREADY;

	mnt = file->mnt;
	ops = mnt->ops;
	if (ops->seek == NULL) return -ERR_UNSUPP;

	return ops->seek(mnt, file, off);
}

off_t vfs_size(int fd)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	vf_t *file;

	if (!_vfs_valid_vfd(fd)) return -ERR_MEM;

	file = &vfds[fd];
	if (!_vfs_ent_opened(file) || !_vfs_ent_file(file)) return -ERR_NOTREADY;

	mnt = file->mnt;
	ops = mnt->ops;
	if (ops->size == NULL) return -ERR_UNSUPP;

	return ops->size(mnt, file);
}

int vfs_mkdir(const char *path)
{
	const vfs_ops_t *ops;
	const char *lp;
	mount_t *mnt;

	if (path == NULL) return -ERR_MEM;
	if (!_vfs_mounted(*path)) return -ERR_NOTREADY;

	lp = _vfs_get_lpath(path);
	if (!_vfs_check_lpath(lp)) return -ERR_ARG;

	mnt = _vfs_mount(*path);
	ops = mnt->ops;
	if (ops->mkdir == NULL) return -ERR_UNSUPP;

	return ops->mkdir(mnt, lp);
}

int vfs_diropen(const char *path)
{
	const vfs_ops_t *ops;
	const char *lp;
	mount_t *mnt;
	int res, dd;
	vf_t *dir;

	/* same as open, but with diropen instead */
	if (path == NULL) return -ERR_MEM;
	if (!_vfs_mounted(*path)) return -ERR_NOTREADY;

	lp = _vfs_get_lpath(path);
	if (!_vfs_check_lpath(lp)) return -ERR_ARG;

	mnt = _vfs_mount(*path);
	ops = mnt->ops;
	if (ops->diropen == NULL) return -ERR_UNSUPP;

	dir = _vfs_open_vfd(&dd);
	if (dir == NULL) return -ERR_MEM;

	dir->mnt = mnt;
	dir->flags = VFS_OPEN | VFS_DIR;

	res = ops->diropen(mnt, dir, lp);
	if (IS_ERR(res)) {
		_vfs_close_vfd(dd);
	} else {
		(mnt->acts)++;
		res = dd;
	}

	return res;
}

int vfs_dirclose(int dd)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	vf_t *dir;
	int res;

	if (!_vfs_valid_vfd(dd)) return -ERR_MEM;

	dir = &vfds[dd];
	if (!_vfs_ent_opened(dir) || !_vfs_ent_dir(dir)) return -ERR_ARG;

	mnt = dir->mnt;
	ops = mnt->ops;
	if (ops->dirclose == NULL) return -ERR_UNSUPP;

	/* ditto, with dirclose */
	res = ops->dirclose(mnt, dir);
	if (!IS_ERR(res))
		(mnt->acts)--;

	return res;
}

int vfs_dirnext(int dd, dirinf_t *next)
{
	const vfs_ops_t *ops;
	mount_t *mnt;
	vf_t *dir;

	if (next == NULL) return -ERR_MEM;
	if (!_vfs_valid_vfd(dd)) return -ERR_ARG;

	dir = &vfds[dd];
	if (!_vfs_ent_opened(dir) || !_vfs_ent_dir(dir)) return -ERR_ARG;

	mnt = dir->mnt;
	ops = mnt->ops;
	if (ops->dirnext == NULL) return -ERR_UNSUPP;

	/* 'jump' to the next directory */
	return ops->dirnext(mnt, dir, next);
}
