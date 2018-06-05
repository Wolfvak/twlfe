#include <nds.h>

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
static const size_t errstr_c = sizeof(errstr)/sizeof(*errstr);

const char *err_getstr(int err)
{
	if (!IS_ERR(err))
		return "no error";

	if (-err >= errstr_c)
		return "unknown error";

	return errstr[-err];
}
