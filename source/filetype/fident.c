#include <stdio.h>
#include <nds.h>

#include "global.h"
#include "err.h"

#include "fident.h"
#include "vfs.h"

#include "vfs_glue.h"

static fident_handler_t *fhandlers[MAX_FHANDLERS] = {NULL};
static int fhandlers_count = 0;
static size_t fhandlers_maxsize = 0;

int fident_register(fident_handler_t *handler)
{
	int ret = fhandlers_count;
	if (handler == NULL || fhandlers_count == MAX_FHANDLERS)
		return -ERR_MEM;

	if (handler->req_size > fhandlers_maxsize)
		fhandlers_maxsize = handler->req_size;

	fhandlers[fhandlers_count++] = handler;
	return ret;
}

fident_t *fident_identify(const char *path)
{
	int fd;
	off_t rb;
	void *data;
	fident_ctx_t ctx;

	fd = vfs_open(path);
	if (IS_ERR(fd)) return 0;

	data = malloc(fhandlers_maxsize);
	if (data == NULL) {
		vfs_close(fd);
		return 0;
	}

	rb = vfs_read(fd, data, fhandlers_maxsize);
	if (IS_ERR(rb)) {
		vfs_close(fd);
		free(data);
		return 0;
	}

	ctx.path = path;
	ctx.ext = path_extension(path);
	ctx.data = data;
	ctx.fd = fd;

	for (int i = 0; i < fhandlers_count; i++) {
		if (fhandlers[i]->verify(&ctx))
			return fhandlers[i]->identity;
	}

	return NULL;
}
