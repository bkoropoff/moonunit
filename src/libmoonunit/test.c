#include <moonunit/test.h>
#include <moonunit/harness.h>
#include <moonunit/util.h>

void
__mu_assert(MoonUnitTest* test, int result, const char* expr,
            const char* file, unsigned int line)
{

    if (result)
        return;
    else
    {
        MoonTestSummary summary;
        
        summary.result = MOON_RESULT_ASSERTION;
        summary.stage = MOON_STAGE_TEST;
        summary.reason = format("Assertion '%s' failed", expr);
        summary.line = line;
        
        test->harness->result(&summary);

        free(summary.reason())
    }
}

void
__mu_success(MoonUnitTest* test)
{
    MoonTestSummary summary;

    summary.result = MOON_RESULT_SUCCESS;
    summary.stage = MOON_STAGE_TEST;
    summary.reason = NULL;
    summary.line = line;

    test->harness->result(&summary);
}
 
void   
__mu_failure(MoonUnitTest*, const char* file, unsigned int line, const char* message, ...)
{
    va_list ap;
    MoonTestSummary summary;

    va_start(ap, message);
    summary.result = MOON_RESULT_FAILURE;
    summary.stage = MOON_STAGE_TEST;
    summary.reason = formatv(message, ap);
    summary.line = line;

    test->harness->result(&summary);

    free(summary.reason);
}
