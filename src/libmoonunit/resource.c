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

#include <moonunit/private/util.h>
#include <string.h>

static hashtable* sections;

static void
section_free(void* key, void* value, void* unused)
{
    free(key);
    hashtable_free((hashtable*) value);
}

/* A section owns its key/value resource pairs */
static void
resource_free(void* key, void* value, void* unused)
{
    free(key);
    free(value);
}

static hashtable*
get_section(const char* name)
{
    hashtable* section;

    if (!sections)
    {
        sections = hashtable_new(511, string_hashfunc, string_hashequal, section_free, NULL);
        section = NULL;
    }
    else
    {
        section = (hashtable*) hashtable_get(sections, (char*) name);
    }

    if (!section)
    {
        section = hashtable_new(511, string_hashfunc, string_hashequal, resource_free, NULL);
        hashtable_set(sections, strdup(name), section);
    }

    return section;
}

const char*
Mu_Resource_Get(const char* section_name, const char* key)
{
    hashtable* section;
    const char* value;

    section = get_section(section_name);
   
    value = (const char*) hashtable_get(section, key);

    return value;
}

const char*
Mu_Resource_Search(char* const section_names[], const char* key)
{
    int i;

    for (i = 0; section_names[i]; i++)
    {
        const char* value = Mu_Resource_Get(section_names[i], key);
        if (value)
            return value;
    }

    return NULL;
}

void
Mu_Resource_Set(const char* section_name, const char* key, const char* value)
{
    hashtable* section;

    section = get_section(section_name);

    hashtable_set(section, strdup(key), strdup(value));
}

char*
Mu_Resource_SectionNameForSuite(const char* suite)
{
    return format("suite:%s", suite);
}

char*
Mu_Resource_SectionNameForLibrary(const char* path)
{
    char* base = strdup(basename_pure(path));
    char* dot = strrchr(base, '.');
    char* result = NULL;

    if (dot)
        *dot = '\0';
    
    result = format("library:%s", base);

    free(base);

    return result;
}
