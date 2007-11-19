#ifndef __MU_UTIL_H__
#define __MU_UTIL_H__

#include <stdarg.h>

char* format(const char* format, ...);
char* formatv(const char* format, va_list ap);
const char* basename(const char* filename);

#endif
