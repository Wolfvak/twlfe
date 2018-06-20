#ifndef PSTOR_H__
#define PSTOR_H__

#include <nds.h>

#include "err.h"

#define PSTOR_CACHE_DIV	(64)
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

int pstor_init(pstor_t *ps, size_t bufsz, size_t max);
void pstor_free(pstor_t *ps);
void pstor_clear(pstor_t *ps);
int pstor_concat(pstor_t *ps, const char *str);
int pstor_getpath(pstor_t *ps, char *out, size_t max, size_t i);
size_t pstor_count(pstor_t *ps);

#endif /* PSTOR_H__ */
