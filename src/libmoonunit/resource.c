/*
 * Copyright (c) Brian Koropoff
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

#include "config.h"
#include <moonunit/resource.h>
#include <moonunit/private/util.h>
#include <string.h>

typedef struct MuResourceSection
{
    char* name;
    hashtable* contents;
} MuResourceSection;

static hashtable* section_map = NULL;
static MuResourceSection** section_array = NULL;

static void
section_free(void* key, void* value, void* unused)
{
    MuResourceSection* section = (MuResourceSection*) value;
    free(section->name);  
    hashtable_free(section->contents);
    free(section);
}

/* A section owns its key/value resource pairs */
static void
resource_free(void* key, void* value, void* unused)
{
    free(key);
    free(value);
}

static MuResourceSection*
get_section(const char* name)
{
    MuResourceSection* section = NULL;

    if (!section_map)
    {
        section_map = hashtable_new(511, string_hashfunc, string_hashequal, section_free, NULL);
    }
    else
    {
        section = hashtable_get(section_map, (char*) name);
    }

    if (!section)
    {
        section = xmalloc(sizeof(*section));
        section->name = strdup(name);
        section->contents = hashtable_new(511, string_hashfunc, string_hashequal, resource_free, NULL);
        hashtable_set(section_map, section->name, section);
        section_array = (MuResourceSection**) array_append((array*) section_array, section);
    }

    return section;
}

const char*
mu_resource_get(const char* section_name, const char* key)
{
    MuResourceSection* section;
    const char* value;

    section = get_section(section_name);
   
    value = (const char*) hashtable_get(section->contents, key);

    return value;
}

void
mu_resource_set(const char* section_name, const char* key, const char* value)
{
    MuResourceSection* section;

    section = get_section(section_name);

    hashtable_set(section->contents, strdup(key), strdup(value));
}

bool
mu_resource_iterate_sections(MuResourceSectionIter iter, void* data)
{
    size_t i;

    for (i = 0; i < array_size((array*) section_array); i++)
    {
        if (iter(section_array[i]->name, data))
        {
            return true;
        }
    }

    return false;
}

typedef struct search_info
{
    char* test_path;
    const char* key;
    const char* value;
} search_info;

static bool
mu_resource_get_resource_section(const char* section, void* data)
{
    search_info* info = (search_info*) data;
    if (match_path(info->test_path, section))
    {
        info->value = mu_resource_get(section, info->key);

        if (info->value)
            return true;
    }
    
    return false;
}

const char*
mu_resource_get_for_test(const char* library, const char* suite, const char* test, const char* key)
{
    search_info info = {NULL, NULL, NULL};
    
    info.key = key;
    info.test_path = format("%s/%s/%s", library, suite, test);

    /* If we don't find anything through pattern matching,
       fall back on the global section */
    if (!mu_resource_iterate_sections(mu_resource_get_resource_section, &info))
    {
        info.value = mu_resource_get("global", key);
    }

    if (info.test_path)
    {
        free(info.test_path);
    }

    return info.value;
}

void
mu_resource_shutdown(void)
{
    if (section_map)
    {
        hashtable_free(section_map);
    }

    if (section_array)
    {
        array_free((array*) section_array);
    }
}
