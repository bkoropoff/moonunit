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

#include <moonunit/type.h>

/* Forward declarations */
struct MuTestToken;
struct MuTest;

typedef enum MuTestResult
{
    // Success
    MOON_RESULT_SUCCESS = 0,
    // Generic failure
    MOON_RESULT_FAILURE = 1,
    // Failure due to assertion
    MOON_RESULT_ASSERTION = 2,
    // Failure due to crash (segfault, usually)
    MOON_RESULT_CRASH = 3,
    // Failure due to timeout (infinite loop)
    MOON_RESULT_TIMEOUT = 4
} MuTestResult;

typedef enum MuTestStage
{
    MOON_STAGE_SETUP = 0,
    MOON_STAGE_TEST = 1,
    MOON_STAGE_TEARDOWN = 2,
    MOON_STAGE_UNKNOWN = 3
} MuTestStage;

typedef struct MuTestSummary
{
    MuTestResult result;
    MuTestStage stage;
    const char* reason;
    // Note that we do not store
    // the file since it should be the
    // same as the test.
    unsigned int line;
} MuTestSummary;

typedef enum
{
    MU_LOG_WARNING,
    MU_LOG_INFO,
    MU_LOG_VERBOSE,
    MU_LOG_TRACE
} MuLogLevel;

typedef struct MuLogEvent
{
    /* Stage at which event occurred */
    MuTestStage stage;
    /* File in which event occured */
    const char* file;
    /* Line on which event occured */
    unsigned int line;
    MuLogLevel level;
    const char* message;
} MuLogEvent;

typedef struct MuTestMethods
{
    void (*event)(struct MuTestToken*, MuLogLevel level, const char* file, unsigned int line, const char* fmt, ...);
    void (*assert)(struct MuTestToken*, int result, const char* expr, const char* file, unsigned int line);
    void (*assert_equal)(struct MuTestToken*, const char* expr, const char* expected,
                         const char* file, unsigned int line, MuType type, ...);
    void (*success)(struct MuTestToken*);
    void (*failure)(struct MuTestToken*, const char* file, unsigned int line, const char* message, ...);    
} MuTestMethods;

typedef struct MuTestToken
{
    /* Basic operations */
    void (*result)(struct MuTestToken*, const MuTestSummary*);
    void (*event)(struct MuTestToken*, const MuLogEvent* event);
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

#endif
