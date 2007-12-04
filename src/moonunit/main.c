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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <popt.h>
#include <stdbool.h>

#include <moonunit/harness.h>
#include <moonunit/test.h>
#include <moonunit/loader.h>
#include <moonunit/runner.h>
#include <moonunit/util.h>

#define ALIGNMENT 60

#define die(fmt, ...)                               \
    do {                                            \
        fprintf(stderr, fmt "\n", ## __VA_ARGS__);  \
        exit(1);                                    \
    } while (0);                                    \

static void library_enter(const char* name)
{
	printf("Library: %s\n", name);
}

static void library_leave()
{
	printf("\n");
}

static void suite_enter(const char* name)
{
	printf("  Suite: %s\n", name);
}

static void suite_leave()
{
	printf("\n");
}

static void result(MoonUnitTest* test, MoonUnitTestSummary* summary)
{
	int i;
	const char* reason, * stage;
	char* failure_message;
	printf("    %s:", test->name);
	
	switch (summary->result)
	{
		case MOON_RESULT_SUCCESS:
			for (i = ALIGNMENT - strlen(test->name) - 5 - 4; i > 0; i--)
				printf(" ");
			printf("\e[32mPASS\e[0m\n");
			break;
		case MOON_RESULT_FAILURE:
		case MOON_RESULT_ASSERTION:
		case MOON_RESULT_CRASH:
			stage = Mu_TestStageToString(summary->stage);
			
			for (i = ALIGNMENT - strlen(test->name) - strlen(stage) - 3 - 5 - 4; i > 0; i--)
				printf(" ");
			
			reason = summary->reason ? summary->reason : "unknown";
			printf("(%s) \e[31mFAIL\e[0m\n", stage);
			
			failure_message = summary->line != 0 
				? format("%s:%i: %s", basename(test->file), summary->line, reason)
				: format("%s", reason);
			
			for (i = ALIGNMENT - strlen(failure_message); i > 0; i--)
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


static int option_gdb = 0;
//static char* option_gdb_break  = NULL;
static bool option_all = false;

static char const ** test_set = NULL;

#define OPTION_SUITE 1
#define OPTION_TEST 2

static const struct poptOption options[] =
{
    {
        .longName = "suite",
        .shortName = 's',
        .argInfo = POPT_ARG_STRING,
        .arg = NULL,
        .val = OPTION_SUITE,
        .descrip = "Run a specific test suite",
        .argDescrip = "<suite>"
    },
    {
        .longName = "test",
        .shortName = 't',
        .argInfo = POPT_ARG_STRING,
        .arg = NULL,
        .val = OPTION_TEST,
        .descrip = "Run a specific test",
        .argDescrip = "<suite>/<name>"
    },
    {
        .longName = "all",
        .shortName = 'a',
        .argInfo = POPT_ARG_NONE,
        .arg = &option_all,
        .val = 0,
        .descrip = "Run all tests in all suites (default)",
        .argDescrip = "<suite>"
    },
    {
        .longName = "gdb",
        .shortName = 'g',
        .argInfo = POPT_ARG_NONE,
        .arg = &option_gdb,
        .val = 0,
        .descrip = "Rerun failed tests in an interactive gdb session",
        .argDescrip = NULL
    },
/*
Not presently implemented
    {
        .longName = "break",
        .shortName = '\0',
        .argInfo = POPT_ARG_STRING,
        .arg = &option_gdb_break,
        .val = 0,
        .descrip = "Specify breakpoint to use for interactive gdb sessions",
        .argDescrip = "< function | line >"
    },
*/
    POPT_AUTOHELP
    POPT_TABLEEND
};

int main (int argc, char** argv)
{
	MoonUnitRunner* runner = Mu_UnixRunner_Create(argv[0], &mu_unixloader, &mu_unixharness, &logger);
    poptContext context = poptGetContext("moonunit", argc, (const char**) argv, options, 0);
	
    int processed = 0;
    int rc;
    int test_index = 0;
    int test_capacity = 32;

    test_set = malloc(sizeof(*test_set) * test_capacity);

    do
    {
        switch ((rc = poptGetNextOpt(context)))
        {
        case OPTION_SUITE:
        case OPTION_TEST:
        {
            const char* entry = poptGetOptArg(context);

            if (rc == OPTION_SUITE && strchr(entry, '/'))
                die("The --suite option requires an argument without a forward slash");
            
            if (rc == OPTION_TEST && !strchr(entry, '/'))
                die("The --test option requires an argument with a forward slash");

            test_set[test_index++] = entry;
            if (test_index >= test_capacity - 1)
            {
                test_capacity *= 2;
                test_set = realloc(test_set, sizeof(*test_set) * test_capacity);
            }
            break;
        }
        case -1:
        {
            const char* file;
            
            Mu_Runner_Option(runner, "gdb", option_gdb);
            
            test_set[test_index] = NULL;

            while ((file = poptGetArg(context)))
            {
                if (option_all || test_index == 0)
                    Mu_Runner_RunAll(runner, file);
                else
                    Mu_Runner_RunSet(runner, file, test_index, (char**) test_set);
                processed++;
            }
            break;
        }
        case 0:
            break;
        }
    } while (rc > 0);
        
    if (!processed)
    {
        poptPrintUsage(context, stderr, 0);
    }

    poptFreeContext(context);

	return rc != -1;
}
