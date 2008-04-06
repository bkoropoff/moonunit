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
    OPTION_SUITE = 1,
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
#include <popt.h>
#include <stdbool.h>

#include "option.h"

static int
error(OptionTable* table, const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    table->errormsg = formatv(fmt, ap);

    va_end(ap);

    return -2;
}

static const struct poptOption options[] =
{
    {
        .longName = "suite",
        .shortName = 's',
        .argInfo = POPT_ARG_STRING,
        .arg = NULL,
        .val = OPTION_SUITE,
        .descrip = "Run a specific test suite",
        .argDescrip = "suite"
    },
    {
        .longName = "test",
        .shortName = 't',
        .argInfo = POPT_ARG_STRING,
        .arg = NULL,
        .val = OPTION_TEST,
        .descrip = "Run a specific test",
        .argDescrip = "suite/name"
    },
    {
        .longName = "all",
        .shortName = 'a',
        .argInfo = POPT_ARG_NONE,
        .arg = NULL,
        .val = OPTION_ALL,
        .descrip = "Run all tests in all suites (default)",
        .argDescrip = NULL
    },
    {
        .longName = "gdb",
        .shortName = 'g',
        .argInfo = POPT_ARG_NONE,
        .arg = NULL,
        .val = OPTION_GDB,
        .descrip = "Rerun failed tests in an interactive gdb session",
        .argDescrip = NULL
    },
    {
        .longName = "logger",
        .shortName = 'l',
        .argInfo = POPT_ARG_STRING,
        .arg = NULL,
        .val = OPTION_LOGGER,
        .descrip = "Use a specific result logger (default: console)",
        .argDescrip = "name"
    },
    {
        .longName = "iterations",
        .shortName = 'n',
        .argInfo = POPT_ARG_INT,
        .arg = NULL,
        .val = OPTION_ITERATIONS,
        .descrip = "Run each test count iterations",
        .argDescrip = "count"
    },
    {
        .longName = "timeout",
        .shortName = '\0',
        .argInfo = POPT_ARG_INT,
        .arg = NULL,
        .val = OPTION_TIMEOUT,
        .descrip = "Terminate unresponsive tests after t milliseconds",
        .argDescrip = "t"
    },
    {
        .longName = "list-plugins",
        .shortName = '\0',
        .argInfo = POPT_ARG_NONE,
        .arg = NULL,
        .val = OPTION_LIST_PLUGINS,
        .descrip = "List installed plugins and their capabilities",
        .argDescrip = NULL
    },
    {
        .longName = "plugin-info",
        .shortName = '\0',
        .argInfo = POPT_ARG_STRING,
        .arg = NULL,
        .val = OPTION_PLUGIN_INFO,
        .descrip = "Show information about a plugin and its supported options",
        .argDescrip = "name"
    },
    POPT_AUTOHELP
    POPT_TABLEEND
};

int
Option_Parse(int argc, char** argv, OptionTable* option)
{
    poptContext context = poptGetContext("moonunit", argc, (const char**) argv, options, 0);
    int rc;

    option->context = context;

    /* Set defaults */

    option->iterations = 1;
    option->timeout = 2000;
    option->mode = MODE_RUN;

    poptSetOtherOptionHelp(context, "<libraries...>");

    if ((rc = poptReadDefaultConfig(context, 0)))
    {
        rc = error(option, "%s: %s", 
                   poptBadOption(context, 0),
                   poptStrerror(rc));
    } 
    else do
    {
        switch ((rc = poptGetNextOpt(context)))
        {
        case OPTION_SUITE:
        case OPTION_TEST:
        {
            const char* entry = poptGetOptArg(context);

            if (rc == OPTION_SUITE && strchr(entry, '/'))
            {
                rc = error(option, "The --suite option requires an argument without a forward slash");
                break;
            }
            
            if (rc == OPTION_TEST && !strchr(entry, '/'))
            {
                rc = error(option, "The --test option requires an argument with a forward slash");
                break;
            }

            option->tests = array_append(option->tests, (char*) entry);
            break;
        }
        case OPTION_ALL:
            option->all = true;
            break;
        case OPTION_GDB:
            option->gdb = true;
            break;
        case OPTION_LOGGER:
            option->loggers = array_append(option->loggers, strdup(poptGetOptArg(context)));
            break;
        case OPTION_ITERATIONS:
            option->iterations = atoi(poptGetOptArg(context));
            break;
        case OPTION_TIMEOUT:
            option->timeout = atoi(poptGetOptArg(context));
            break;
        case OPTION_LIST_PLUGINS:
            option->mode = MODE_LIST_PLUGINS;
            break;
        case OPTION_PLUGIN_INFO:
            option->mode = MODE_PLUGIN_INFO;
            option->plugin_info = poptGetOptArg(context);
            break;
        case -1:
        {
            const char* file;

            while ((file = poptGetArg(context)))
            {
                option->files = array_append(option->files, (char*) file);
            }
            break;
        }
        case 0:
            break;
        default:
            rc = error(option, "%s: %s",
                       poptBadOption(context, 0),
                       poptStrerror(rc));
        }
    } while (rc > 0);
        
    if (rc == -1 && option->mode == MODE_RUN && !array_size(option->files))
    {
        poptPrintUsage(context, stderr, 0);
        rc = error(option, "Please specify one or more library files");
    }

	return rc != -1;
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

    poptFreeContext(option->context);
}
