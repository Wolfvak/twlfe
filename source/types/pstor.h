#ifndef PSTOR_H__
#define PSTOR_H__

#include <nds.h>

#include "err.h"

#define PSTOR_CACHE_DIV	(32)
/*
 * need to loop ^ times in a worst case
 * scenario to get an arbitrary path
 */

typedef struct {
	size_t lastc;
	size_t count;
	size_t buflen;
	size_t max;
	size_t cache_sz;

	char *buf;
	u8 *lengths;
	char **cache;
} pstor_t;


/*
 * initializes path store with `bufsz` bytes for the char buffer
 * and enough space to hold up to `max` elements
 */
int pstor_init(pstor_t *ps, size_t bufsz, size_t max);

/* frees any memory taken by the path store */
void pstor_free(pstor_t *ps);

/* clears all paths */
void pstor_clear(pstor_t *ps);

/* concatenate a string to the current path */
int pstor_cat(pstor_t *ps, const char *str);

/* move on to the next path */
int pstor_finish(pstor_t *ps);

/* copy the `i`-th path to `out` */
int pstor_getpath(pstor_t *ps, char *out, size_t max, size_t i);

/* get the current amount of paths in the store */
static inline size_t pstor_count(pstor_t *ps) {
	return ps->count;
}

/* get the number of available slots in the store */
static inline size_t pstor_max(pstor_t *ps) {
	return ps->max;
}

#endif /* PSTOR_H__ */
