#include <nds.h>

#include "err.h"
#include "vfs.h"

typedef int (*vfs_blk_cb)(off_t blk_len, void *buf, void *priv);

typedef struct {
	int fd;
	vfs_blk_cb cb;

	size_t min_blk_len;
	size_t max_blk_len;
	off_t data_len;
	void *priv;
} vfs_blk_op_t;

int vfs_blk_read(vfs_blk_op_t *op);
int vfs_blk_write(vfs_blk_op_t *op);
