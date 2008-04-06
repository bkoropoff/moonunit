/*
 * Copyright (c) 2008, Brian Koropoff
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

#include <string.h>

#include "upopt.h"
#include <moonunit/util.h>

struct UpoptContext
{
    bool only_normal;
    int argc, index;
    char **argv; 
    const UpoptOptionInfo* options;
};

UpoptContext*
Upopt_CreateContext(const UpoptOptionInfo* options, int argc, char** argv)
{
    UpoptContext* context = malloc(sizeof(*context));
    
    context->argc = argc;
    context->argv = argv;
    context->options = options;
    context->index = 1;
    context->only_normal = false;

    return context;
}

static const UpoptOptionInfo*
find_long(const UpoptOptionInfo* info, const char* name)
{
    int i;

    for (i = 0; info[i].constant != UPOPT_ARG_END; i++)
    {
        if (info[i].longname && !strcmp(name, info[i].longname))
            return &info[i];
    }

    return NULL;
}

static const UpoptOptionInfo*
find_short(const UpoptOptionInfo* info, const char letter)
{
    int i;

    for (i = 0; info[i].constant != UPOPT_ARG_END; i++)
    {
        if (info[i].shortname == letter)
            return &info[i];
    }

    return NULL;
}

static inline bool
is_long(const char* arg)
{
    return arg[0] && arg[1] && arg[0] == '-' && arg[1] == '-';
}

static inline bool
is_short(const char* arg)
{
    return arg[0] && arg[1] && arg[0] == '-';
}

static inline bool
is_option(const char* arg)
{
    return is_long(arg) || is_short(arg);
}

UpoptStatus
Upopt_Next(UpoptContext* context, int* constant, const char** value, char** error)
{
    const char* arg;
    const char* val = NULL;
    const UpoptOptionInfo* info = NULL;
   
    if (context->index >= context->argc)
        return UPOPT_STATUS_DONE;

    arg = context->argv[context->index++];

    if (!context->only_normal)
    {
        if (is_long(arg))
        {
            if (!arg[2])
            {
                context->only_normal = 1;
                return Upopt_Next(context, constant, value, error);
            }
            else
            {
                char* equal = strchr(arg, '=');
                
                if (equal)
                {
                    *equal = '\0';
                    val = equal + 1;
                }
                
                info = find_long(context->options, arg+2);
                
                if (equal)
                {
                    *equal = '=';
                }
                
                if (!info)
                {
                    *error = format("Unrecognized option: %s\n", arg);
                    return UPOPT_STATUS_ERROR;
                }
            }
        }
        else if (is_short(arg))
        {
            if (arg[2])
            {
                val = arg+2;
            }

            info = find_short(context->options, arg[1]);
            
            if (!info)
            {
                *error = format("Unrecognized option: %s\n", arg);
                return UPOPT_STATUS_ERROR;
            }
        }
    }
     
    if (info)
    {
        if (val && !info->argument)
        {
            if (is_long(arg))
            {
                *error = format("Did not expect an argument after --%s\n", 
                                info->longname);
            }
            else
            {
                *error = format("Did not expect an argument after -%c\n", 
                                info->shortname);
            }
            return UPOPT_STATUS_ERROR;
        }
        else if (!val && info->argument)
        {
            if ((context->index >= context->argc ||
                 (!context->only_normal &&
                  is_option(context->argv[context->index]))))
            {
                *error = format("Expected argument after %s\n", arg);
                return UPOPT_STATUS_ERROR;
            }
            val = context->argv[context->index++];
        }

        *constant = info->constant;
        *value = val;
        *error = NULL;
        return UPOPT_STATUS_NORMAL;
    }
    else
    {
        *constant = UPOPT_ARG_NORMAL;
        *value = arg;
        *error = NULL;
        return UPOPT_STATUS_NORMAL;
    }
}

void
Upopt_DestroyContext(UpoptContext* context)
{
    free(context);
}
