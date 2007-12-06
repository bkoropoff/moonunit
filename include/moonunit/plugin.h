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

#ifndef __MU_PLUGIN_H__
#define __MU_PLUGIN_H__

struct MoonUnitLogger;
struct MoonUnitHarness;
struct MoonUnitRunner;
struct MoonUnitLoader;

typedef struct MoonUnitPlugin
{
    const char* name;
    struct MoonUnitLoader*  (*create_loader) ();
    struct MoonUnitHarness* (*create_harness) ();
    struct MoonUnitLogger*  (*create_logger) ();
    struct MoonUnitRunner*  (*create_runner) 
        (const char* self, struct MoonUnitLoader*, struct MoonUnitHarness*, struct MoonUnitLogger*);
} MoonUnitPlugin;

#define MU_PLUGIN_INIT \
    MoonUnitPlugin* __mu_p_init ()

struct MoonUnitLoader* Mu_Plugin_CreateLoader(const char *name);
struct MoonUnitHarness* Mu_Plugin_CreateHarness(const char *name);
struct MoonUnitLogger* Mu_Plugin_CreateLogger(const char* name);
struct MoonUnitRunner* Mu_Plugin_CreateRunner(const char* name, const char* self,
                                              struct MoonUnitLoader*, struct MoonUnitHarness*, struct MoonUnitLogger*);
struct MoonUnitRunner* Mu_CreateRunner(const char* name, const char* self, struct MoonUnitLogger* logger);


#endif
