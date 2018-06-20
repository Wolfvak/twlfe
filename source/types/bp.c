#include <nds.h>
#include <string.h>

#include "bp.h"

#define BP_USZ		(sizeof(size_t))
#define BP_UBITS	(BP_USZ * 8)

#define BP_SIZEW(n)	(((n) + BP_USZ - 1) / BP_USZ)
#define BP_SIZEB(n)	(BP_SIZEW(n) * BP_USZ)

#define BP_IDX(n)	((n) / BP_USZ)
#define BP_MASK(n)	(1 << ((n) % BP_UBITS))

#define BP_ALLCLR	((size_t)0)
#define BP_ALLSET	((size_t)~0ULL)

int bp_init(bp_t *bp, int n)
{
	bp->map = malloc(BP_SIZEB(n));
	if (bp->map == NULL) return -1;

	memset(bp->map, 0, BP_SIZEB(n));

	bp->count = 0;
	bp->max = n;
	bp->lastc = 0;
	bp->lasts = 0;
	return 0;
}

void bp_free(bp_t *bp)
{
	free(bp->map);
}

int bp_count(bp_t *bp)
{
	return bp->count;
}

int bp_rem(bp_t *bp)
{
	return bp->max - bp->count;
}

bool bp_test(bp_t *bp, int i)
{
	return (bp->map[BP_IDX(i)] & BP_MASK(i)) != 0;
}

void bp_set(bp_t *bp, int i)
{
	if (bp_test(bp, i)) return;
	bp->map[BP_IDX(i)] |= BP_MASK(i);
	bp->lasts = i;
	bp->count++;
}

void bp_clear(bp_t *bp, int i)
{
	if (!bp_test(bp, i)) return;
	bp->map[BP_IDX(i)] &= ~BP_MASK(i);
	bp->lastc = i;
	bp->count--;
}

int bp_ffc(bp_t *bp)
{
	int i, ret;
	if (bp_rem(bp) == 0) return -1;

	i = bp->lastc - (bp->lastc % BP_UBITS);
	while(1) {
		size_t w = bp->map[BP_IDX(i)];
		if (w != BP_ALLSET) {
			ret = (BP_UBITS - 1) - __builtin_clz(w);
			break;
		}

		i += BP_UBITS;
		if (i >= bp->max) i = 0;
	}
	return ret;
}

int bp_ffs(bp_t *bp)
{
	int i, ret;
	if (bp_count(bp) == 0) return -1;

	i = bp->lasts - (bp->lasts % BP_UBITS);
	while(1) {
		size_t w = bp->map[BP_IDX(i)];
		if (w != BP_ALLCLR) {
			ret = (BP_UBITS - 1) - __builtin_clz(~w);
			break;
		}

		i += BP_UBITS;
		if (i >= bp->max) i = 0;
	}
	return ret;
}
