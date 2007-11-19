#include <moonunit/test.h>
#include <moonunit/harness.h>
#include <moonunit/util.h>

#include <stdlib.h>

void
__mu_assert(MoonUnitTest* test, int result, const char* expr,
            const char* file, unsigned int line)
{

    if (result)
        return;
    else
    {
        MoonUnitTestSummary summary;
        
        summary.result = MOON_RESULT_ASSERTION;
        summary.stage = MOON_STAGE_TEST;
        summary.reason = format("Assertion '%s' failed", expr);
        summary.line = line;
        
        test->harness->result(test, &summary);

        free((void*) summary.reason);
    }
}

void
__mu_success(MoonUnitTest* test)
{
    MoonUnitTestSummary summary;

    summary.result = MOON_RESULT_SUCCESS;
    summary.stage = MOON_STAGE_TEST;
    summary.reason = NULL;
    summary.line = 0;

    test->harness->result(test, &summary);
}
 
void   
__mu_failure(MoonUnitTest* test, const char* file, unsigned int line, const char* message, ...)
{
    va_list ap;
    MoonUnitTestSummary summary;

    va_start(ap, message);
    summary.result = MOON_RESULT_FAILURE;
    summary.stage = MOON_STAGE_TEST;
    summary.reason = formatv(message, ap);
    summary.line = line;

    test->harness->result(test, &summary);

    free((void*) summary.reason);
}
