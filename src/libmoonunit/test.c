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

#include <moonunit/test.h>
#include <moonunit/harness.h>
#include <moonunit/util.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

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
        
        test->harness->result(test->harness, test, &summary);

        free((void*) summary.reason);
    }
}

static void
assert_equal_integer(const char* expr, const char* expected, va_list ap, int* result, char** reason)
{
	int a = va_arg(ap, int);
	int b = va_arg(ap, int);
	
	*result = a == b;
	if (!*result)
		*reason = format("Assertion '%s == %s' failed (%i != %i)", expr, expected, a, b);
}

static void
assert_equal_string(const char* expr, const char* expected, va_list ap, int* result, char** reason)
{
	const char* a = va_arg(ap, char*);
	const char* b = va_arg(ap, char*);
	
	*result = !strcmp(a,b);
	if (!*result)
		*reason = format("Assertion '\"%s\" == \"%s\"' failed (\"%s\" != \"%s\")", expr, expected, a, b);
}

static void
assert_equal_float(const char* expr, const char* expected, va_list ap, int* result, char** reason)
{
	double a = va_arg(ap, double);
	double b = va_arg(ap, double);
	
	*result = a == b;
	if (!*result)
		*reason = format("Assertion '%s == %s' failed (%f != %f)", expr, expected, a, b);
}

void
__mu_assert_equal(MoonUnitTest* test, const char* expr, const char* expected, 
                  const char* file, unsigned int line, MoonUnitType type, ...)
{
	int result;
	char* reason;
	
	va_list ap;
	
	va_start(ap, type);
	
	switch (type)
	{
		case MU_INTEGER:
			assert_equal_integer(expr, expected, ap, &result, &reason);
			break;
		case MU_STRING:
			assert_equal_string(expr, expected, ap, &result, &reason);
			break;
		case MU_FLOAT:
			assert_equal_float(expr, expected, ap, &result, &reason);
	}
	
    if (result)
        return;
    else
    {
        MoonUnitTestSummary summary;
        
        summary.result = MOON_RESULT_ASSERTION;
        summary.stage = MOON_STAGE_TEST;
        summary.reason = reason;
        summary.line = line;
        
        test->harness->result(test->harness, test, &summary);

        free(reason);
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

    test->harness->result(test->harness, test, &summary);
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

    test->harness->result(test->harness, test, &summary);

    free((void*) summary.reason);
}

MoonUnitTestMethods Mu_TestMethods =
{
	__mu_assert,
	__mu_assert_equal,
	__mu_success,
	__mu_failure
};
