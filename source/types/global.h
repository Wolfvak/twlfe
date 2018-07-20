#ifndef GLOBAL_H__
#define GLOBAL_H__

/*
 * general order for includes:
 *
 * 1) libc (string.h, stdlib.h, stdio.h)
 * 2) libnds (nds.h, nds/arm9/dldi.h)
 * 3) generic type (global.h, err.h)
 * 4) subsystem (vfs.h, filetype.h)
 * 5) shared helpers (vfs_glue.h, vfs_blkop.h)
 * 6) non-shared helpers (font.h, vfd.h)
 */

#include <nds.h>

#define LIKELY(x)	__builtin_expect((x), 1)
#define UNLIKELY(x)	__builtin_expect((x), 0)

#define SET_PRIVDATA(x, d)	((x)->priv = ((void*)(d)))
#define GET_PRIVDATA(x, t)	((t)((x)->priv))

#define CLAMP(x, a, b)		((x) < (a) ? (a) : (((x) > (b) ? (b) : (x))))

#define PROCESS_KEYS(keys) \
	for (typeof((keys)) _k = keys, _kproc = 1; _k && _kproc; _k &= ~BIT(31 - __builtin_clz(_k))) \
	switch(_k & BIT(31 - __builtin_clz(_k)))

#define PROCESS_KEYS_STOP _kproc = 0

#endif /* GLOBAL_H__ */
