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
#include <moonunit/util.h>
#include <config.h>
#include <string.h>

#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>

static MoonUnitPlugin** loaded_plugins = NULL;
static unsigned int loaded_plugins_size = 0;

static MoonUnitPlugin*
load_plugin_path(const char* prefix, const char* name)
{
    const char* path = format("%s%s%s",
                              prefix,
                              name,
                              PLUGIN_EXTENSION);
    
    void* handle = dlopen(path, RTLD_LAZY);

    MoonUnitPlugin* (*load)(void);

    free((void*) path);

    if (!handle)
    {
        return NULL;
    }

    load = dlsym(handle, "__mu_p_init");

    if (!load)
        return NULL;


    return load();
}

static MoonUnitPlugin*
load_plugin(const char* name)
{
    MoonUnitPlugin* plugin;
    char* pathenv;

    if ((pathenv = getenv("MU_PLUGIN_PATH")))
    {
        pathenv = strdup(pathenv);
        char* path, *next;

        for (path = pathenv; path; path = next)
        {
            char* _path;

            next = strchr(path, ':');
            if (next)
            {
                *(next++) = 0;
            }
            
            _path = format("%s/", path);

            if ((plugin = load_plugin_path(_path, name)))
            {
                free(_path);
                free(pathenv);
                return plugin;
            }
            
            free(_path);
        }
    }
    

    if ((plugin = load_plugin_path(PLUGIN_PATH "/", name)))
        return plugin;
    else if ((plugin = load_plugin_path("", name)))
        return plugin;

    return NULL;
}

static MoonUnitPlugin*
get_plugin(const char* name)
{
    unsigned int index;
    MoonUnitPlugin* plugin;

    for (index = 0; index < loaded_plugins_size; index++)
    {
        if (!strcmp(name, loaded_plugins[index]->name))
        {
            return loaded_plugins[index];
        }
    }

    plugin = load_plugin(name);

    if (plugin)
    {
        loaded_plugins = realloc(loaded_plugins, sizeof(*loaded_plugins) * ++loaded_plugins_size);
        loaded_plugins[loaded_plugins_size-1] = plugin;
    }

    return plugin;
}

struct MoonUnitLoader*
Mu_Plugin_CreateLoader(const char *name)
{
    MoonUnitPlugin* plugin = get_plugin(name);

    if (!plugin)
        return NULL;

    if (!plugin->create_loader)
        return NULL;

    return plugin->create_loader();
}

struct MoonUnitHarness*
Mu_Plugin_CreateHarness(const char *name)
{
    MoonUnitPlugin* plugin = get_plugin(name);

    if (!plugin)
        return NULL;

    if (!plugin->create_harness)
        return NULL;

    return plugin->create_harness();
}

struct MoonUnitLogger*
Mu_Plugin_CreateLogger(const char* name)
{
    MoonUnitPlugin* plugin = get_plugin(name);

    if (!plugin)
        return NULL;

    if (!plugin->create_logger)
        return NULL;

    return plugin->create_logger();
}

struct MoonUnitRunner* 
Mu_Plugin_CreateRunner(const char* name, const char* self, struct MoonUnitLoader* loader, 
                       struct MoonUnitHarness* harness, struct MoonUnitLogger* logger)
{
    MoonUnitPlugin* plugin = get_plugin(name);

    if (!plugin)
        return NULL;

    if (!plugin->create_runner)
        return NULL;

    return plugin->create_runner(self, loader, harness, logger);
}

struct MoonUnitRunner* 
Mu_CreateRunner(const char* name, const char* self, struct MoonUnitLogger* logger)
{
    struct MoonUnitLoader* loader;
    struct MoonUnitHarness* harness;
    
    loader = Mu_Plugin_CreateLoader(name);
   
    if (!loader)
        return NULL;

    harness = Mu_Plugin_CreateHarness(name);

    if (!harness)
        return NULL;

    return Mu_Plugin_CreateRunner(name, self, loader, harness, logger);
}
