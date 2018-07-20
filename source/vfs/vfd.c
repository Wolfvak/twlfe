#include <nds.h>

#include "vfs.h"

#include "vfd.h"

static vf_t vfds[MAX_FILES];
static int vfdc, vfdstack[MAX_FILES];

static void __attribute__((constructor)) __vfd_ctor(void)
{
	memset(vfds, 0, sizeof(vfds));
	vfdc = 0;
	for (int i = 0; i < MAX_FILES; i++)
		vfdstack[i] = i;
}

int vfd_valid_fd(int fd)
{
	if (fd < 0 || fd >= MAX_FILES) return 0;
	return _vf_opened(&vfds[fd]);
}

vf_t *vfd_get(int fd)
{
	return &vfds[fd];
}

int vfd_retrieve(void)
{
	if (vfdc == MAX_FILES) return -1;
	return vfdstack[vfdc++];
}

void vfd_return(int fd)
{
	vfdstack[--vfdc] = fd;
	memset(&vfds[fd], 0, sizeof(*vfds));
}
