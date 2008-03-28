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

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#include <moonunit/harness.h>
#include <moonunit/logger.h>
#include <moonunit/test.h>
#include <moonunit/loader.h>
#include <moonunit/util.h>
#include <moonunit/plugin.h>

#include "option.h"
#include "run.h"
#include "multilog.h"

#define ALIGNMENT 60

#define die(fmt, ...)                               \
    do {                                            \
        fprintf(stderr, fmt "\n", ## __VA_ARGS__);  \
        exit(1);                                    \
    } while (0);                                    \

int main (int argc, char** argv)
{
    MuError* err = NULL;
    unsigned int file_index;
    RunSettings settings;
    OptionTable option = {0};
    array* loggers;

    if (Option_Parse(argc, argv, &option))
    {
        die("Error: %s", option.errormsg);
    }

    loggers = Option_CreateLoggers(&option);

    settings.self = argv[0];
    settings.debug = option.gdb;
    settings.iterations = option.iterations;

    if (array_size(loggers) == 0)
    {
        /* Create default console logger */
        settings.logger = Mu_Plugin_CreateLogger("console");

        if (!settings.logger)
        {
            die("Error: Could not create logger 'console'");
        }
        
        Mu_Logger_SetOption(settings.logger, "ansi", true);
    }
    else if (array_size(loggers) == 1)
    {
        settings.logger = loggers[0];
    }
    else
    {
        settings.logger = create_multilogger(loggers);
    }

    if (!(settings.harness = Mu_Plugin_GetHarness("unix")))
    {
        die("Error: Could not create harness 'unix'");
    }

    if (Mu_Harness_OptionType(settings.harness, "timeout") == MU_INTEGER)
    {
        Mu_Harness_SetOption(settings.harness, "timeout", (int) option.timeout);
    }

    for (file_index = 0; file_index < array_size(option.files); file_index++)
    {
        char* file = option.files[file_index];

        settings.loader = Mu_Plugin_GetLoaderForFile(file);

        if (!settings.loader)
        {
            die("Error: Could not find loader for file %s", basename_pure(file));
        }
        
        if (option.all || array_size(option.tests) == 0)
        {
            run_all(&settings, file, &err);
        }
        else
        {
            run_tests(&settings, file, array_size(option.tests), (char**) option.tests, &err);
        }

        if (err)
        {
            die("Error: %s", err->message);
        }
    }

    Option_Release(&option);
    Mu_Plugin_DestroyLogger(settings.logger);

	return 0;
}
