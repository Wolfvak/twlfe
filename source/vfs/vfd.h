#ifndef VFD_H__
#define VFD_H__

#include <nds.h>

#include "vfs.h"

int vfd_valid_fd(int fd);
vf_t *vfd_get(int fd);
int vfd_retrieve(void);
void vfd_return(int fd);

static inline int _vf_opened(vf_t *v) {
	return (v->flags & VFS_OPEN);
}

static inline int _vf_file(vf_t *v) {
	return (v->flags & VFS_FILE);
}

static inline int _vf_dir(vf_t *v) {
	return (v->flags & VFS_DIR);
}

static inline int _vf_readable(vf_t *v) {
	return (v->flags & VFS_RO);
}

static inline int _vf_writable(vf_t *v) {
	return (v->flags & VFS_WO);
}

#endif /* VFD_H__ */
