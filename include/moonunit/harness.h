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

#include <sys/types.h>

struct MoonUnitTest;
struct MoonUnitPlugin;

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
    struct MoonUnitPlugin* plugin;
    // Called by a unit test when it determines its result
    // early (through a failed assertion, etc.).  The structure
    // passed in will be stack-allocated and should be copied if
    // preservation is required
    void (*result)(struct MoonUnitHarness*, struct MoonUnitTest*, const MoonUnitTestSummary*);

    // Called to run a single unit test.  Results should be stored
    // in the passed in MoonTestSummary structure.
    void (*dispatch)(struct MoonUnitHarness*, struct MoonUnitTest*, MoonUnitTestSummary*);

    // Called to run and immediately suspend a unit test in
    // a separate process.  The test can then be traced by
    // a debugger.
    pid_t (*debug)(struct MoonUnitHarness*, struct MoonUnitTest*);

    // Clean up any memory in a MoonTestSummary filled in by
    // a call to dispatch
    void (*cleanup)(struct MoonUnitHarness*, MoonUnitTestSummary*);
} MoonUnitHarness;

const char* Mu_TestResultToString(MoonUnitTestResult result);
const char* Mu_TestStageToString(MoonUnitTestStage stage);
