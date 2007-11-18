#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

const char* format(const char* format, ...)
{
    va_list ap;
    const char* result;
    va_start(ap, format);
    result = Gib_Formatv(format, ap);
    va_end(ap);
    return result;
}

const char* formatv(const char* format, va_list ap)
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
