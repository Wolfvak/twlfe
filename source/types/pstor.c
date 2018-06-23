#include <nds.h>

#include "err.h"
#include "pstor.h"

int pstor_init(pstor_t *ps, size_t bufsz, size_t max)
{
	void *wbuf;
	size_t cache_sz, wbufsz;

	if (ps == NULL) return -ERR_MEM;

	cache_sz = ((max + PSTOR_CACHE_DIV - 1) / PSTOR_CACHE_DIV) * sizeof(char*);
	wbufsz = bufsz + max + cache_sz;
	wbuf = malloc(wbufsz);
	if (wbuf == NULL) return -ERR_MEM;

	ps->buflen = bufsz;
	ps->max = max;
	ps->cache_sz = cache_sz;

	ps->buf = (char*)wbuf;
	ps->lengths = (u8*)(wbuf + bufsz);
	ps->cache = (char**)(wbuf + bufsz + max);

	pstor_clear(ps);
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
	ps->cache[0] = ps->buf;
	ps->lastc = 0;
	ps->count = 0;
}

int pstor_cat(pstor_t *ps, const char *str)
{
	size_t slen, totlen;

	if (ps == NULL || str == NULL) return -ERR_MEM;

	slen = strlen(str);
	totlen = slen + ps->lengths[ps->count];

	if (totlen > ps->buflen || totlen >= 0x100) return -ERR_MEM;

	memcpy(&ps->buf[ps->lastc], str, slen);
	ps->lastc += slen;
	ps->lengths[ps->count] = totlen;
	return 0;
}

int pstor_finish(pstor_t *ps)
{
	if (ps == NULL || ps->count == ps->max) return -ERR_MEM;

	ps->count++;
	if ((ps->count % PSTOR_CACHE_DIV) == 0)
		ps->cache[ps->count / PSTOR_CACHE_DIV] = &ps->buf[ps->lastc];
	return 0;
}

int pstor_getpath(pstor_t *ps, char *out, size_t max, size_t i)
{
	size_t cache_idx, plen;
	char *path;

	if (ps == NULL || out == NULL || i >= ps->count) return -ERR_MEM;

	cache_idx = i / PSTOR_CACHE_DIV;

	path = ps->cache[cache_idx];
	for (int j = cache_idx * PSTOR_CACHE_DIV; j < i; j++)
		path += ps->lengths[j];

	plen = ps->lengths[i];
	plen = (plen < max) ? plen : max;
	memcpy(out, path, plen);
	out[plen] = '\0';
	return plen;
}

size_t pstor_count(pstor_t *ps)
{
	return ps->count;
}

size_t pstor_max(pstor_t *ps)
{
	return ps->max;
}
