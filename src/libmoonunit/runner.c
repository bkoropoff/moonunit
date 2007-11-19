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
	
	MoonUnitTest** tests = runner->loader->scan(library);
	
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
	runner->loader->cleanup(tests);
	
	if ((thunk = runner->loader->library_teardown(library)))
		thunk();
	
	runner->loader->close(library);
}
