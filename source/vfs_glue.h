#ifndef VFS_GLUE_H__
#define VFS_GLUE_H__

#include <nds.h>
#include <stdarg.h>

#define SIZE_KIB(x) ((off_t)(x) << 10ULL)
#define SIZE_MIB(x) ((off_t)(x) << 20ULL)
#define SIZE_GIB(x) ((off_t)(x) << 30ULL)

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

/* get the base filename from a full path */
const char *path_basename(const char *fpath);

/*
 * copies `path` to `out` up to the base path
 * assumes sizeof(out) >= sizeof(path)
 */
size_t path_basedir(char *out, const char *path);

/*
 * opens a file with a special path, pretty much auxiliary
 */
int open_compound_path(int mode, const char *fmt, ...);

size_t size_format(char *out, size_t max, off_t size);

#endif /* VFS_GLUE_H__ */
