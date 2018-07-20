#include <nds.h>

#include "global.h"
#include "err.h"

#include "bp.h"

int bp_init(bp_t *bp, int n)
{
	bp->map = malloc(BP_SIZEB(n));
	if (bp->map == NULL) return -ERR_MEM;

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

void bp_clearall(bp_t *bp)
{
	memset(bp->map, BP_ALLCLR, BP_SIZEB(bp->max));
	bp->count = 0;
	bp->lastc = 0;
	bp->lasts = 0;
}

void bp_setall(bp_t *bp)
{
	memset(bp->map, BP_ALLSET, BP_SIZEB(bp->max));
	bp->count = bp->max;
	bp->lastc = bp->max;
	bp->lasts = bp->max;
}

void bp_set(bp_t *bp, int i)
{
	if (UNLIKELY(bp_tst(bp, i))) return;
	bp->map[BP_IDX(i)] |= BP_MASK(i);
	bp->lasts = i;
	bp->count++;
}

void bp_clr(bp_t *bp, int i)
{
	if (UNLIKELY(!bp_tst(bp, i))) return;
	bp->map[BP_IDX(i)] &= ~BP_MASK(i);
	bp->lastc = i;
	bp->count--;
}

void bp_xor(bp_t *bp, int i)
{
	int present = bp_tst(bp, i);
	bp->map[BP_IDX(i)] ^= BP_MASK(i);

	if (present) {
		bp->count--;
		bp->lastc = i;
	} else {
		bp->count++;
		bp->lasts = i;
	}
}

int bp_find_clr(bp_t *bp)
{
	size_t i;
	int ret;

	if (UNLIKELY(bp_clrcnt(bp) == 0)) return -1;

	i = BP_IDX(bp->lastc);

	while(1) {
		size_t w = bp->map[i];
		if (w != BP_ALLSET) {
			ret = (BP_UBITS - 1) - __builtin_clz(~w);
			break;
		}

		i++;
		if (UNLIKELY(i >= BP_SIZEW(bp->max)))
			i = 0;
	}
	return ret + (i * BP_UBITS);
}

int bp_find_set(bp_t *bp)
{
	size_t i;
	int ret;

	if (UNLIKELY(bp_setcnt(bp) == 0)) return -1;

	i = BP_IDX(bp->lasts);

	while(1) {
		size_t w = bp->map[i];
		if (w != BP_ALLCLR) {
			ret = (BP_UBITS - 1) - __builtin_clz(w);
			break;
		}

		i++;
		if (UNLIKELY(i >= BP_SIZEW(bp->max)))
			i = 0;
	}
	return ret + (i * BP_UBITS);
}
