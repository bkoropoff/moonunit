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

#include <moonunit/runner.h>
#include <moonunit/loader.h>
#include <moonunit/harness.h>
#include <moonunit/util.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void Mu_Runner_RunAll(MoonUnitRunner* runner, const char* library)
{
    runner->run_all(runner, library);
}

void Mu_Runner_RunSet(MoonUnitRunner* runner, const char* library, int setc, char** set)
{
    runner->run_set(runner, library, setc, set);
}

void Mu_Runner_Option(MoonUnitRunner* runner, const char *name, ...)
{
    va_list ap;
    void* data;
    bool boolean;
    int integer;
    char* string;
    double fpoint;
    void* pointer;

    va_start(ap, name);

    switch (runner->option_type(runner, name))
    {
    case 'b':
        boolean = va_arg(ap, int);
        data = &boolean;
        break;
    case 'i':
        integer = va_arg(ap, int);
        data = &integer;
        break;
    case 's':
        string = va_arg(ap, char*);
        data = string;
        break;
    case 'f':
        fpoint = va_arg(ap, double);
        data = &fpoint;
        break;
    case 'p':
        pointer = va_arg(ap, void*);
        data = pointer;
        break;
    default:
        data = NULL;
        break;
    }

    va_end(ap);

    if (data)
        runner->option(runner, name, data);
}
