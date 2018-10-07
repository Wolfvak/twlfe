#ifndef FILEIDENT_H__
#define FILEIDENT_H__

#define MAX_FHANDLERS (4)

typedef int (*fident_handle_t)(const char *path);

typedef struct {
	const char *type_name;		/** Type name */
	fident_handle_t handler;	/** Handler function */
} fident_t;

typedef struct {
	const char *path;	/** File path */
	const char *ext;	/** File extension */
	const void *data;	/** First bytes of the file, at least req_size */
	int fd;				/** Already open file descriptor */
} fident_ctx_t;

typedef int (*fident_verify_t)(fident_ctx_t*);

typedef struct {
	u32 priority;			/** Handler priority, lower priority goes first */
	size_t req_size;		/** Required buffer size */
	fident_verify_t verify;	/** Verification function */
	fident_t *identity;		/** Identity structure */
} fident_handler_t;

int fident_register(fident_handler_t *handler);
fident_t *fident_identify(const char *path);

#endif /* FILEIDENT_H__ */
