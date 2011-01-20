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
    OPTION_TEST,
    OPTION_ALL,
    OPTION_DEBUG,
    OPTION_LOGGER,
    OPTION_LOADER_OPTION,
    OPTION_ITERATIONS,
    OPTION_TIMEOUT,
    OPTION_LIST_PLUGINS,
    OPTION_PLUGIN_INFO,
    OPTION_RESOURCE,
    OPTION_USAGE,
    OPTION_HELP
};



#include <moonunit/private/util.h>
#include <moonunit/logger.h>
#include <moonunit/loader.h>
#include <moonunit/plugin.h>
#include <moonunit/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#include "option.h"
#include "upopt.h"

static int
error(OptionTable* table, int code, const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    table->errormsg = formatv(fmt, ap);

    va_end(ap);

    return code;
}

#define UPOPT_ERROR(table, ...) error(table, UPOPT_STATUS_ERROR, __VA_ARGS__)

static const struct UpoptOptionInfo options[] =
{
    {
        .longname = "test",
        .shortname = 't',
        .argument = "library/suite/name",
        .constant = OPTION_TEST,
        .description = "Run a specific test or subset of tests (glob allowed)",
    },
    {
        .longname = "all",
        .shortname = 'a',
        .constant = OPTION_ALL,
        .description = "Run all tests (default)",
        .argument = NULL
    },
    {
        .longname = "debug",
        .shortname = 'd',
        .constant = OPTION_DEBUG,
        .description = "Run a specific test directly and exit",
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
        .longname = "resource",
        .shortname = 'r',
        .constant = OPTION_RESOURCE,
        .description = "Load a resource file",
        .argument = "file"
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
option_parse(int argc, char** argv, OptionTable* option)
{
    UpoptContext* context = upopt_create_context(options, argc, argv);
    UpoptStatus rc;
    int constant;
    const char* value;


    upopt_set_info(context, basename_pure(argv[0]), "libraries ...", "Run MoonUnit unit tests");

    /* Set defaults */

    option->iterations = 0;
    option->timeout = 0;
    option->mode = MODE_RUN;

    while ((rc = upopt_next(context, &constant, &value, &option->errormsg)) != UPOPT_STATUS_DONE)
    {
        if (rc == UPOPT_STATUS_ERROR)
        {
            goto error;
        }

        switch (constant)
        {
        case OPTION_TEST:
            option->tests = array_append(option->tests, strdup(value));
            break;
        case OPTION_ALL:
            option->all = true;
            break;
        case OPTION_DEBUG:
            option->debug = true;
            break;
        case OPTION_LOGGER:
            option->loggers = array_append(option->loggers, strdup(value));
            break;
        case OPTION_LOADER_OPTION:
            option->loader_options = array_append(option->loader_options, strdup(value));
            break;
        case OPTION_RESOURCE:
            option->resources = array_append(option->resources, strdup(value));
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
            upopt_print_usage(context, stdout, 80);
            option->mode = MODE_USAGE;
            break;
        case OPTION_HELP:
            upopt_print_help(context, stdout, 80);
            option->mode = MODE_HELP;
            break;
        case UPOPT_ARG_NORMAL:
        {
            option->files = array_append(option->files, strdup(value));
            break;
        }
        }
    }
    
    if (option->mode == MODE_RUN && !array_size(option->files))
    {
        rc = UPOPT_ERROR(option, "No libraries specified");
    }
    
error:
    
    upopt_destroy_context(context);

    return rc != UPOPT_STATUS_DONE;
}

static void
option_parse_plugin_options(const char* str, 
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

    if (target)
        free(target);
}

static void
logger_parse_cb(const char* logger, const char* key, const char* value, void* data)
{
    MuLogger** plogger = (MuLogger**) data;

    if (!key)
    {
        *plogger = mu_plugin_create_logger(logger);
    }
    else if (*plogger && key)
    {
        if (value)
        {
            mu_logger_set_option_string(*plogger, key, value);
        }
        else if (mu_logger_option_type(*plogger, key) == MU_TYPE_BOOLEAN)
        {
            mu_logger_set_option(*plogger, key, true);
        }
    }
}

array*
option_create_loggers(OptionTable* option)
{
    unsigned int index;
    array mu_loggers = NULL;
    MuLogger* logger = NULL;

    for (index = 0; index < array_size(option->loggers); index++)
    {
        option_parse_plugin_options((char*) option->loggers[index], logger_parse_cb, &logger);
        mu_loggers = array_append(mu_loggers, logger);
    }

    return mu_loggers;
}

static void
loader_parse_cb(const char* name, const char* key, const char* value, void* data)
{
    if (key)
    {
        MuLoader* loader = mu_plugin_get_loader_with_name(name);
        
        if (loader)
        {
            if (value)
            {
                mu_loader_set_option_string(loader, key, value);
            }
            else if (mu_loader_option_type(loader, key) == MU_TYPE_BOOLEAN)
            {
                mu_loader_set_option(loader, key, true);
            }
        }
    }
}

void
option_configure_loaders(OptionTable* option)
{
    unsigned int index;

    for (index = 0; index < array_size(option->loader_options); index++)
    {
        option_parse_plugin_options((char*) option->loader_options[index], loader_parse_cb, NULL);
    }
}

static void
add_resource(const char* section, const char* key, const char* value, void* unused)
{
    mu_resource_set(section, key, value);
}

int
option_process_resources(OptionTable* option)
{
    unsigned int index;

    for (index = 0; index < array_size(option->resources); index++)
    {
        const char* filename = (const char*) option->resources[index];
        FILE* file = fopen(filename, "r");
        
        if (!file)
        {
            return error(option, -1, "Could not open resource file %s: %s",
                         filename, strerror(errno));
        }

        ini_read(file, add_resource, NULL);
    }

    return 0;
}

void
option_release(OptionTable* option)
{
    int i;
    if (option->errormsg)
        free(option->errormsg);
    
    for (i = 0; i < array_size(option->files); i++)
        free(option->files[i]);

    array_free(option->files);

    for (i = 0; i < array_size(option->tests); i++)
        free(option->tests[i]);

    array_free(option->tests);

    for (i = 0; i < array_size(option->loggers); i++)
        free(option->loggers[i]);

    array_free(option->loggers);

    for (i = 0; i < array_size(option->resources); i++)
        free(option->resources[i]);

    array_free(option->resources);

    for (i = 0; i < array_size(option->loader_options); i++)
        free(option->loader_options[i]);

    array_free(option->loader_options);
}
