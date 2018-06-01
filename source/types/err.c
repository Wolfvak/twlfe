#include <stddef.h>

#include "err.h"

static const char *errstr[] = {
	[ERR_ARG]		= "invalid argument",
	[ERR_MEM]		= "out of memory",
	[ERR_DEV]		= "invalid device",
	[ERR_IO]		= "IO error",
	[ERR_BUSY]		= "device or file busy",
	[ERR_NOTREADY]	= "not ready",
	[ERR_NOTFOUND]	= "path not found",
	[ERR_UNSUPP]	= "unsupported operation",
};

const char *err_getstr(int err)
{
	if (err < 0)
		return errstr[-err];

	return "no error";
}
