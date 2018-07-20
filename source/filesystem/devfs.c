#include <nds.h>

#include "global.h"
#include "err.h"

#include "vfs.h"

#include "devfs.h"

int devfs_vfs_mount(mount_t *mnt)
{
	return 0;
}

int devfs_vfs_unmount(mount_t *mnt)
{
	free(mnt);
	return 0;
}

int devfs_vfs_ioctl(mount_t *mnt, int ctl, vfs_ioctl_t *data)
{
	devfs_t *dfs = GET_PRIVDATA(mnt, devfs_t*);
	switch(ctl) {
		default:
			return -ERR_ARG;

		case VFS_IOCTL_SIZE:
			data->size = 0;
			break;

		case VFS_IOCTL_LABEL:
			data->string = dfs->label;
			break;
	}
	return 0;
}

int devfs_vfs_open(mount_t *mnt, vf_t *file, const char *path, int mode)
{
	devfs_t *dfs = GET_PRIVDATA(mnt, devfs_t*);
	devfs_entry_t *dev_entry;
	size_t fidx;

	/*
	 * look for the desired file, return an error if not found
	 * yes, it's currently done with a dumb strcasecmp comparison
	 * deal with it
	 */
	for (fidx = 0; fidx < dfs->n_entries; fidx++) {
		if (!strcasecmp(path, dfs->dev_entry[fidx].name)) break;
	}
	if (fidx == dfs->n_entries) return -ERR_NOTFOUND;

	dev_entry = &dfs->dev_entry[fidx];

	/* check permissions */
	if ((dev_entry->flags & mode) != mode) return -ERR_ARG;

	/* mark the file entry index */
	SET_PRIVDATA(file, fidx);
	return 0;
}

int devfs_vfs_close(mount_t *mnt, vf_t *file)
{
	return 0;
}

off_t devfs_vfs_read(mount_t *mnt, vf_t *file, void *buf, off_t size)
{
	devfs_t *dfs = GET_PRIVDATA(mnt, devfs_t*);
	devfs_entry_t *dev_entry = &dfs->dev_entry[GET_PRIVDATA(file, size_t)];
	off_t rb = size;

	/* clamp the position to bounds */
	if ((file->pos + rb) > dev_entry->size)
		rb = dev_entry->size - file->pos;

	/* read from the device */
	rb = (dfs->dev_read)(dev_entry, buf, file->pos, rb);
	return rb;
}

off_t devfs_vfs_write(mount_t *mnt, vf_t *file, const void *buf, off_t size)
{
	/* basically the same thing as devfs_vfs_read */
	devfs_t *dfs = GET_PRIVDATA(mnt, devfs_t*);
	devfs_entry_t *dev_entry = &dfs->dev_entry[GET_PRIVDATA(file, size_t)];
	off_t wb = size;

	if ((file->pos + wb) > dev_entry->size)
		wb = dev_entry->size - file->pos;

	wb = (dfs->dev_write)(dev_entry, buf, file->pos, wb);
	return wb;
}

off_t devfs_vfs_size(mount_t *mnt, vf_t *file)
{
	devfs_t *dfs = GET_PRIVDATA(mnt, devfs_t*);
	devfs_entry_t *dev_entry = &dfs->dev_entry[GET_PRIVDATA(file, size_t)];
	return dev_entry->size;
}

int devfs_vfs_diropen(mount_t *mnt, vf_t *dir, const char *path)
{
	/* subdirs are currently not supported */
	if (strcmp(path, "/")) return -ERR_NOTFOUND;
	SET_PRIVDATA(dir, 1);
	return 0;
}

int devfs_vfs_dirclose(mount_t *mnt, vf_t *dir)
{
	return 0;
}

int devfs_vfs_dirnext(mount_t *mnt, vf_t *dir, dirinf_t *next)
{
	devfs_t *dfs = GET_PRIVDATA(mnt, devfs_t*);
    size_t idx = dir->pos;

    /* past the last file - return an error */
    if (idx >= dfs->n_entries) return -ERR_NOTFOUND;
    strcpy(next->path, &(dfs->dev_entry[idx].name)[1]);
    next->flags = dfs->dev_entry[idx].flags;
    return 0;
}

static const vfs_ops_t devfs_ops = {
	.mount = devfs_vfs_mount,
	.unmount = devfs_vfs_mount,
	.ioctl = devfs_vfs_ioctl,

	.open = devfs_vfs_open,
	.close = devfs_vfs_close,

	.unlink = NULL,
	.rename = NULL,

	.read = devfs_vfs_read,
	.write = devfs_vfs_write,
	.size = devfs_vfs_size,

	.mkdir = NULL,
	.diropen = devfs_vfs_diropen,
	.dirclose = devfs_vfs_dirclose,
	.dirnext = devfs_vfs_dirnext,
};

int devfs_mount(char drive, devfs_t *devfs)
{
	int res;
	mount_t *mnt = malloc(sizeof(*mnt));
	if (mnt == NULL) return -ERR_MEM;

	mnt->ops = &devfs_ops;
	mnt->caps = VFS_RW;
	SET_PRIVDATA(mnt, devfs);

	res = vfs_mount(drive, mnt);
	if (IS_ERR(res)) {
		free(mnt);
	}

	return res;
}
