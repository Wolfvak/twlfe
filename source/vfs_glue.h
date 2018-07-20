#ifndef VFS_GLUE_H__
#define VFS_GLUE_H__

#include <stdarg.h>

#include <nds.h>

/*
 * copies file `path` to `new` and shows the progress
 */
int copy_file_ui(const char *path, const char *new);

/*
 * attempts to rename the file from `old` to `new` and
 * if that fails *AND* `fallback` is set, it tries to
 * copy the file (with progress), then remove the original
 */
int move_file_ui(const char *old, const char *new, bool fallback);

/* null terminates the last '/' char */
size_t path_basedir(char *path);

/* opens a file with a special path, pretty much auxiliary */
int open_compound_path(int mode, const char *fmt, ...);

size_t size_format(char *out, off_t size);

static inline bool path_is_topdir(const char *p) {
	if (p[0] < VFS_FIRSTMOUNT || p[0] > VFS_LASTMOUNT) return false;
	if (p[1] != ':') return false;
	if (p[2] != '/') return false;
	if (p[3] != '\0') return false;
	return true;
}

#endif /* VFS_GLUE_H__ */
