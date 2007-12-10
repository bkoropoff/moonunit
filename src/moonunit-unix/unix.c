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

#include <moonunit/plugin.h>
#include <moonunit/loader.h>
#include <moonunit/harness.h>
#include <moonunit/runner.h>

#include <stdlib.h>

extern struct MoonUnitLoader mu_unixloader;
extern struct MoonUnitHarness mu_unixharness;
extern struct MoonUnitRunner* Mu_UnixRunner_Create(const char*, 
                                                   struct MoonUnitLoader*, 
                                                   struct MoonUnitHarness*, 
                                                   struct MoonUnitLogger*);

static struct MoonUnitLoader*
create_unixloader()
{
    MoonUnitLoader* loader = malloc(sizeof (*loader));

    if (!loader)
        return NULL;

    *loader = mu_unixloader;

    return loader;
}

static void
destroy_unixloader(MoonUnitLoader* loader)
{
    free(loader);
}

static struct MoonUnitHarness*
create_unixharness()
{
    MoonUnitHarness* harness = malloc(sizeof(*harness));

    if (!harness)
        return NULL;

    *harness = mu_unixharness;

    return harness;
}

static void
destroy_unixharness(MoonUnitHarness* harness)
{
    free(harness);
}

static struct MoonUnitRunner*
create_unixrunner(const char* self, struct MoonUnitLoader* loader, 
                  struct MoonUnitHarness* harness, struct MoonUnitLogger* logger)
{
    return Mu_UnixRunner_Create(self, loader, harness, logger);
}

static void
destroy_unixrunner(MoonUnitRunner* runner)
{
    free(runner);
    // FIXME: how do we handle modules owned by the runner?
}

static MoonUnitPlugin plugin =
{
    .name = "unix",
    .create_loader = create_unixloader,
    .destroy_loader = destroy_unixloader,
    .create_harness = create_unixharness,
    .destroy_harness = destroy_unixharness,
    .create_runner = create_unixrunner,
    .destroy_runner = destroy_unixrunner,
    .create_logger = NULL,
    .destroy_logger = NULL,
};

MU_PLUGIN_INIT
{
    return &plugin;
}
