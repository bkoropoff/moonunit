/*
 * Copyright (c) 2007-2008, Brian Koropoff
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

#include <moonunit/loader.h>
#include <moonunit/library.h>

bool
mu_loader_can_open(struct MuLoader* loader, const char* path)
{
    return loader->can_open(loader, path);
}

MuLibrary*
mu_loader_open(struct MuLoader* loader, const char* path, MuError** err)
{
    return loader->open(loader, path, err);
}

void
mu_loader_set_option(MuLoader* loader, const char *name, ...)
{
    va_list ap;

    va_start(ap, name);

    mu_option_setv(loader->options, loader, name, ap);

    va_end(ap);
}

void 
mu_loader_set_option_string(MuLoader* loader, const char *name, const char *value)
{
    mu_option_set_string(loader->options, loader, name, value);
}

MuType
mu_loader_option_type(MuLoader* loader, const char *name)
{
    return mu_option_type(loader->options, name);
}
