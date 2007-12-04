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
    
    if (vsnprintf(result, length+1, format, mine) < length)
        return NULL;
    else
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
		return final_slash + 1;
	else
		return filename;
}
