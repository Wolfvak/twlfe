#include <nds.h>

#include "global.h"
#include "err.h"

#include "vfs.h"

#include "vfd.h"

#define VFS_CALL_OP(mnt, op, ...) ({ \
	const vfs_ops_t *o = (mnt)->ops; \
	(UNLIKELY(o->op == NULL)) ?      \
	-ERR_UNSUPP : o->op(__VA_ARGS__);})

static struct {
	mount_t *mount;
	int open_handles;
} mount_state[VFS_MOUNTPOINTS];

static int mounted_filesystems;

static inline int _vfs_drvlet_to_idx(int drv)
{
	if (VFS_DRVVALID(drv)) return (drv - VFS_FIRSTMOUNT);
	return -1;
}

static inline mount_t *_vfs_mount(int drv)
{
	int idx = _vfs_drvlet_to_idx(drv);
	return (idx < 0) ? NULL : mount_state[idx].mount;
}

static inline int _vfs_mounted(int drv)
{
	int idx = _vfs_drvlet_to_idx(drv);
	return (idx < 0) ? 0 : (mount_state[idx].open_handles >= 0);
}

static inline int _vfs_actives(int drv)
{
	int idx = _vfs_drvlet_to_idx(drv);
	return (idx < 0) ? -1 : mount_state[idx].open_handles;
}

static inline void _vfs_actives_inc(int idx)
{
	(mount_state[idx].open_handles)++;
}

static inline void _vfs_actives_dec(int idx)
{
	(mount_state[idx].open_handles)--;
}

static inline void _vfs_reset_mount(int drv)
{
	int idx = _vfs_drvlet_to_idx(drv);
	if (idx < 0) return;

	mount_state[idx].mount = NULL;
	mount_state[idx].open_handles = -1;
}

static inline void _vfs_set_mount(int drv, mount_t *mnt)
{
	int idx = _vfs_drvlet_to_idx(drv);
	if (idx < 0) return;

	mount_state[idx].mount = mnt;
	mount_state[idx].open_handles = 0;
}

static void __attribute__((constructor)) __vfs_ctor(void)
{
	for (int i = VFS_FIRSTMOUNT; i <= VFS_LASTMOUNT; i++)
		_vfs_reset_mount(i);
	mounted_filesystems = 0;
}


static const char *_vfs_get_lpath(const char *fp)
{
	/* make sure the first letter corresponds to a mount */
	if (*fp < VFS_FIRSTMOUNT || *fp > VFS_LASTMOUNT) return NULL;
	/* verify the next two chars are ':' & '/' */
	if (*(++fp) != ':') return NULL;
	if (*(++fp) != '/') return NULL;
	return fp;
}

static int _vfs_check_lpath(const char *lp)
{
	if (lp == NULL || strlen(lp) > MAX_PATH) return 0;
	return 1;
}



int vfs_count(void)
{
	return mounted_filesystems;
}

int vfs_state(int drive)
{
	return _vfs_actives(drive);
}

int vfs_mount(int drive, mount_t *mnt_info)
{
	int res;

	if (mnt_info == NULL || mnt_info->ops == NULL) return -ERR_MEM;
	if (_vfs_mounted(drive)) return -ERR_BUSY;

	/* call the mount operation, cleanup if it fails */
	res = VFS_CALL_OP(mnt_info, mount, mnt_info);
	if (IS_ERR(res)) {
		_vfs_reset_mount(drive);
	} else {
		_vfs_set_mount(drive, mnt_info);
		mounted_filesystems++;
	}

	return res;
}

int vfs_unmount(int drive)
{
	int res;
	mount_t *mnt;

	/* make sure the index is valid, the fs is mounted and no files are open */
	if (!_vfs_mounted(drive)) return -ERR_NOTREADY;
	if (_vfs_actives(drive) > 0) return -ERR_BUSY;
	mnt = _vfs_mount(drive);

	/* call the unmount operation */
	res = VFS_CALL_OP(mnt, unmount, mnt);
	if (!IS_ERR(res)) {
		_vfs_reset_mount(drive);
		mounted_filesystems--;
	}

	return res;
}

int vfs_ioctl(int drive, int ctl, vfs_ioctl_t *data)
{
	mount_t *mnt;

	if (!_vfs_mounted(drive)) return -ERR_NOTREADY;
	mnt = _vfs_mount(drive);

	return VFS_CALL_OP(mnt, ioctl, mnt, ctl, data);
}

int vfs_open(const char *path, int mode)
{
	vf_t *file;
	mount_t *mnt;
	const char *lp;

	int res, fd, drv;

	/* sanity checks */
	if (path == NULL) return -ERR_MEM;

	drv = *path;
	if (!_vfs_mounted(drv)) return -ERR_NOTREADY;

	/* valid path? */
	lp = _vfs_get_lpath(path);
	if (!_vfs_check_lpath(lp)) return -ERR_ARG;

	mnt = _vfs_mount(drv);
	if ((mnt->caps & mode) != mode) return -ERR_ARG;

	/* get a free file descriptor */
	fd = vfd_retrieve();
	if (fd < 0) return -ERR_MEM;

	/* set up the file structure with the corresponding data */
	file = vfd_get(fd);
	file->mnt = mnt;
	file->idx = _vfs_drvlet_to_idx(drv);
	file->pos = 0;
	file->flags = VFS_OPEN | VFS_FILE | mode;

	/* call the open operation, inc actives if successful */
	res = VFS_CALL_OP(mnt, open, mnt, file, lp, mode);
	if (IS_ERR(res)) {
		vfd_return(fd);
	} else {
		_vfs_actives_inc(file->idx);
		res = fd;
	}

	return res;
}

int vfs_close(int fd)
{
	int res;
	vf_t *file;
	mount_t *mnt;

	if (!vfd_valid_fd(fd)) return -ERR_ARG;

	file = vfd_get(fd);
	if (!_vf_opened(file) || !_vf_file(file)) return -ERR_NOTREADY;

	/* call the close operation, dec state if successful */
	mnt = file->mnt;
	res = VFS_CALL_OP(mnt, close, mnt, file);
	if (!IS_ERR(res)) {
		vfd_return(fd);
		_vfs_actives_dec(file->idx);
	}

	return res;
}

int vfs_unlink(const char *path)
{
	int drv;
	mount_t *mnt;
	const char *lp;

	if (path == NULL) return -ERR_MEM;

	drv = *path;
	if (!_vfs_mounted(drv)) return -ERR_NOTREADY;

	lp = _vfs_get_lpath(path);
	if (!_vfs_check_lpath(lp)) return -ERR_ARG;

	mnt = _vfs_mount(drv);
	return VFS_CALL_OP(mnt, unlink, mnt, lp);
}

int vfs_rename(const char *oldp, const char *newp)
{
	mount_t *mnt;
	int odrv, ndrv;
	const char *lop, *lnp;

	if (oldp == NULL || newp == NULL) return -ERR_MEM;

	/* rename only works within the same fs for obvious reasons */
	odrv = *oldp;
	ndrv = *newp;

	if (odrv != ndrv) return -ERR_UNSUPP;

	if (!_vfs_mounted(odrv)) return -ERR_NOTREADY;

	lop = _vfs_get_lpath(oldp);
	lnp = _vfs_get_lpath(newp);
	if (!_vfs_check_lpath(lop) || !_vfs_check_lpath(lnp)) return -ERR_ARG;

	mnt = _vfs_mount(odrv);
	return VFS_CALL_OP(mnt, rename, mnt, lop, lnp);
}

off_t vfs_read(int fd, void *buf, off_t size)
{
	mount_t *mnt;
	vf_t *file;
	off_t rb;

	/* sanity checks */
	if (!vfd_valid_fd(fd) || size < 0) return -ERR_ARG;
	if (buf == NULL) return -ERR_MEM;

	file = vfd_get(fd);
	if (!_vf_opened(file) || !_vf_file(file)) return -ERR_NOTREADY;

	/* don't even try if the file wasn't opened with read access */
	if (!_vf_readable(file)) return -ERR_ARG;

	if (size == 0) {
		return 0;
	}

	mnt = file->mnt;
	rb = VFS_CALL_OP(mnt, read, mnt, file, buf, size);
	if (!IS_ERR(rb)) {
		file->pos += rb;
	}

	return rb;
}

off_t vfs_write(int fd, const void *buf, off_t size)
{
	/* same as above but with write */
	mount_t *mnt;
	vf_t *file;
	off_t wb;

	if (!vfd_valid_fd(fd) || size < 0) return -ERR_ARG;
	if (buf == NULL) return -ERR_MEM;

	file = vfd_get(fd);
	if (!_vf_opened(file) || !_vf_file(file)) return -ERR_NOTREADY;
	if (!_vf_writable(file)) return -ERR_ARG;

	if (size == 0) {
		return 0;
	}

	mnt = file->mnt;
	wb = VFS_CALL_OP(mnt, write, mnt, file, buf, size);
	if (!IS_ERR(wb)) {
		file->pos += wb;
	}
	return wb;
}

off_t vfs_seek(int fd, off_t off, int whence)
{
	vf_t *file;
	off_t pos, size;

	if (!vfd_valid_fd(fd)) return -ERR_MEM;

	file = vfd_get(fd);
	if (!_vf_opened(file) || !_vf_file(file)) return -ERR_NOTREADY;

	size = vfs_size(fd);
	if (IS_ERR(size)) return size;

	switch(whence) {
		case SEEK_SET:
			pos = off;
			break;

		case SEEK_CUR:
			pos = file->pos + off;
			break;

		case SEEK_END:
			pos = size + off;
			break;

		default:
			return -ERR_ARG;
	}

	if (pos > size) {
		pos = size;
	} else if (pos < 0) {
		pos = 0;
	}

	file->pos = pos;
	return pos;
}

off_t vfs_size(int fd)
{
	mount_t *mnt;
	vf_t *file;

	if (!vfd_valid_fd(fd)) return -ERR_MEM;

	file = vfd_get(fd);
	if (!_vf_opened(file) || !_vf_file(file)) return -ERR_NOTREADY;

	mnt = file->mnt;
	return VFS_CALL_OP(mnt, size, mnt, file);
}

int vfs_mkdir(const char *path)
{
	mount_t *mnt;
	const char *lp;
	int drv;

	if (path == NULL) return -ERR_MEM;

	drv = *path;
	if (!_vfs_mounted(drv)) return -ERR_NOTREADY;

	lp = _vfs_get_lpath(path);
	if (!_vfs_check_lpath(lp)) return -ERR_ARG;

	mnt = _vfs_mount(drv);
	return VFS_CALL_OP(mnt, mkdir, mnt, lp);
}

int vfs_diropen(const char *path)
{
	const char *lp;
	mount_t *mnt;
	int res, dd, drv;
	vf_t *dir;

	/* same as open, but with diropen instead */
	if (path == NULL) return -ERR_MEM;
	if (!_vfs_mounted(*path)) return -ERR_NOTREADY;

	lp = _vfs_get_lpath(path);
	if (!_vfs_check_lpath(lp)) return -ERR_ARG;

	drv = *path;
	mnt = _vfs_mount(drv);

	dd = vfd_retrieve();
	if (dd < 0) return -ERR_MEM;

	dir = vfd_get(dd);
	dir->mnt = mnt;
	dir->idx = _vfs_drvlet_to_idx(drv);
	dir->pos = 0;
	dir->flags = VFS_OPEN | VFS_DIR;

	res = VFS_CALL_OP(mnt, diropen, mnt, dir, lp);
	if (IS_ERR(res)) {
		vfd_return(dd);
	} else {
		_vfs_actives_inc(dir->idx);
		res = dd;
	}

	return res;
}

int vfs_dirclose(int dd)
{
	mount_t *mnt;
	vf_t *dir;
	int res;

	if (!vfd_valid_fd(dd)) return -ERR_MEM;

	dir = vfd_get(dd);
	if (!_vf_opened(dir) || !_vf_dir(dir)) return -ERR_ARG;

	/* same as close, but with dirclose */
	mnt = dir->mnt;
	res = VFS_CALL_OP(mnt, dirclose, mnt, dir);
	if (!IS_ERR(res)) {
		vfd_return(dd);
		_vfs_actives_dec(dir->idx);
	}

	return res;
}

int vfs_dirnext(int dd, dirinf_t *next)
{
	mount_t *mnt;
	vf_t *dir;
	int ret;

	if (next == NULL) return -ERR_MEM;
	if (!vfd_valid_fd(dd)) return -ERR_ARG;

	dir = vfd_get(dd);
	if (!_vf_opened(dir) || !_vf_dir(dir)) return -ERR_ARG;

	mnt = dir->mnt;

	/* 'jump' to the next directory */
	ret = VFS_CALL_OP(mnt, dirnext, mnt, dir, next);
	if (!IS_ERR(ret)) {
		dir->pos++;
	}

	return ret;
}


/* simple ioctl wrappers */
off_t vfs_ioctl_size(int drive)
{
	int res;
	vfs_ioctl_t io;

	res = vfs_ioctl(drive, VFS_IOCTL_SIZE, &io);
	if (IS_ERR(res)) return res;
	return io.size;
}

const char *vfs_ioctl_label(int drive)
{
	int res;
	vfs_ioctl_t io;

	res = vfs_ioctl(drive, VFS_IOCTL_LABEL, &io);
	if (IS_ERR(res)) return "Invalid label";
	return io.string;
}
