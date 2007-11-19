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
#include <moonunit/loader.h>
#include <moonunit/runner.h>
#include <moonunit/util.h>

#define ALIGNMENT 80

#define die(...)                                \
    do {                                        \
        fprintf(stderr, __VA_ARGS__);           \
        exit(1);                                \
    } while (0);                                \

static void library_enter(const char* name)
{
	printf("Library: %s\n", name);
}

static void library_leave()
{
}

static void suite_enter(const char* name)
{
	printf("    Suite: %s\n", name);
}

static void suite_leave()
{
}

static void result(MoonUnitTest* test, MoonUnitTestSummary* summary)
{
	int i;
	const char* reason;
	char* failure_message;
	printf("        %s:", test->name);
	for (i = ALIGNMENT - strlen(test->name) - 9 - 4; i; i--)
		printf(" ");
	
	switch (summary->result)
	{
		case MOON_RESULT_SUCCESS:
			printf("\e[32mPASS\e[0m\n");
			break;
		case MOON_RESULT_FAILURE:
		case MOON_RESULT_ASSERTION:
		case MOON_RESULT_CRASH:
			printf("\e[31mFAIL\e[0m\n");
			reason = summary->reason ? summary->reason : "unknown";
			failure_message = summary->line != 0 ? 
				  format("%s:%i (%s): %s", test->file, summary->line, Mu_TestStageToString(summary->stage), reason)
				: format("(%s): %s", Mu_TestStageToString(summary->stage), reason);
				
			for (i = ALIGNMENT - strlen(failure_message); i; i--)
				printf(" ");
			printf("%s\n", failure_message);
	}
}

static MoonUnitLogger logger =
{
	library_enter,
	library_leave,
	suite_enter,
	suite_leave,
	result
};

int main (int argc, char** argv)
{
	MoonUnitRunner* runner = Mu_Runner_New(&mu_unixloader, &mu_unixharness, &logger);
	int i;
	
	for (i = 1; i < argc; i++)
	{
		Mu_Runner_RunTests(runner, argv[i]);
	}
	
	return 0;
}
