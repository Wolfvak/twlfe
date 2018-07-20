#include <stdio.h>
#include <stdarg.h>
#include <nds.h>

#include "global.h"
#include "err.h"

#include "ui.h"
#include "vfs.h"

#include "vfs_glue.h"
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

int copy_file_ui(const char *path, const char *new)
{
	int nfd, ofd, res = 0;
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
		vfs_blk_op_t blk_copy;
		file_xfer xfer;

		blk_copy.fd = ofd;
		blk_copy.cb = copy_blkcb;
		blk_copy.min_blk_len = 1;
		blk_copy.max_blk_len = SIZE_KIB(256);
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

int move_file_ui(const char *old, const char *new, bool fallback)
{
	int res = vfs_rename(old, new);
	if (res == -ERR_UNSUPP && fallback) { /* fallback */
		res = copy_file_ui(old, new);
		if (IS_ERR(res)) return res; /* give up */
		res = vfs_unlink(old);
		if (IS_ERR(res)) return res; /* at least the file was copied, yay */
	}
	return res;
}

size_t path_basedir(char *path)
{
	char *path_s = path;

	while(*path) path++;
	path--;

	while(*path == '/') path--;
	while(*path != '/') path--;
	*(++path) = '\0';

	return (path - path_s);
}

int open_compound_path(int mode, const char *fmt, ...)
{
	char path[MAX_PATH + 1];
	va_list va;

	va_start(va, fmt);
	vsnprintf(path, MAX_PATH, fmt, va);
	va_end(va);

	return vfs_open(path, mode);
}

/* this should be a gcc intrinsic tbh... */
static inline size_t ctz64(uint64_t n) {
	size_t lower = n, upper = n >> 32;
	if (n == 0) return 0;
	switch(upper) {
		case 0:  return 31 - __builtin_clz(lower);
		default: return 63 - __builtin_clz(upper);
	}
}

static const char sizepre[] = {
	'B', 'K', 'M', 'G', 'T', 'P', 'E'
};

size_t size_format(char *out, off_t size)
{
	size_t mag, intp, decp;

	if (size < 0) {
		strcpy(out, "<0?");
		return 3;
	} else if (size < 1024) {
		return sprintf(out, "%d B", (size_t)size);
	}

	mag = (ctz64(size) / 10) * 10;

	intp = size >> mag;
	decp = (size - ((size >> mag) << mag)) >> (mag - 10);
	if (decp >= 100) decp /= 10;

	return sprintf(out, "%d.%d %ciB", (int)intp, (int)decp, sizepre[mag/10]);
}
