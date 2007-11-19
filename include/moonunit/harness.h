#include <moonunit/test.h>

typedef enum MoonUnitTestResult
{
    // Success
    MOON_RESULT_SUCCESS = 0,
    // Generic failure
    MOON_RESULT_FAILURE = 1,
    // Failure due to assertion
    MOON_RESULT_ASSERTION = 2,
    // Failure due to crash (segfault, usually)
    MOON_RESULT_CRASH = 3,
} MoonUnitTestResult;

typedef enum MoonUnitTestStage
{
    MOON_STAGE_SETUP = 0,
    MOON_STAGE_TEST = 1,
    MOON_STAGE_TEARDOWN = 2,
    MOON_STAGE_UNKNOWN = 3
} MoonUnitTestStage;

typedef struct MoonUnitTestSummary
{
    MoonUnitTestResult result;
    MoonUnitTestStage stage;
    const char* reason;
    // Note that we do not store
    // the file since it should be the
    // same as the test.
    unsigned int line;
} MoonUnitTestSummary;

typedef struct MoonUnitHarness
{
    // Called by a unit test when it determines its result
    // early (through a failed assertion, etc.).  The structure
    // passed in will be stack-allocated and should be copied if
    // preservation is required
    void (*result)(MoonUnitTest*, const MoonUnitTestSummary*);

    // Called to run a single unit test.  Results should be stored
    // in the passed in MoonTestSummary structure.
    void (*dispatch)(MoonUnitTest*, MoonUnitTestSummary*);
    // Clean up any memory in a MoonTestSummary filled in by
    // a call to dispatch
    void (*cleanup)(MoonUnitTestSummary*);
} MoonUnitHarness;

typedef struct MoonUnitLogger
{
    void (*library_enter) (const char*);
    void (*library_leave) ();
    void (*suite_enter) (const char*);
    void (*suite_leave) ();
    void (*result) (MoonUnitTest*, MoonUnitTestSummary*);
} MoonUnitLogger;

extern MoonUnitHarness mu_unixharness;

const char* Mu_TestResultToString(MoonUnitTestResult result);
const char* Mu_TestStageToString(MoonUnitTestStage stage);
