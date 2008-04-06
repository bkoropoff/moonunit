/*
 * Copyright (c) 2008, Brian Koropoff
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <moonunit/test.h>

static void
fill_backtrace(MuBacktrace* trace, char* info)
{
    char* open_paren;
    char* open_brace;

    open_brace = strchr(info, '[');
    if (open_brace)
    {
        trace->ret_addr = strtoul(open_brace + 3, NULL, 16);
    }
    else
    {
        trace->ret_addr = 0x0;
    }

    if (open_brace > info)
    {
        open_brace[-1] = '\0';
        if ((open_paren = strchr(info, '(')))
        {
            char* close_paren = strchr(open_paren, ')');
            char* plus = strchr(open_paren, '+');
            *open_paren = '\0';
            *close_paren = '\0';
            *plus = '\0';
            trace->func_name = strdup(open_paren+1);
            trace->ip_offset = strtoul(plus + 3, NULL, 16);
        }
        else
        {
            trace->func_name = NULL;
        }
        trace->file_name = strdup(info);
    }
    else
    {
        trace->file_name = trace->func_name = NULL;
    }
}

MuBacktrace*
get_backtrace(int skip)
{
    MuBacktrace* trace, **out;
    void* buffer[100];
    int num_frames;
    int i;
    char** symbols;

    num_frames = backtrace(buffer, sizeof(buffer) / sizeof(*buffer));
    symbols = backtrace_symbols(buffer, num_frames);

    out = &trace;

    for (i = skip; i < num_frames; i++)
    {
        *out = calloc(1, sizeof(MuBacktrace));
        fill_backtrace(*out, symbols[i]);
        out = &(*out)->up;
    }
             
    *out = NULL;

    return trace;
}
