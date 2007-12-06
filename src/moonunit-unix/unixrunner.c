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

    return false;
}

static void UnixRunner_Run(UnixRunner* runner, const char* path, int setc, char** set)
{
   	MoonUnitLibrary* library = runner->loader->open(path);
	
	runner->logger->library_enter(basename(path));
	
	MoonUnitTest** tests = runner->loader->tests(library);
	
    /* FIXME: it's probably not ok to sort this array in place since
     * it's owned by the loader
     */
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
