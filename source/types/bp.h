#ifndef BP_H__
#define BP_H__

/* size of the bitpack atom in bytes / bits */
#define BP_USZ		(sizeof(size_t))
#define BP_UBITS	(BP_USZ * 8)

/* size of a bitpack map in atoms / bytes */
#define BP_SIZEW(n)	(((n) + BP_USZ - 1) / BP_USZ)
#define BP_SIZEB(n)	(BP_SIZEW(n) * BP_USZ)

/* map index / bitmask */
#define BP_IDX(n)	((n) / BP_UBITS)
#define BP_MASK(n)	(1 << ((n) % BP_UBITS))

/* map values when all bits are clear / set */
#define BP_ALLCLR	((size_t)0)
#define BP_ALLSET	((size_t)~0ULL)

typedef struct {
	size_t *map;
	int count;
	int max;
	int lastc;
	int lasts;
} bp_t;

/* initialize the bitpack with up to `n` entries */
int bp_init(bp_t *bp, int n);

/* free any memory allocated by `bp` */
void bp_free(bp_t *bp);

/* clear all bits in `bp` */
void bp_clearall(bp_t *bp);

/* set all bits in `bp` */
void bp_setall(bp_t *bp);

/* return the amount of set / clear / total bits in `bp` */
static inline int bp_setcnt(bp_t *bp) {
	return bp->count;
}

static inline int bp_clrcnt(bp_t *bp) {
	return bp->max - bp->count;
}

static inline int bp_maxcnt(bp_t *bp) {
	return bp->max;
}

/* test / set / clear / toggle the `i`-th bit in `bp` */
static inline int bp_tst(bp_t *bp, int i) {
	return (bp->map[BP_IDX(i)] & BP_MASK(i)) != 0;
}

void bp_set(bp_t *bp, int i);
void bp_clr(bp_t *bp, int i);
void bp_xor(bp_t *bp, int i);

/* finds a single clear/set bit in `bp` */
int bp_find_clr(bp_t *bp);
int bp_find_set(bp_t *bp);

#endif /* BP_H__ */
