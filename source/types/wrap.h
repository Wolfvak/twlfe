#ifndef WRAP_H__
#define WRAP_H__

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <nds.h>
#include <stdio.h>

static inline void *malloc_abrt(size_t size) {
	void *ret = malloc(size);
	if (ret == NULL) {
		iprintf("out of memory\n");
		while(1);
	}
	return ret;
}

static inline void *zmalloc_abrt(size_t size) {
	void *ret = malloc_abrt(size);
	memset(ret, 0, size);
	return ret;
}

#endif /* WRAP_H__ */