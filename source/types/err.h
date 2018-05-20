#ifndef ERR_H__
#define ERR_H__

enum {
	ERR_NONE = 0,
	ERR_ARG,		/**< Invalid argument */
	ERR_MEM,		/**< Out of memory / NULL pointer */
	ERR_DEV,		/**< Invalid device */
	ERR_IO,			/**< I/O error */
	ERR_BUSY,		/**< Device is busy */
	ERR_PERM,		/**< No permissions */
	ERR_NOTREADY,	/**< Device not ready */
	ERR_NOTFOUND,	/**< File or device not found */
};

#define IS_ERR(x) ((x) != 0)

const char *err_getstr(int err);

#endif /* ERR_H__ */
