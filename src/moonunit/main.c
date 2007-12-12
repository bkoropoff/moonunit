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
#include <moonunit/runner.h>
#include <moonunit/util.h>
#include <moonunit/plugin.h>

#include "option.h"

#define ALIGNMENT 60

#define die(fmt, ...)                               \
    do {                                            \
        fprintf(stderr, fmt "\n", ## __VA_ARGS__);  \
        exit(1);                                    \
    } while (0);                                    \

int main (int argc, char** argv)
{
    MuError* err = NULL;
	MoonUnitRunner* runner;
    unsigned int file_index;
    MoonUnitLogger* logger;
    OptionTable option = {0};

    if (Option_Parse(argc, argv, &option))
    {
        die("Error: %s", option.errormsg);
    }

    logger = option.logger ?
        Mu_Plugin_CreateLogger(option.logger) :
        Mu_Plugin_CreateLogger("console");

    if (!logger)
    {
        die("Error: Could not create logger '%s'", option.logger);
    }
            
    if (Mu_Logger_OptionType(logger, "fd") == MU_INTEGER)
    {
        Mu_Logger_SetOption(logger, "fd", fileno(stdout));
    }
    
    if (Mu_Logger_OptionType(logger, "align") == MU_INTEGER)
    {
        Mu_Logger_SetOption(logger, "align", ALIGNMENT);
    }

    if (Mu_Logger_OptionType(logger, "ansi") == MU_BOOLEAN)
    {
        Mu_Logger_SetOption(logger, "ansi", true);
    }

    if (Option_ApplyToLogger(&option, logger))
    {
        die("Error: %s", option.errormsg);
    }

    runner = Mu_CreateRunner("unix", argv[0], logger);
    
    if (!runner)
    {
        die("Error: Could not create runner '%s'", "unix");
    }

    Mu_Runner_SetOption(runner, "gdb", option.gdb);
            
    for (file_index = 0; file_index < option.files.size; file_index++)
    {
        char* file = option.files.value[file_index];
        
        if (option.all || option.tests.size == 0)
        {
            Mu_Runner_RunAll(runner, file, &err);
        }
        else
        {
            Mu_Runner_RunSet(runner, file, option.tests.size, option.tests.value, &err);
        }

        if (err)
        {
            die("Error: %s", err->message);
        }
    }

    Option_Release(&option);
    Mu_Plugin_DestroyRunner(runner);

	return 0;
}
