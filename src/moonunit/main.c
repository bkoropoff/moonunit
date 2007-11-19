#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include <moonunit/harness.h>
#include <moonunit/test.h>
#include <moonunit/scan.h>

#define die(...)                                \
    do {                                        \
        fprintf(stderr, __VA_ARGS__);           \
        exit(1);                                \
    } while (0);                                \

int main (int argc, char** argv)
{
	MoonScanner* scanner = &mu_unixscanner;
	MoonHarness* harness = &mu_unixharness;
	MoonUnitLibrary* library = scanner->open(argv[1]);
	MoonUnitTest** tests = scanner->scan(library);
	
	unsigned int i;
	
	for (i = 0; tests[i]; i++)
	{
		MoonTestSummary summary;
		printf("Unit test: %s\n", tests[i]->name);
		
		harness->dispatch(tests[i], &summary);
		
		printf("    %s (%s)\n", Mu_TestResultToString(summary.result), summary.reason ? summary.reason : "passed");
	}
	
	return 0;
}
