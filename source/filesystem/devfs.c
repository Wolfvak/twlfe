#include <string.h>

#include "devfs.h"
#include "err.h"
#include "vfs.h"

#define DEVFS_FIDX_SET(f,s)	do{(f)->priv = (void*)(s);}while(0)
#define DEVFS_FIDX_GET(f)	(((size_t)(f)->priv))

int devfs_vfs_mount(mount_t *mnt)
{
	/* initialize all file states to "not open" */
	devfs_t *dfs = mnt->priv;
	for (size_t i = 0; i < dfs->n_entries; i++)
		dfs->dev_entry[i].state = -1;
	return 0;
}

int devfs_vfs_unmount(mount_t *mnt)
{
	return 0;
}

int devfs_vfs_open(mount_t *mnt, file_t *file, const char *path, int mode)
{
	devfs_entry_t *dev_entry;
	devfs_t *dfs = mnt->priv;
	size_t idx;

	/* trying to create a file within devfs? don't even bother */
	if (mode & VFS_CREATE) return ERR_ARG;

	/* look for the desired file, return an error if not found
	 * yes, it's done with a dumb strcasecmp comparison
	 * deal with it */
	for (idx = 0; idx < dfs->n_entries; idx++) {
		if (!strcasecmp(path, dfs->dev_entry[idx].name)) break;
	}
	if (idx == dfs->n_entries) return ERR_NOTFOUND;

	dev_entry = &dfs->dev_entry[idx];

	/* file is already open */
	if (dev_entry->state >= 0) return ERR_BUSY;

	/* can't open the file in this mode */
	if ((dev_entry->mode & mode) != mode) return ERR_PERM;

	/* set the file as "opened" and reset the position */
	dev_entry->state = 0;
	dev_entry->pos = 0;

	/* mark the file entry index within the devfs */
	DEVFS_FIDX_SET(file, idx);

	return 0;
}

int devfs_vfs_close(mount_t *mnt, file_t *file)
{
	devfs_entry_t *dev_entry;
	devfs_t *dfs = mnt->priv;

	dev_entry = &dfs->dev_entry[DEVFS_FIDX_GET(file)];

	/* file is already not opened? */
	if (dev_entry->state < 0) return ERR_BUSY;

	/* mark as "not open" */
	dev_entry->state = -1;
	DEVFS_FIDX_SET(file, -1);

	return 0;
}

int devfs_vfs_unlink(mount_t *mnt, const char *path)
{
	/* removing files is not supported */
	return ERR_ARG;
}

off_t devfs_vfs_read(mount_t *mnt, file_t *file, void *buf, off_t size)
{
	devfs_t *dfs = mnt->priv;
	devfs_entry_t *dev_entry = &dfs->dev_entry[DEVFS_FIDX_GET(file)];
	off_t rb = size;

	/* clamp the position to bounds */
	if ((dev_entry->pos + rb) > dev_entry->size)
		rb = dev_entry->size - dev_entry->pos;

	/* read from the device and increase the position */
	rb = (dfs->dev_read)(dev_entry, buf, rb);
	dev_entry->pos += rb;
	return rb;
}

off_t devfs_vfs_write(mount_t *mnt, file_t *file, const void *buf, off_t size)
{
	/* basically the same thing as devfs_vfs_read */
	devfs_t *dfs = mnt->priv;
	devfs_entry_t *dev_entry = &dfs->dev_entry[DEVFS_FIDX_GET(file)];
	off_t wb = size;

	if ((dev_entry->pos + wb) > dev_entry->size)
		wb = dev_entry->size - dev_entry->pos;

	wb = (dfs->dev_write)(dev_entry, buf, wb);
	dev_entry->pos += wb;
	return wb;
}

off_t devfs_vfs_seek(mount_t *mnt, file_t *file, off_t off)
{
	devfs_t *dfs = mnt->priv;
	devfs_entry_t *dev_entry = &dfs->dev_entry[DEVFS_FIDX_GET(file)];

	/* seek (forwards or backwards, depends on off) and clamp down to bounds */
	/* TODO: should probably add overflow checks or something */
	off_t pos = dev_entry->pos + off;
	if (pos > dev_entry->size) {
		pos = dev_entry->size;
	} else if (pos < 0) {
		pos = 0;
	}
	dev_entry->pos = pos;

	return pos;
}

int devfs_vfs_mkdir(mount_t *mnt, const char *path)
{
	/* same as unlink */
	return ERR_ARG;
}

int devfs_vfs_diropen(mount_t *mnt, dir_t *dir, const char *path)
{
	/* only dir available is root, for now at least */
    if (strcmp(path, "")) return ERR_NOTFOUND;
    dir->priv = DEVFS_DIR_OPEN_POISON;
    return 0;
}

int devfs_vfs_dirclose(mount_t *mnt, dir_t *dir)
{
    if (dir->priv != DEVFS_DIR_OPEN_POISON) return ERR_ARG;
    dir->priv = NULL;
    return 0;
}

int devfs_vfs_dirnext_clr(mount_t *mnt, dir_t *dir, dirent_t *next)
{
	if (dir->priv != DEVFS_DIR_OPEN_POISON) return ERR_ARG;
	next->path = NULL;
	DEVFS_FIDX_SET(next, 0);
	return 0;
}

int devfs_vfs_dirnext(mount_t *mnt, dir_t *dir, dirent_t *next)
{
	devfs_t *dfs = mnt->priv;
    size_t idx;

    /* it doesn't really matter but it's here for correctness anyway */
    if (dir->priv != DEVFS_DIR_OPEN_POISON) return ERR_ARG;

    /* if the next path is null then we're at the start of dir loop
     * fetch the first file, otherwise fetch the next */
    if (next->path == NULL) {
        idx = 0;
    } else {
        idx = DEVFS_FIDX_GET(next) + 1;
    }

    /* past the last file, reset next and return an error */
    if (idx >= dfs->n_entries) {
    	devfs_vfs_dirnext_clr(mnt, dir, next);
        return ERR_NOTFOUND;
    }

    next->path = dfs->dev_entry[idx].name;
    DEVFS_FIDX_SET(next, idx);
    return 0;
}

static const vfs_ops_t devfs_ops = {
	.mount = devfs_vfs_mount,
	.unmount = devfs_vfs_unmount,

	.open = devfs_vfs_open,
	.close = devfs_vfs_close,
	.unlink = devfs_vfs_unlink,

	.read = devfs_vfs_read,
	.write = devfs_vfs_write,
	.seek = devfs_vfs_seek,

	.mkdir = devfs_vfs_mkdir,
	.diropen = devfs_vfs_diropen,
	.dirclose = devfs_vfs_dirclose,
	.dirnext = devfs_vfs_dirnext,
	.dirnext_clr = devfs_vfs_dirnext_clr,
};

int devfs_mount(char drive, devfs_t *devfs)
{
	const mount_t mount = {
		.ops = &devfs_ops,
		.info = {
			.size = 0,
			.label = devfs->label,
			.serial = devfs->serial,
		},
		.priv = devfs
	};

	return vfs_mount(drive, &mount);
}
