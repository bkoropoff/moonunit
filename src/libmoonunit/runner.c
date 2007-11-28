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

#include <moonunit/runner.h>
#include <moonunit/loader.h>
#include <moonunit/harness.h>
#include <moonunit/util.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "gdb.h"

typedef struct UnixRunner
{
    MoonUnitRunner base;

	MoonUnitLoader* loader;
	MoonUnitHarness* harness;
	MoonUnitLogger* logger;
    const char* self;

    struct 
    {
        bool gdb;
    } option;
} UnixRunner;

static int test_compare(const void* _a, const void* _b)
{
	MoonUnitTest* a = *(MoonUnitTest**) _a;
	MoonUnitTest* b = *(MoonUnitTest**) _b;
	int result;
	
	if ((result = strcmp(a->suite, b->suite)))
		return result;
	else
		return (a == b) ? 0 : ((a < b) ? -1 : 1);
}

static unsigned int test_count(MoonUnitTest** tests)
{
	unsigned int result;
	
	for (result = 0; tests[result]; result++);
	
	return result;
}

static char UnixRunner_OptionType(MoonUnitRunner* _runner, const char* name)
{
    if (!strcmp(name, "gdb"))
    {
        return 'b';
    }
    else
    {
        return '\0';
    }
}

static void UnixRunner_Option(MoonUnitRunner* _runner, const char* name, void* _value)
{
    UnixRunner* runner = (UnixRunner*) _runner;

    if (!strcmp(name, "gdb"))
    {
        bool value = *(bool*) _value;
        
        runner->option.gdb = value;
    }
}

static bool in_set(MoonUnitTest* test, int setc, char** set)
{
    unsigned int i;

    for (i = 0; i < setc; i++)
    {
        char* suite_name = set[i];
        char* slash = strchr(set[i], '/');
        char* test_name = slash ? slash+1 : NULL;

        if (test_name)
        {
            if (!strncmp(suite_name, test->suite, slash - suite_name) &&
                !strcmp(test_name, test->name))
                return true;
        }
        else
        {
            if (!strcmp(suite_name, test->suite))
                return true;
        }
    }
}

static void UnixRunner_Run(UnixRunner* runner, const char* path, int setc, char** set)
{
   	MoonUnitLibrary* library = runner->loader->open(path);
	
	runner->logger->library_enter(basename(path));
	
	MoonUnitTest** tests = runner->loader->tests(library);
	
	qsort(tests, test_count(tests), sizeof(*tests), test_compare);
	
	unsigned int index;
	const char* current_suite = NULL;
	MoonUnitThunk thunk;
	
	if ((thunk = runner->loader->library_setup(library)))
		thunk();
	
	for (index = 0; tests[index]; index++)
	{
		MoonUnitTestSummary summary;
		MoonUnitTest* test = tests[index];
        
        if (set != NULL && !in_set(test, setc, set))
            continue;

		if (current_suite == NULL || strcmp(current_suite, test->suite))
		{
			if (current_suite)
				runner->logger->suite_leave();
			current_suite = test->suite;
			runner->logger->suite_enter(test->suite);
		}
		
		runner->harness->dispatch(test, &summary);
		runner->logger->result(test, &summary);

        if (summary.result != MOON_RESULT_SUCCESS && runner->option.gdb)
        {
            pid_t pid = runner->harness->debug(test);
            char* breakpoint;

            if (summary.line)
                breakpoint = format("%s:%u", test->file, summary.line);
            else if (summary.stage == MOON_STAGE_SETUP)
                breakpoint = format("*%p", runner->loader->fixture_setup(test->suite, library));
            else if (summary.stage == MOON_STAGE_TEARDOWN)
                breakpoint = format("*%p", runner->loader->fixture_teardown(test->suite, library));
            else
                breakpoint = format("*%p", test->function);

            gdb_attach_interactive(runner->self, pid, breakpoint);
        }

		runner->harness->cleanup(&summary);
	}
	
	if (current_suite)
		runner->logger->suite_leave();
	runner->logger->library_leave();
	
	if ((thunk = runner->loader->library_teardown(library)))
		thunk();
	
	runner->loader->close(library);
}

static void UnixRunner_RunSet(MoonUnitRunner* _runner, const char* path, int setc, char** set)
{
    UnixRunner_Run((UnixRunner*) _runner, path, setc, set);
}

static void UnixRunner_RunAll(MoonUnitRunner* _runner, const char* path)
{
    UnixRunner_Run((UnixRunner*) _runner, path, 0, NULL);
}

MoonUnitRunner*
Mu_UnixRunner_Create(const char* self, MoonUnitLoader* loader, MoonUnitHarness* harness, MoonUnitLogger* logger)
{
	UnixRunner* runner = malloc(sizeof(*runner));

	runner->loader = loader;
	runner->harness = harness;
	runner->logger = logger;
    runner->self = strdup(self);
	
    runner->base.run_all = UnixRunner_RunAll;
    runner->base.run_set = UnixRunner_RunSet;
    runner->base.option = UnixRunner_Option;
    runner->base.option_type = UnixRunner_OptionType;

    runner->option.gdb = false;

	return (MoonUnitRunner*) runner;
}

void Mu_Runner_RunAll(MoonUnitRunner* runner, const char* library)
{
    runner->run_all(runner, library);
}

void Mu_Runner_RunSet(MoonUnitRunner* runner, const char* library, int setc, char** set)
{
    runner->run_set(runner, library, setc, set);
}

void Mu_Runner_Option(MoonUnitRunner* runner, const char *name, ...)
{
    va_list ap;
    void* data;
    bool boolean;
    int integer;
    char* string;
    double fpoint;
    void* pointer;

    va_start(ap, name);

    switch (runner->option_type(runner, name))
    {
    case 'b':
        boolean = va_arg(ap, int);
        data = &boolean;
        break;
    case 'i':
        integer = va_arg(ap, int);
        data = &integer;
        break;
    case 's':
        string = va_arg(ap, char*);
        data = string;
        break;
    case 'f':
        fpoint = va_arg(ap, double);
        data = &fpoint;
        break;
    case 'p':
        pointer = va_arg(ap, void*);
        data = pointer;
        break;
    default:
        data = NULL;
        break;
    }

    va_end(ap);

    if (data)
        runner->option(runner, name, data);
}
