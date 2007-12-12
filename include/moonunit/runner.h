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

#ifndef __MU_RUNNER_H__
#define __MU_RUNNER_H__

struct MoonUnitLoader;
struct MoonUnitHarness;
struct MoonUnitLogger;
struct MoonUnitPlugin;

#include <moonunit/option.h>
#include <moonunit/error.h>

typedef struct MoonUnitRunner
{
    struct MoonUnitPlugin* plugin;
    void (*run_all) (struct MoonUnitRunner*, const char* library, MuError** err);
    void (*run_set) (struct MoonUnitRunner*, const char* library, int setc, char** set, MuError** err);
    MoonUnitOption option;
} MoonUnitRunner;

MoonUnitRunner* Mu_UnixRunner_Create(const char* self, 
                                     struct MoonUnitLoader* loader, 
                                     struct MoonUnitHarness* harness, 
                                     struct MoonUnitLogger* logger);

void Mu_Runner_RunAll(MoonUnitRunner*, const char* library, MuError** err);
void Mu_Runner_RunSet(MoonUnitRunner*, const char* library, int setc, char** set, MuError** err);
void Mu_Runner_SetOption(MoonUnitRunner*, const char *name, ...);

#endif
