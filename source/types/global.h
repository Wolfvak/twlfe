#ifndef GLOBAL_H__
#define GLOBAL_H__

#define LIKELY(x)	__builtin_expect((x), 1)
#define UNLIKELY(x)	__builtin_expect((x), 0)

#define SET_PRIVDATA(x, d)	((x)->priv = ((void*)(d)))
#define GET_PRIVDATA(x, t)	((t)((x)->priv))

#define CLAMP(x, a, b)		((x) < (a) ? (a) : (((x) > (b) ? (b) : (x))))

#endif /* GLOBAL_H__ */
