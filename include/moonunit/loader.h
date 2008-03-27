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

#ifndef __MU_LOADER_H__
#define __MU_LOADER_H__

#include <moonunit/error.h>
#include <stdbool.h>

struct MuError;
struct MuTest;
struct MuLibrary;
struct MuPlugin;

typedef struct MuLibrary MuLibrary;

typedef void (*MuThunk) (void);
typedef void (*MuTestThunk) (struct MuTest*);

typedef struct MuLoader
{
    struct MuPlugin* plugin;
    // Determines if a library can be opened by this loader
    bool (*can_open) (struct MuLoader*, const char* path);
    // Opens a library and returns a handle
    MuLibrary* (*open) (struct MuLoader*, const char* path, MuError** err);
    // Returns a null-terminated list of unit tests
    struct MuTest** (*tests) (struct MuLoader*, MuLibrary* handle);
    // Returns the library setup routine for handle
    MuThunk (*library_setup)(struct MuLoader*, MuLibrary* handle);
    // Returns the library teardown routine for handle
    MuThunk (*library_teardown)(struct MuLoader*, MuLibrary* handle);
    // Returns the fixture setup routine for suite name in handle
    MuTestThunk (*fixture_setup)(struct MuLoader*, 
                                       const char* name, MuLibrary* handle);
    // Returns the fixture teardown routine for suite name in handle
    MuTestThunk (*fixture_teardown)(struct MuLoader*,
                                          const char* name, MuLibrary* handle);
    // Closes a library
    void (*close) (struct MuLoader*, MuLibrary* handle);
    // Get name of a library
    const char * (*name) (struct MuLoader*, MuLibrary* handle);
} MuLoader;

bool Mu_Loader_CanOpen(struct MuLoader* loader, const char* path);
MuLibrary* Mu_Loader_Open(struct MuLoader* loader, const char* path, MuError** err);
struct MuTest** Mu_Loader_Tests(struct MuLoader* loader, MuLibrary* handle);
MuThunk Mu_Loader_LibrarySetup(struct MuLoader* loader, MuLibrary* handle);
MuThunk Mu_Loader_LibraryTeardown(struct MuLoader* loader, MuLibrary* handle);
MuTestThunk Mu_Loader_FixtureSetup(struct MuLoader* loader, MuLibrary* handle, const char* name);
MuTestThunk Mu_Loader_FixtureTeardown(struct MuLoader* loader, MuLibrary* handle, const char* name);
void Mu_Loader_Close(struct MuLoader* loader, MuLibrary* handle);
const char* Mu_Loader_Name(struct MuLoader* loader, MuLibrary* handle);

#endif
