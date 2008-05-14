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

/**
 * @file test.h
 * @brief Test-related types, enums, and structures
 */

/**
 * @defgroup test_bits Types, enumerations, and structures
 * @ingroup test
 */
/* @{ */

C_BEGIN_DECLS

/* Forward declarations */
struct MuTestToken;
struct MuTest;

/**
 * Represents the result of a test
 */
typedef enum MuTestStatus
{
    /** Success */
    MU_STATUS_SUCCESS = 0,
    /** Generic failure */
    MU_STATUS_FAILURE,
    /** Failure due to assertion */
    MU_STATUS_ASSERTION,
    /** Failure due to crash */
    MU_STATUS_CRASH,
    /** Failure due to test exceeding time allowance */
    MU_STATUS_TIMEOUT,
    /** Failure due to uncaught exception */
    MU_STATUS_EXCEPTION,
    /** Test skipped */
    MU_STATUS_SKIPPED,
} MuTestStatus;

/**
 * Represents the stage at which a failure occured
 */
typedef enum MuTestStage
{
    /** Fixture setup */
    MU_STAGE_SETUP,
    /** Test */
    MU_STAGE_TEST,
    /** Fixture teardown */
    MU_STAGE_TEARDOWN,
    /** Stage unknown */
    MU_STAGE_UNKNOWN
} MuTestStage;

#ifndef DOXYGEN
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
#endif

/**
 * Indicates the level of a log event
 */
typedef enum
{
    /** Warning */
    MU_LEVEL_WARNING,
    /** Informational message */
    MU_LEVEL_INFO,
    /** Verbose message */
    MU_LEVEL_VERBOSE,
    /** Trace message */
    MU_LEVEL_TRACE
} MuLogLevel;

#ifndef DOXYGEN
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
    /* Extensible meta-data channel */
    void (*meta)(struct MuTestToken*, MuTestMeta type, ...);
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

const char* Mu_TestStatus_ToString(MuTestStatus status);
void Mu_Test_Expect(MuTestToken* token, MuTestStatus status);
void Mu_Test_Timeout(MuTestToken* token, long ms);
void Mu_Test_Event(MuTestToken* token, MuLogLevel level, const char* file, unsigned int line, const char* fmt, ...);
void Mu_Test_Assert(MuTestToken* token, int result, int sense, const char* expr, const char* file, unsigned int line);
void Mu_Test_AssertEqual(MuTestToken* token, const char* expr, const char* expected, int sense, const char* file, unsigned int line, MuType type, ...);
void Mu_Test_Success(MuTestToken* token);
void Mu_Test_Failure(MuTestToken* token, const char* file, unsigned int line, const char* message, ...);
void Mu_Test_Skip(MuTestToken* token, const char* file, unsigned int line, const char* message, ...);

#endif

C_END_DECLS

/* @} */

#endif
