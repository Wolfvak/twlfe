#include <stddef.h>

#include "err.h"

static const char *errstr[] = {
	[ERR_ARG]		= "invalid argument",
	[ERR_MEM]		= "out of memory",
	[ERR_DEV]		= "invalid device",
	[ERR_IO]		= "IO error",
	[ERR_BUSY]		= "device or file busy",
	[ERR_PERM]		= "invalid permissions",
	[ERR_NOTREADY]	= "not ready",
	[ERR_NOTFOUND]	= "path not found",
};
static const size_t errstr_c = sizeof(errstr)/sizeof(*errstr);

const char *err_getstr(int err)
{
	if (err >= 0 && err < errstr_c) {
		if (errstr[err] != NULL)
			return errstr[err];
	}
	return "unknown";
}
