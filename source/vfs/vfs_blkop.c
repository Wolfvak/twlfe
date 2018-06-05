#include <nds.h>

#include "err.h"
#include "vfs.h"

#include "vfs_blkop.h"

#define BLK_MAX (0x80000)

static void *_vfs_blk_buffer(size_t min, size_t max, size_t *bsz)
{
	if (max > BLK_MAX) max = BLK_MAX;
	void *ret = NULL;
	do {
		ret = malloc(max);
		if (ret == NULL) max /= 2;
	} while((ret == NULL) && (max >= min));
	*bsz = max;
	return ret;
}

int vfs_blk_read(vfs_blk_op_t *op)
{
	int res;
	void *dbuf;
	size_t dbuf_len;
	off_t rem, blk_sz, rb;

	if (op == NULL || op->cb == NULL) return -ERR_ARG;
	if (op->min_blk_len <= 0 ||
		op->min_blk_len > op->max_blk_len ||
		op->min_blk_len > op->data_len) return -ERR_ARG;

	dbuf_len = op->max_blk_len;
	dbuf = _vfs_blk_buffer(op->min_blk_len, dbuf_len, &dbuf_len);
	if (dbuf == NULL) return -ERR_MEM;

	res = 0;
	rem = op->data_len;
	while(rem) {
		blk_sz = (rem > dbuf_len) ? dbuf_len : rem;
		rb = vfs_read(op->fd, dbuf, blk_sz);
		if (IS_ERR(rb)) break;
		res = op->cb(rb, dbuf, op->priv);
		if (IS_ERR(res)) break;

		rem -= rb;
		if (rb != blk_sz) break;
	}

	op->data_len = rem;
	free(dbuf);
	return res;
}

int vfs_blk_write(vfs_blk_op_t *op)
{
	int res;
	void *dbuf;
	size_t dbuf_len;
	off_t rem, blk_sz, wb;

	if (op == NULL || op->cb == NULL) return -ERR_ARG;
	if (op->min_blk_len <= 0 ||
		op->min_blk_len > op->max_blk_len ||
		op->min_blk_len > op->data_len) return -ERR_ARG;

	dbuf_len = op->max_blk_len;
	dbuf = _vfs_blk_buffer(op->min_blk_len, dbuf_len, &dbuf_len);
	if (dbuf == NULL) return -ERR_MEM;

	res = 0;
	rem = op->data_len;
	while(rem) {
		blk_sz = (rem > dbuf_len) ? dbuf_len : rem;
		wb = vfs_write(op->fd, dbuf, blk_sz);
		if (IS_ERR(wb)) break;
		res = op->cb(wb, dbuf, op->priv);
		if (IS_ERR(res)) break;

		rem -= wb;
		if (wb != blk_sz) break;
	}

	op->data_len = rem;
	free(dbuf);
	return res;
}
