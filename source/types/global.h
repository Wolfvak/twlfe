#ifndef GLOBAL_H__
#define GLOBAL_H__

#define LIKELY(x)	__builtin_expect((x), 1)
#define UNLIKELY(x)	__builtin_expect((x), 0)

#endif /* GLOBAL_H__ */
