#ifndef VFS_HELP_H__
#define VFS_HELP_H__

#include <string.h>
#include "vfs.h"

static inline int _vfs_drvlet_to_idx(char drv)
{
	if (drv >= VFS_FIRSTMOUNT && drv <= VFS_LASTMOUNT)
		return (drv - VFS_FIRSTMOUNT);
	return -1;
}

static const char *_vfs_get_lpath(const char *fp)
{
	/* make sure the first letter corresponds to a mount */
	if (*fp < VFS_FIRSTMOUNT || *fp > VFS_LASTMOUNT) return NULL;

	/* verify the next two chars are ':' & '/' */
	if (*(++fp) != ':') return NULL;
	if (*(++fp) != '/') return NULL;

	/* skip any trailing '/' */
	while(*fp == '/') fp++;
	return fp;
}

static int _vfs_check_lpath(const char *lp)
{
	if (lp == NULL || strlen(lp) > MAX_PATH) return 0;
	return 1;
}

static inline int _vfs_ent_opened(vf_t *v) {
	return (v->flags & VFS_OPEN);
}

static inline int _vfs_ent_file(vf_t *v) {
	return (v->flags & VFS_FILE) == VFS_FILE;
}

static inline int _vfs_ent_dir(vf_t *v) {
	return (v->flags & VFS_DIR) == VFS_DIR;
}

static inline int _vfs_ent_readable(vf_t *v) {
	return (v->flags & VFS_RO);
}

static inline int _vfs_ent_writable(vf_t *v) {
	return (v->flags & VFS_WO);
}

#endif /* VFS_HELP_H__ */
