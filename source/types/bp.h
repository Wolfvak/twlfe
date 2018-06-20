#ifndef BP_H__
#define BP_H__

typedef struct {
	size_t *map;
	int count;
	int max;
	int lastc;
	int lasts;
} bp_t;

int bp_init(bp_t *bp, int n);
void bp_free(bp_t *bp);

int bp_count(bp_t *bp);
int bp_rem(bp_t *bp);

bool bp_test(bp_t *bp, int i);
void bp_set(bp_t *bp, int i);
void bp_clear(bp_t *bp, int i);

int bp_ffc(bp_t *bp);
int bp_ffs(bp_t *bp);

#endif /* BP_H__ */
