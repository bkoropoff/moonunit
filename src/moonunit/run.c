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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "run.h"

#include <moonunit/private/util.h>
#include <moonunit/library.h>
#include <moonunit/error.h>

#include <stdlib.h>
#include <string.h>

static int
test_compare(const void* _a, const void* _b)
{
	MuTest* a = *(MuTest**) _a;
	MuTest* b = *(MuTest**) _b;
	int result;
	
	if ((result = strcmp(mu_test_suite(a), mu_test_suite(b))))
		return result;
	else
	    return strcmp(mu_test_name(a), mu_test_name(b));
}

static unsigned int
test_count(MuTest** tests)
{
	unsigned int result;
	
	for (result = 0; tests[result]; result++);
	
	return result;
}

static bool
in_set(MuTest* test, int setc, char** set)
{
    unsigned int i;
    const char* test_name = mu_test_name(test);
    const char* suite_name = mu_test_suite(test);
    const char* library_name = mu_library_name(test->library);
    char* test_path = format("%s/%s/%s", library_name, suite_name, test_name);
    bool result;

    for (i = 0; i < setc; i++)
    {
        if (match_path(test_path, set[i]))
        {
            result = true;
            goto done;
        }
    }

    result = false;

done:

    if (test_path)
    {
        free(test_path);
    }

    return result;
}

static void
event_proxy_cb(MuLogEvent const* event, void* data)
{
    MuLogger* logger = (MuLogger*) data;

    mu_logger_test_log(logger, event);
}

unsigned int
run_tests(RunSettings* settings, const char* path, int setc, char** set, MuError** _err)
{
    MuError* err = NULL;
    unsigned int failed = 0;
    MuLogger* logger = settings->logger;
    MuLoader* loader = settings->loader;
    MuLibrary* library = NULL;
    MuTest** tests = NULL;

    library = mu_loader_open(loader, path, &err);

    /* Even if library loading failed, log that
       we attempted to visit it */
    mu_logger_library_enter(logger, path, library); 

    MU_CATCH(err, MU_ERROR_LOAD_LIBRARY)
    {
        mu_logger_library_fail(logger, err->message);
        failed++;
        MU_HANDLE(&err);
        goto leave;
    }

    mu_library_construct(library, &err);

    MU_CATCH(err, MU_ERROR_CONSTRUCT_LIBRARY)
    {
        mu_logger_library_fail(logger, err->message);
        failed++;
        MU_HANDLE(&err);
        goto leave;
    }

    MU_CATCH_ALL(err)
    {
        MU_RERAISE_GOTO(error, _err, err);
    }

    tests = mu_library_get_tests(library);
    
    if (tests)
    {
        qsort(tests, test_count(tests), sizeof(*tests), test_compare);
        
        unsigned int index;
        const char* current_suite = NULL;
        
        for (index = 0; tests[index]; index++)
        {
            MuTestResult* summary = NULL;
            MuTest* test = tests[index];

            if (set != NULL && !in_set(test, setc, set))
                continue;
            
            if (current_suite == NULL || strcmp(current_suite, mu_test_suite(test)))
            {
                if (current_suite)
                    mu_logger_suite_leave(logger);
                current_suite = mu_test_suite(test);
                mu_logger_suite_enter(logger, mu_test_suite(test));
            }
            
            mu_logger_test_enter(logger, test);
            summary = loader->dispatch(loader, test, event_proxy_cb, logger,
                                       mu_logger_max_log_level(logger));
            mu_logger_test_leave(logger, test, summary);

            if (summary->status != MU_STATUS_SKIPPED &&
                    summary->status != summary->expected)
                failed++;

            loader->free_result(loader, summary);
        }
        
        if (current_suite)
            mu_logger_suite_leave(logger);
    }

    mu_library_destruct(library, &err);

    MU_CATCH(err, MU_ERROR_DESTRUCT_LIBRARY)
    {
        mu_logger_library_fail(logger, err->message);
        failed++;
        MU_HANDLE(&err);
        goto leave;
    }
    MU_CATCH_ALL(err)
    {
        MU_RERAISE_GOTO(error, _err, err);
    }    

leave:

    mu_logger_library_leave(logger);
    
error:
    if (tests)
        mu_library_free_tests(library, tests);
   
    if (library)
        mu_library_close(library);

    return failed;
}

unsigned int
run_all(RunSettings* settings, const char* path, MuError** _err)
{
    return run_tests(settings, path, 0, NULL, _err);
}

void
print_tests(MuLoader* loader, const char* path, int setc, char** set, MuError** _err)
{
    MuError* err = NULL;
    MuLibrary* library = NULL;
    MuTest** tests = NULL;

    library = mu_loader_open(loader, path, &err);
    MU_PROPAGATE(leave, _err, err);

    tests = mu_library_get_tests(library);

    if (tests)
    {
        qsort(tests, test_count(tests), sizeof(*tests), test_compare);

        unsigned int index;

        for (index = 0; tests[index]; index++)
        {
            MuTest* test = tests[index];

            if (set != NULL && !in_set(test, setc, set))
                continue;

            printf("%s/%s/%s\n", mu_library_name(library), mu_test_suite(test), mu_test_name(test));
        }
    }

leave:

    if (tests)
    {
        mu_library_free_tests(library, tests);
    }

    if (library)
    {
        mu_library_close(library);
    }
}
