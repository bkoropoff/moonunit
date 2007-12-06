#include <moonunit/plugin.h>
#include <moonunit/harness.h>

#include <stdlib.h>
#include <stdio.h>

static void library_enter(const char* name)
{
    printf("<library name=\"%s\">\n", name);
}

static void library_leave()
{
	printf("</library>\n");
}

static void suite_enter(const char* name)
{
	printf("  <suite name=\"%s\">\n", name);
}

static void suite_leave()
{
	printf("  </suite>\n");
}

static void result(MoonUnitTest* test, MoonUnitTestSummary* summary)
{
    const char* stage;
	switch (summary->result)
	{
		case MOON_RESULT_SUCCESS:
            printf("    <test name=\"%s\" result=\"pass\"/>\n", test->name);
			break;
		case MOON_RESULT_FAILURE:
		case MOON_RESULT_ASSERTION:
		case MOON_RESULT_CRASH:
			stage = Mu_TestStageToString(summary->stage);

            if (summary->reason)
            {
                printf("    <test name=\"%s\" result=\"fail\" stage=\"%s\">\n",
                       test->name, stage);
                printf("      <![CDATA[%s]]>\n", summary->reason);
                printf("    </test>\n");
            }
            else
            {
                printf("    <test name=\"%s\" result=\"fail\" stage=\"%s\"/>\n",
                       test->name, stage);
            }
	}
}

static MoonUnitLogger xmllogger =
{
	library_enter,
	library_leave,
	suite_enter,
	suite_leave,
	result
};

static MoonUnitLogger*
create_xmllogger()
{
    return &xmllogger;
}

static MoonUnitPlugin plugin =
{
    .name = "xml",
    .create_loader = NULL,
    .create_harness = NULL,
    .create_runner = NULL,
    .create_logger = create_xmllogger
};

MU_PLUGIN_INIT
{
    return &plugin;
}
