#include <nds.h>
#include <stdio.h>

#include "err.h"
#include "vfs.h"

#include "ui.h"
#include "vfs_blkop.h"

typedef struct {
	const char *path;
	const char *new;

	int oldfd;
	int newfd;

	off_t size;
	off_t current;
} file_xfer;

static int copy_blkcb(off_t blk_len, void *buf, void *priv)
{
	file_xfer *xfer = priv;
	off_t wb = vfs_write(xfer->newfd, buf, blk_len);
	if (IS_ERR(wb)) return wb;

	xfer->current += wb;
	ui_progress(xfer->current, xfer->size, "B", "Copying file...");
	return 0;
}

int copy_file_prog(const char *new, const char *path)
{
	vfs_blk_op_t blk_copy;
	int nfd, ofd, res = 0;
	file_xfer xfer;
	off_t osz;

	nfd = vfs_open(new, VFS_WO | VFS_CREATE);
	if (IS_ERR(nfd)) return nfd;

	ofd = vfs_open(path, VFS_RO);
	if (IS_ERR(ofd)) {
		vfs_close(nfd);
		return ofd;
	}

	osz = vfs_size(ofd);
	if (IS_ERR(osz)) {
		vfs_close(nfd);
		vfs_close(ofd);
		return osz;
	}

	if (osz != 0) {
		blk_copy.fd = ofd;
		blk_copy.cb = copy_blkcb;
		blk_copy.min_blk_len = 1;
		blk_copy.max_blk_len = 64 << 10;
		blk_copy.data_len = osz;
		blk_copy.priv = &xfer;

		xfer.path = path;
		xfer.new = new;
		xfer.oldfd = ofd;
		xfer.newfd = nfd;
		xfer.size = osz;
		xfer.current = 0;

		res = vfs_blk_read(&blk_copy);
		ui_progress(1, 0, NULL, NULL);
	}

	vfs_close(ofd);
	vfs_close(nfd);
	return res;
}
