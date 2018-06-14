#include <nds.h>
#include <stdio.h>
#include <stdarg.h>

#include "err.h"
#include "vfs.h"

#include "ui.h"
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

const char *path_basename(const char *fpath)
{
	if (fpath == NULL) return NULL;
	while(*fpath) fpath++;
	fpath--;
	while(*fpath == '/') fpath--;
	while(*fpath != '/') fpath--;
	fpath++;
	return fpath;
}

size_t path_basedir(char *out, const char *path)
{
	size_t ret = 0;
	const char *basename = path_basename(path);

	if (path == NULL || basename == NULL) return 0;
	while(&path[ret] != basename) {
		out[ret] = path[ret];
		ret++;
	}

	out[ret] = '\0';
	return ret;
}

int open_compound_path(int mode, const char *fmt, ...)
{
	char path[MAX_PATH + 1];
	va_list va;

	va_start(va, fmt);
	vsnprintf(path, MAX_PATH, fmt, va);
	va_end(va);

	path[MAX_PATH] = '\0';
	return vfs_open(path, mode);
}

static const char *format_suf[] = {
	"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"
};

size_t size_format(char *out, off_t size)
{
	size_t size_log2, scale, intpart, decpart;

	if (size < 0) {
		strcpy(out, "negative?");
		return 0;
	}

	if (size == 0) {
		size_log2 = 0;
	} else if ((size_t)(size >> 32) == 0) {
		size_log2 = (31 - __builtin_clz((size_t)size));
	} else {
		size_log2 = (63 - __builtin_clz((size_t)(size >> 32)));
	}

	scale = (size_log2 / 10) * 10;

	intpart = size >> scale;
	decpart = ((size - (intpart << scale)) % 1000) / 100;

	return sprintf(out, "%d.%d %s", intpart, decpart, format_suf[scale / 10]);
}
