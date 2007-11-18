#ifndef __MU_UTIL_H__
#define __MU_UTIL_H__

#include <stdarg.h>

const char* format(const char* format, ...);
const char* formatv(const char* format, va_list ap);

#endif
