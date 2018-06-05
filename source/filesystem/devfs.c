#include <nds.h>

#include "err.h"
#include "vfs.h"

#include "devfs.h"

#define DEVFS_FIDX_SET(f,s)	do{(f)->priv = (void*)(s);}while(0)
#define DEVFS_FIDX_GET(f)	(((size_t)(f)->priv))

int devfs_vfs_mount(mount_t *mnt)
{
	return 0;
}

int devfs_vfs_unmount(mount_t *mnt)
{
	return 0;
}

int devfs_vfs_open(mount_t *mnt, vf_t *file, const char *path, int mode)
{
	devfs_entry_t *dev_entry;
	devfs_t *dfs = mnt->priv;
	size_t fidx;

	/* look for the desired file, return an error if not found
	 * yes, it's done with a dumb strcasecmp comparison
	 * deal with it */
	for (fidx = 0; fidx < dfs->n_entries; fidx++) {
		if (!strcasecmp(path, dfs->dev_entry[fidx].name)) break;
	}
	if (fidx == dfs->n_entries) return -ERR_NOTFOUND;

	dev_entry = &dfs->dev_entry[fidx];

	/* file is already open? */
	if (dev_entry->flags & VFS_OPEN) return -ERR_BUSY;

	/* tried to open the file with an invalid mode */
	if ((mode & dev_entry->flags) != mode) return -ERR_ARG;

	/* reset the position */
	dev_entry->flags |= VFS_OPEN;
	dev_entry->vf = file;

	/* mark the file entry index within the devfs */
	DEVFS_FIDX_SET(file, fidx);
	return 0;
}

int devfs_vfs_close(mount_t *mnt, vf_t *file)
{
	devfs_entry_t *dev_entry;
	devfs_t *dfs = mnt->priv;

	dev_entry = &dfs->dev_entry[DEVFS_FIDX_GET(file)];

	if (!(dev_entry->flags & VFS_OPEN)) return -ERR_ARG;

	/* mark as "not open" */
	dev_entry->flags &= ~VFS_OPEN;
	dev_entry->vf = NULL;
	DEVFS_FIDX_SET(file, -1);
	return 0;
}

off_t devfs_vfs_read(mount_t *mnt, vf_t *file, void *buf, off_t size)
{
	devfs_t *dfs = mnt->priv;
	devfs_entry_t *dev_entry = &dfs->dev_entry[DEVFS_FIDX_GET(file)];
	off_t rb = size;

	/* clamp the position to bounds */
	if ((file->pos + rb) > dev_entry->size)
		rb = dev_entry->size - file->pos;

	/* read from the device */
	rb = (dfs->dev_read)(dev_entry, buf, rb);
	return rb;
}

off_t devfs_vfs_write(mount_t *mnt, vf_t *file, const void *buf, off_t size)
{
	/* basically the same thing as devfs_vfs_read */
	devfs_t *dfs = mnt->priv;
	devfs_entry_t *dev_entry = &dfs->dev_entry[DEVFS_FIDX_GET(file)];
	off_t wb = size;

	if ((file->pos + wb) > dev_entry->size)
		wb = dev_entry->size - file->pos;

	wb = (dfs->dev_write)(dev_entry, buf, wb);
	return wb;
}

off_t devfs_vfs_size(mount_t *mnt, vf_t *file)
{
	devfs_t *dfs = mnt->priv;
	devfs_entry_t *dev_entry = &dfs->dev_entry[DEVFS_FIDX_GET(file)];
	return dev_entry->size;
}

int devfs_vfs_diropen(mount_t *mnt, vf_t *dir, const char *path)
{
	/* only dir available is root, for now at least */
    if (strcmp(path, "")) return -ERR_NOTFOUND;
    return 0;
}

int devfs_vfs_dirclose(mount_t *mnt, vf_t *dir)
{
    return 0;
}

int devfs_vfs_dirnext(mount_t *mnt, vf_t *dir, dirinf_t *next)
{
	devfs_t *dfs = mnt->priv;
    size_t idx = dir->pos;

    /* past the last file return an error */
    if (idx >= dfs->n_entries) return -ERR_NOTFOUND;

    strcpy(next->path, dfs->dev_entry[idx].name);
    next->flags = dfs->dev_entry[idx].flags;
    return 0;
}

static const vfs_ops_t devfs_ops = {
	.mount = devfs_vfs_mount,
	.unmount = devfs_vfs_unmount,

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
	mount_t mount = {
		.ops = &devfs_ops,
		.caps = VFS_RW,
		.info = {
			.size = 0,
			.label = devfs->label,
			.serial = "",
		},
		.priv = devfs,
	};

	return vfs_mount(drive, &mount);
}
