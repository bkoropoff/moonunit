/*
 * Copyright (c) 2007, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

#include <moonunit/util.h>

bool
ends_with (const char* haystack, const char* needle)
{
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);

    if (hlen >= nlen)
        return !strcmp(haystack + hlen - nlen, needle);
    else
        return false;
}

char* formatv(const char* format, va_list ap)
{
    va_list mine;
    int length;
    char* result = NULL;

    length = vsnprintf(NULL, 0, format, mine);

    if (length == -1)
    {
	int capacity = 16;
	do
	{
	    capacity *= 2;
	    va_copy(mine, ap);
	    result = realloc(result, capacity);
	} while ((length = vsnprintf(result, capacity-1, format, mine)) == -1 || capacity <= length);
	result[length] = '\0';

	return result;
    }
    else
    {
	va_copy(mine, ap);
	
	result = malloc(length+1);
	
	if (vsnprintf(result, length+1, format, mine) < length)
	    return NULL;
	else
	    return result;
    }
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

const char* basename_pure(const char* filename)
{
	char* final_slash = strrchr(filename, '/');
	
	if (final_slash)
		return final_slash + 1;
	else
		return filename;
}

typedef struct
{
    size_t size, capacity;
    void* elements[];
} _array;

static inline array*
hide(_array* a)
{
    return (array*) (((char*) a) + sizeof(_array));
}

static inline _array*
reveal(array* a)
{
    return (_array*) (((char*) a) - sizeof(_array));
}

static inline _array*
ensure(_array* a, size_t size)
{
    if (a->capacity >= size)
    {
        return a;
    }
    else
    {
        if (a->capacity == 0)
        {
            a->capacity = 32;
        }
        while (a->capacity < size)
        {
            a->capacity *= 2;
        }
        a = realloc(a, sizeof(_array) + sizeof(void*) * a->capacity);
        return a;
    }
}

array*
array_new(void)
{
    _array* a = malloc(sizeof(_array));

    a->size = a->capacity = 0;

    return hide(a);
}

size_t
array_size(array* a)
{
    return reveal(a)->size;
}

array*
array_append(array* a, void* e)
{
    _array* _a = a ? reveal(a) : reveal(array_new());

    _a = ensure(_a, _a->size + 1);

    _a->elements[_a->size++] = e;

    return hide(_a);
}


void*
mu_dlopen(const char* path, int flags)
{
    void* handle = dlopen(path, flags);

#if defined(RTLD_MEMBER) && defined(PLUGIN_MEMBER_EXTENSION)
    if (!handle && ends_with(path, PLUGIN_EXTENSION))
    {
        char* base = strdup(basename_pure(path));
        if (strlen(base) > strlen(PLUGIN_EXTENSION))
            base[strlen(base) - strlen(PLUGIN_EXTENSION)] = '\0';
        char* newpath = format("%s(%s%s)", path, base, PLUGIN_MEMBER_EXTENSION);
        handle = dlopen(newpath, flags | RTLD_MEMBER);
        free(base);
        free(newpath);
    }
#endif

    return handle;
}
