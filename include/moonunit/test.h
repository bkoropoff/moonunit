/*
 * Copyright (c) 2008, Brian Koropoff
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

#ifndef __MU_TEST_H__
#define __MU_TEST_H__

#include <moonunit/boilerplate.h>
#include <moonunit/type.h>

C_BEGIN_DECLS

/* Forward declarations */
struct MuTestToken;
struct MuTest;

typedef enum MuTestStatus
{
    // Success
    MU_STATUS_SUCCESS = 0,
    // Generic failure
    MU_STATUS_FAILURE = 1,
    // Failure due to assertion
    MU_STATUS_ASSERTION = 2,
    // Failure due to crash (segfault, usually)
    MU_STATUS_CRASH = 3,
    // Failure due to timeout (infinite loop, etc.)
    MU_STATUS_TIMEOUT = 4,
    // Skipped test
    MU_STATUS_SKIPPED = 5,
    // Exception (probably C++)
    MU_STATUS_EXCEPTION = 6
} MuTestStatus;

typedef enum MuTestStage
{
    MU_STAGE_SETUP = 0,
    MU_STAGE_TEST = 1,
    MU_STAGE_TEARDOWN = 2,
    MU_STAGE_UNKNOWN = 3
} MuTestStage;

typedef struct MuBacktrace
{
    unsigned long ret_addr;
    unsigned long ip_offset;
    const char* file_name;
    const char* func_name;
    struct MuBacktrace* up;
} MuBacktrace;

typedef struct MuTestResult
{
    /** Status of the test (pass/fail) */
    MuTestStatus status;
    /** Expected status of the test */
    MuTestStatus expected;
    /** If test failed, the stage at which the failure occured */
    MuTestStage stage;
    /** Human-readable reason why the test failed */
    const char* reason;
    /** File in which the failure occured */
    const char* file;
    /** Line on which the failure occured */
    unsigned int line;
    /** Backtrace, if available */
    MuBacktrace* backtrace;
} MuTestResult;

typedef enum
{
    MU_LEVEL_WARNING,
    MU_LEVEL_INFO,
    MU_LEVEL_VERBOSE,
    MU_LEVEL_TRACE
} MuLogLevel;

typedef struct MuLogEvent
{
    /** Stage at which event occurred */
    MuTestStage stage;
    /** File in which event occured */
    const char* file;
    /** Line on which event occured */
    unsigned int line;
    /** Severity of event */
    MuLogLevel level;
    /** Logged message */
    const char* message;
} MuLogEvent;

typedef struct MuTestMethods
{
    void (*expect)(struct MuTestToken*, MuTestStatus status);
    void (*timeout)(struct MuTestToken*, long ms);
    void (*event)(struct MuTestToken*, MuLogLevel level, const char* file, unsigned int line, const char* fmt, ...);
    void (*assert)(struct MuTestToken*, int result, int sense, const char* expr, const char* file, unsigned int line);
    void (*assert_equal)(struct MuTestToken*, const char* expr, const char* expected, int sense,
                         const char* file, unsigned int line, MuType type, ...);
    void (*success)(struct MuTestToken*);
    void (*failure)(struct MuTestToken*, const char* file, unsigned int line, const char* message, ...);
    void (*skip)(struct MuTestToken*, const char* file, unsigned int line, const char* message, ...);
} MuTestMethods;

typedef enum MuTestMeta
{
    MU_META_EXPECT,
    MU_META_TIMEOUT
} MuTestMeta;

typedef struct MuTestToken
{
    /* Basic operations */
    void (*result)(struct MuTestToken*, const MuTestResult*);
    void (*event)(struct MuTestToken*, const MuLogEvent* event);
    void (*meta)(struct MuTestToken*, MuTestMeta type, ...);
    /* Generic operations */
    MuTestMethods method;
    struct MuTest* test;
} MuTestToken;

typedef struct MuTest
{
    /** Test suite name */
    const char* suite;
    /** Test name */
    const char* name;
    /** Source file where test is located */
    const char* file;
    /** First line of test definition */
    unsigned int line;
    /** Loader which loaded this test */
    struct MuLoader* loader;
    /** Library which contains this test */
    struct MuLibrary* library;
    /** Function to run the test */
    void (*run) (MuTestToken* token);
} MuTest;

typedef struct MuFixtureSetup
{
    const char* name;
    const char* file;
    unsigned int line;
    void (*run) (MuTestToken* token);
} MuFixtureSetup;

typedef struct MuFixtureTeardown
{
    const char* name;
    const char* file;
    unsigned int line;
    void (*run) (MuTestToken* token);
} MuFixtureTeardown;

typedef struct MuLibrarySetup
{
    const char* file;
    unsigned int line;
    void (*run) (void);
} MuLibrarySetup;

typedef struct MuLibraryTeardown
{
    const char* file;
    unsigned int line;
    void (*run) (void);
} MuLibraryTeardown;

typedef void (*MuThunk) (void);
typedef void (*MuTestThunk) (MuTestToken*);

void Mu_TestToken_FillMethods(MuTestToken* token);

const char* Mu_TestStatus_ToString(MuTestStatus status);

C_END_DECLS

#endif
