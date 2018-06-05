#include <nds.h>
#include "err.h"
#include "vfs.h"
#include "vfs_blkop.h"

#define BLK_MAX (0x80000)

static void *_vfs_blk_buffer(size_t min, size_t max, size_t *bsz)
{
	size_t blk_sz = max;
	void *ret = NULL;
	do {
		ret = malloc(blk_sz);
		if (ret == NULL) blk_sz /= 2;
	} while((ret == NULL) && (blk_sz >= min));
	*bsz = blk_sz;
	return ret;
}

int vfs_blk_read(vfs_blk_op_t *op)
{
	int res;
	void *dbuf;
	size_t dbuf_len;
	off_t rem, blk_sz;

	if (op == NULL || op->cb == NULL) return -ERR_ARG;
	if (op->min_blk_len <= 0 ||
		op->min_blk_len > op->max_blk_len ||
		op->min_blk_len > op->data_len) return -ERR_ARG;

	dbuf_len = op->max_blk_len;
	if (dbuf_len > BLK_MAX)
		dbuf_len = BLK_MAX;

	dbuf = _vfs_blk_buffer(op->min_blk_len, dbuf_len, &dbuf_len);
	if (dbuf == NULL) return -ERR_MEM;

	res = 0;
	rem = op->data_len;
	while(rem) {
		blk_sz = (rem > dbuf_len) ? dbuf_len : rem;

		res = vfs_read(op->fd, dbuf, blk_sz);
		if (IS_ERR(res)) break;

		res = op->cb(blk_sz, dbuf, op->priv);
		if (IS_ERR(res)) break;

		rem -= blk_sz;
	}

	op->data_len = rem;
	free(dbuf);
	return res;
}
