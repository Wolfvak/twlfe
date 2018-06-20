#include <nds.h>

#include "err.h"
#include "pstor.h"

int pstor_init(pstor_t *ps, size_t bufsz, size_t max)
{
	void *wbuf;
	size_t cache_sz, wbufsz;

	cache_sz = ((max + PSTOR_CACHE_DIV - 1) / PSTOR_CACHE_DIV) * sizeof(char*);
	wbufsz = bufsz + max + cache_sz;

	if (ps == NULL) return -ERR_MEM;

	wbuf = malloc(wbufsz);
	if (wbuf == NULL) return -ERR_MEM;

	ps->lastc = 0;
	ps->count = 0;
	ps->buflen = bufsz;
	ps->max = max;
	ps->cache_sz = cache_sz;

	ps->buf = (char*)wbuf;
	ps->lengths = (u8*)(wbuf + bufsz);
	ps->cache = (char**)(wbuf + bufsz + max);

	memset(wbuf, 0, wbufsz);
	return 0;
}

void pstor_free(pstor_t *ps)
{
	free(ps->buf);
}

void pstor_clear(pstor_t *ps)
{
	memset(ps->buf, 0, ps->buflen);
	memset(ps->lengths, 0, ps->max);
	memset(ps->cache, 0, ps->cache_sz);
	ps->lastc = 0;
	ps->count = 0;
}

int pstor_concat(pstor_t *ps, const char *str)
{
	size_t slen, pos, idx;
	if (ps == NULL || str == NULL || ps->count == ps->max) return -ERR_MEM;

	slen = strlen(str);
	pos = ps->lastc;
	idx = ps->count;

	if ((pos + slen) > ps->buflen) return -ERR_MEM;

	memcpy(&ps->buf[pos], str, slen);
	ps->lengths[idx] = slen;

	if ((idx % PSTOR_CACHE_DIV) == 0)
		ps->cache[idx / PSTOR_CACHE_DIV] = &ps->buf[pos];

	ps->count++;
	ps->lastc += slen;
	return 0;
}

int pstor_getpath(pstor_t *ps, char *out, size_t max, size_t i)
{
	size_t cache_idx, plen;
	char *path;

	if (ps == NULL || out == NULL || i >= ps->count) return -ERR_MEM;

	cache_idx = i / PSTOR_CACHE_DIV;

	path = ps->cache[cache_idx];
	for (int j = cache_idx * PSTOR_CACHE_DIV; j < i; j++) {
		path += ps->lengths[j];
	}

	plen = ps->lengths[i];
	plen = (plen < max) ? plen : max;
	strncpy(out, path, plen);
	return plen;
}

size_t pstor_count(pstor_t *ps)
{
	return ps->count;
}
