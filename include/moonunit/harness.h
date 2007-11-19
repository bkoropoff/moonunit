#include <moonunit/test.h>

typedef enum MoonTestResult
{
    // Success
    MOON_RESULT_SUCCESS = 0,
    // Generic failure
    MOON_RESULT_FAILURE = 1,
    // Failure due to assertion
    MOON_RESULT_ASSERTION = 2,
    // Failure due to crash (segfault, usually)
    MOON_RESULT_CRASH = 3,
} MoonTestResult;

typedef enum MoonTestStage
{
    MOON_STAGE_SETUP = 0,
    MOON_STAGE_TEST = 1,
    MOON_STAGE_TEARDOWN = 2,
} MoonTestStage;

typedef struct MoonTestSummary
{
    MoonTestResult result;
    MoonTestStage stage;
    const char* reason;
    // Note that we do not store
    // the file since it should be the
    // same as the test.
    unsigned int line;
} MoonTestSummary;

typedef struct MoonHarness
{
    // Called by a unit test when it determines its result
    // early (through a failed assertion, etc.).  The structure
    // passed in will be stack-allocated and should be copied if
    // preservation is required
    void (*result)(MoonUnitTest*, const MoonTestSummary*);

    // Called to run a single unit test.  Results should be stored
    // in the passed in MoonTestSummary structure.
    void (*dispatch)(MoonUnitTest*, MoonTestSummary*);
    // Clean up any memory in a MoonTestSummary filled in by
    // a call to dispatch
    void (*cleanup)(MoonTestSummary*);
} MoonHarness;

typedef struct MoonLogger
{
    void (*library_enter) (const char*);
    void (*library_leave) ();
    void (*suite_enter) (const char*);
    void (*suite_leave) ();
    void (*result) (MoonUnitTest*, MoonTestSummary*);
} MoonLogger;

extern MoonHarness mu_unixharness;

const char* Mu_TestResultToString(MoonTestResult result);
