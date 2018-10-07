#include <stdio.h>
#include <stdarg.h>
#include <nds.h>

#include "global.h"
#include "err.h"

#include "ui.h"
#include "vfs.h"

#include "vfs_glue.h"

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
