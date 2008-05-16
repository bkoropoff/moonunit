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
#include <moonunit/test.h>
#include <moonunit/plugin.h>
#include <moonunit/option.h>
#include <stdbool.h>
#include <sys/types.h>

struct MuLoader;

typedef struct MuLibrary
{
    struct MuLoader* loader;
} MuLibrary;

typedef void (*MuLogCallback)(MuLogEvent* event, void* data);

typedef struct MuLoader
{
    struct MuPlugin* plugin;
    MuOption* options;
    // Determines if a library can be opened by this loader
    bool (*can_open) (struct MuLoader*, const char* path);
    // Opens a library and returns a handle
    MuLibrary* (*open) (struct MuLoader*, const char* path, MuError** err);
    // Returns a null-terminated list of unit tests
    struct MuTest** (*get_tests) (struct MuLoader*, MuLibrary* handle);
    // Frees a list of unit tests that had been returned by get_tests
    void (*free_tests) (struct MuLoader*, MuLibrary* handle, struct MuTest** list);
    // Closes a library
    void (*close) (struct MuLoader*, MuLibrary* handle);
    // Get name of a library
    const char* (*library_name) (struct MuLoader*, MuLibrary* handle);
    // Get the name of a test
    const char* (*test_name) (struct MuLoader*, struct MuTest*);
    // Get the suite of a test
    const char* (*test_suite) (struct MuLoader*, struct MuTest*);
    // Dispatch a single test
    MuTestResult* (*dispatch)(struct MuLoader*, struct MuTest*, MuLogCallback, void*);
    // Free the result of a unit test
    void (*free_result)(struct MuLoader*, MuTestResult*);
    // Called to run and immediately suspend a unit test in
    // a separate process.  The test can then be traced by
    // a debugger.
    pid_t (*debug)(struct MuLoader*, struct MuTest*, MuTestStage, void**);
} MuLoader;

bool Mu_Loader_CanOpen(MuLoader* loader, const char* path);
MuLibrary* Mu_Loader_Open(MuLoader* loader, const char* path, MuError** err);
struct MuTest** Mu_Library_GetTests(MuLibrary* handle);
void Mu_Library_FreeTests(MuLibrary* handle, struct MuTest**);
void Mu_Library_Close(MuLibrary* handle);
const char* Mu_Library_Name(MuLibrary* handle);
void Mu_Loader_SetOption(MuLoader* loader, const char *name, ...);
void Mu_Loader_SetOptionString(MuLoader* loader, const char *name, const char *value);
MuType Mu_Loader_OptionType(MuLoader* loader, const char *name);

#endif
