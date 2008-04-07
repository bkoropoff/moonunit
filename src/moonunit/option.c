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
    OPTION_OPTION,
    OPTION_ITERATIONS,
    OPTION_TIMEOUT,
    OPTION_LIST_PLUGINS,
    OPTION_PLUGIN_INFO
};



#include <moonunit/util.h>
#include <moonunit/logger.h>
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
        .argument = "name"
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
    UPOPT_END
};

int
Option_Parse(int argc, char** argv, OptionTable* option)
{
    UpoptContext* context = Upopt_CreateContext(options, argc, argv);
    UpoptStatus rc;
    int constant;
    const char* value;

    /* Set defaults */

    option->iterations = 1;
    option->timeout = 2000;
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

array*
Option_CreateLoggers(OptionTable* option)
{
    unsigned int index;
    array mu_loggers = NULL;

    for (index = 0; index < array_size(option->loggers); index++)
    {
        char* logger_name = strdup((char*) option->loggers[index]);
        char* colon = strchr(logger_name, ':');
        char* options = NULL;
        MuLogger* logger;

        if (colon)
        {
            *colon = '\0';
            options = colon+1;
        }

        logger = Mu_Plugin_CreateLogger(logger_name);

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
                    Mu_Logger_SetOptionString(logger, o, equal+1);
                }
                else if (Mu_Logger_OptionType(logger, o) == MU_TYPE_BOOLEAN)
                {
                    Mu_Logger_SetOption(logger, o, true);
                }
            }
        }

        mu_loggers = array_append(mu_loggers, logger);
    }

    return mu_loggers;
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
