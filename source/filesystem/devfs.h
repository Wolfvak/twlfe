#ifndef DEVFS_H__
#define DEVFS_H__

#include <nds.h>

#include "err.h"
#include "vfs.h"

typedef struct devfs_entry_t {
	const char *name;
	int flags;
	off_t size;

	void *priv;
} devfs_entry_t;

typedef struct devfs_t {
	devfs_entry_t *dev_entry;
	size_t n_entries;

	const char *label;

	off_t (*dev_read)(devfs_entry_t *dev_entry, void *buf, off_t pos, off_t size);
	off_t (*dev_write)(devfs_entry_t *dev_entry, const void *buf, off_t pos, off_t size);

	void *priv;
} devfs_t;

int devfs_mount(char drive, devfs_t *devfs);

#endif /* DEVFS_H__ */
