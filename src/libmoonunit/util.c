#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <moonunit/util.h>

char* formatv(const char* format, va_list ap)
{
    va_list mine;
    int length;
    char* result;

    va_copy(mine, ap);

    length = vsnprintf(NULL, 0, format, mine);

    va_copy(mine, ap);

    result = malloc(length+1);
    
    vsnprintf(result, length+1, format, mine);

    return result;
}

char* format(const char* format, ...)
{
    va_list ap;
    char* result;
    va_start(ap, format);
    result = formatv(format, ap);
    va_end(ap);
    return result;
}

const char* basename(const char* filename)
{
	char* final_slash = strrchr(filename, '/');
	
	if (final_slash)
		return final_slash - 1;
	else
		return filename;
}
