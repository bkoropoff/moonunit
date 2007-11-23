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

#include <stdlib.h>
#include <string.h>

struct MoonUnitRunner
{
	MoonUnitLoader* loader;
	MoonUnitHarness* harness;
	MoonUnitLogger* logger;
};

MoonUnitRunner*
Mu_Runner_New(MoonUnitLoader* loader, MoonUnitHarness* harness, MoonUnitLogger* logger)
{
	MoonUnitRunner* runner = malloc(sizeof(*runner));
	runner->loader = loader;
	runner->harness = harness;
	runner->logger = logger;
	
	return runner;
}

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

void Mu_Runner_RunTests(MoonUnitRunner* runner, const char* path)
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
		if (current_suite == NULL || strcmp(current_suite, test->suite))
		{
			if (current_suite)
				runner->logger->suite_leave();
			current_suite = test->suite;
			runner->logger->suite_enter(test->suite);
		}
		
		runner->harness->dispatch(test, &summary);
		runner->logger->result(test, &summary);
		runner->harness->cleanup(&summary);
	}
	
	if (current_suite)
		runner->logger->suite_leave();
	runner->logger->library_leave();
	
	if ((thunk = runner->loader->library_teardown(library)))
		thunk();
	
	runner->loader->close(library);
}
