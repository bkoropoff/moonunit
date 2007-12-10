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

#ifndef __MU_LOADER_H__
#define __MU_LOADER_H__

struct MoonUnitTest;
struct MoonUnitLibrary;
struct MoonUnitPlugin;

typedef struct MoonUnitLibrary MoonUnitLibrary;

typedef void (*MoonUnitThunk) (void);
typedef void (*MoonUnitTestThunk) (struct MoonUnitTest*);

typedef struct MoonUnitLoader
{
    struct MoonUnitPlugin* plugin;
    // Opens a library and returns a handle
    MoonUnitLibrary* (*open) (struct MoonUnitLoader*, const char* path);
    // Returns a null-terminated list of unit tests
    struct MoonUnitTest** (*tests) (struct MoonUnitLoader*, MoonUnitLibrary* handle);
    // Returns the library setup routine for handle
    MoonUnitThunk (*library_setup)(struct MoonUnitLoader*, MoonUnitLibrary* handle);
    // Returns the library teardown routine for handle
    MoonUnitThunk (*library_teardown)(struct MoonUnitLoader*, MoonUnitLibrary* handle);
    // Returns the fixture setup routine for suite name in handle
    MoonUnitTestThunk (*fixture_setup)(struct MoonUnitLoader*, 
                                       const char* name, MoonUnitLibrary* handle);
    // Returns the fixture teardown routine for suite name in handle
    MoonUnitTestThunk (*fixture_teardown)(struct MoonUnitLoader*,
                                          const char* name, MoonUnitLibrary* handle);
    // Closes a library
    void (*close) (struct MoonUnitLoader*, MoonUnitLibrary* handle);
    // Get name of a library
    const char * (*name) (struct MoonUnitLoader*, MoonUnitLibrary* handle);
} MoonUnitLoader;

#endif
