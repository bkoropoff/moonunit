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

enum
{
    OPTION_SUITE,
    OPTION_TEST,
    OPTION_ALL,
    OPTION_GDB,
    OPTION_LOGGER,
    OPTION_LOADER_OPTION,
    OPTION_ITERATIONS,
    OPTION_TIMEOUT,
    OPTION_LIST_PLUGINS,
    OPTION_PLUGIN_INFO,
    OPTION_USAGE,
    OPTION_HELP
};



#include <moonunit/util.h>
#include <moonunit/logger.h>
#include <moonunit/loader.h>
#include <moonunit/plugin.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <stdbool.h>

#include "option.h"
#include "upopt.h"

static UpoptStatus
error(OptionTable* table, const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    table->errormsg = formatv(fmt, ap);

    va_end(ap);

    return UPOPT_STATUS_ERROR;
}

static const struct UpoptOptionInfo options[] =
{
    {
        .longname = "suite",
        .shortname = 's',
        .description = "Run a specific test suite",
        .argument = "suite",
        .constant = OPTION_SUITE
    },
    {
        .longname = "test",
        .shortname = 't',
        .argument = "suite/name",
        .constant = OPTION_TEST,
        .description = "Run a specific test",
    },
    {
        .longname = "all",
        .shortname = 'a',
        .constant = OPTION_ALL,
        .description = "Run all tests in all suites (default)",
        .argument = NULL
    },
    {
        .longname = "gdb",
        .shortname = 'g',
        .constant = OPTION_GDB,
        .description = "Rerun failed tests in an interactive gdb session",
        .argument = NULL
    },
    {
        .longname = "logger",
        .shortname = 'l',
        .constant = OPTION_LOGGER,
        .description = "Use a specific result logger (default: console)",
        .argument = "name:opt=val,..."
    },
    {
        .longname = "loader-option",
        .shortname = '\0',
        .constant = OPTION_LOADER_OPTION,
        .description = "Set options for a particular loader plugin",
        .argument = "name:opt=val,..."
    },
    {
        .longname = "iterations",
        .shortname = 'n',
        .constant = OPTION_ITERATIONS,
        .description = "Run each test count iterations",
        .argument = "count"
    },
    {
        .longname = "timeout",
        .shortname = '\0',
        .constant = OPTION_TIMEOUT,
        .description = "Terminate unresponsive tests after t milliseconds",
        .argument = "t"
    },
    {
        .longname = "list-plugins",
        .shortname = '\0',
        .constant = OPTION_LIST_PLUGINS,
        .description = "List installed plugins and their capabilities",
        .argument = NULL
    },
    {
        .longname = "plugin-info",
        .shortname = '\0',
        .constant = OPTION_PLUGIN_INFO,
        .description = "Show information about a plugin and its supported options",
        .argument = "name"
    },
    {
        .longname = "usage",
        .shortname = '\0',
        .constant = OPTION_USAGE,
        .description = "Show usage information",
        .argument = NULL
    },
    {
        .longname = "help",
        .shortname = 'h',
        .constant = OPTION_HELP,
        .description = "Show detailed help information",
        .argument = NULL
    },
    UPOPT_END
};

int
Option_Parse(int argc, char** argv, OptionTable* option)
{
    UpoptContext* context = Upopt_CreateContext(options, argc, argv);
    UpoptStatus rc;
    int constant;
    const char* value;


    Upopt_SetInfo(context, basename_pure(argv[0]), "libraries ...", "Run MoonUnit unit tests");

    /* Set defaults */

    option->iterations = 1;
    option->timeout = 0;
    option->mode = MODE_RUN;

    while ((rc = Upopt_Next(context, &constant, &value, &option->errormsg)) != UPOPT_STATUS_DONE)
    {
        if (rc == UPOPT_STATUS_ERROR)
        {
            goto error;
        }

        switch (constant)
        {
        case OPTION_SUITE:
        case OPTION_TEST:
        {
            if (constant == OPTION_SUITE && strchr(value, '/'))
            {
                rc = error(option, "The --suite option requires an argument without a forward slash");
                goto error;
            }
            
            if (constant == OPTION_TEST && !strchr(value, '/'))
            {
                rc = error(option, "The --test option requires an argument with a forward slash");
                goto error;
            }

            option->tests = array_append(option->tests, (char*) value);
            break;
        }
        case OPTION_ALL:
            option->all = true;
            break;
        case OPTION_GDB:
            option->gdb = true;
            break;
        case OPTION_LOGGER:
            option->loggers = array_append(option->loggers, strdup(value));
            break;
        case OPTION_LOADER_OPTION:
            option->loader_options = array_append(option->loader_options, strdup(value));
            break;
        case OPTION_ITERATIONS:
            option->iterations = atoi(value);
            break;
        case OPTION_TIMEOUT:
            option->timeout = atoi(value);
            break;
        case OPTION_LIST_PLUGINS:
            option->mode = MODE_LIST_PLUGINS;
            break;
        case OPTION_PLUGIN_INFO:
            option->mode = MODE_PLUGIN_INFO;
            option->plugin_info = value;
            break;
        case OPTION_USAGE:
            Upopt_PrintUsage(context, stdout, 80);
            option->mode = MODE_USAGE;
            break;
        case OPTION_HELP:
            Upopt_PrintHelp(context, stdout, 80);
            option->mode = MODE_HELP;
            break;
        case UPOPT_ARG_NORMAL:
        {
            option->files = array_append(option->files, (char*) value);
            break;
        }
        }
    }
    
    if (option->mode == MODE_RUN && !array_size(option->files))
    {
        rc = error(option, "No libraries specified");
    }
    
error:
    
    Upopt_DestroyContext(context);

    return rc != UPOPT_STATUS_DONE;
}

static void
Option_ParsePluginOptions(const char* str, 
                          void (*cb)(const char* target, const char* key, const char* value, void* data),
                          void* data)
{
    char* target = strdup(str);
    char* colon = strchr(target, ':');
    char* options = NULL;
    
    if (colon)
    {
        *colon = '\0';
        options = colon+1;
    }

    cb(target, NULL, NULL, data);

    if (options)
    {
        char* o, *next;
        
        for (o = options; o; o = next)
        {
            char* equal;
            next = strchr(o, ',');
            if (next)
            {
                *(next++) = '\0';
            }
            
            equal = strchr(o, '=');
            
            if (equal)
            {
                *equal = '\0';
                
                cb(target, o, equal+1, data);
            }
            else
            {
                cb(target, o, NULL, data);
            }
        }
    }
}

static void
logger_parse_cb(const char* logger, const char* key, const char* value, void* data)
{
    MuLogger** plogger = (MuLogger**) data;

    if (!key)
    {
        *plogger = Mu_Plugin_CreateLogger(logger);
    }
    else if (*plogger && key)
    {
        if (value)
        {
            Mu_Logger_SetOptionString(*plogger, key, value);
        }
        else if (Mu_Logger_OptionType(*plogger, key) == MU_TYPE_BOOLEAN)
        {
            Mu_Logger_SetOption(*plogger, key, true);
        }
    }
}

array*
Option_CreateLoggers(OptionTable* option)
{
    unsigned int index;
    array mu_loggers = NULL;
    MuLogger* logger = NULL;

    for (index = 0; index < array_size(option->loggers); index++)
    {
        Option_ParsePluginOptions((char*) option->loggers[index], logger_parse_cb, &logger);
        mu_loggers = array_append(mu_loggers, logger);
    }

    return mu_loggers;
}

static void
loader_parse_cb(const char* name, const char* key, const char* value, void* data)
{
    if (key)
    {
        MuLoader* loader = Mu_Plugin_GetLoaderWithName(name);
        
        if (loader)
        {
            if (value)
            {
                Mu_Loader_SetOptionString(loader, key, value);
            }
            else if (Mu_Loader_OptionType(loader, key) == MU_TYPE_BOOLEAN)
            {
                Mu_Loader_SetOption(loader, key, true);
            }
        }
    }
}

void
Option_ConfigureLoaders(OptionTable* option)
{
    unsigned int index;

    for (index = 0; index < array_size(option->loader_options); index++)
    {
        Option_ParsePluginOptions((char*) option->loader_options[index], loader_parse_cb, NULL);
    }
}

void
Option_Release(OptionTable* option)
{
    if (option->errormsg)
        free(option->errormsg);
    array_free(option->files);
    array_free(option->tests);
    array_free(option->loggers);
}
