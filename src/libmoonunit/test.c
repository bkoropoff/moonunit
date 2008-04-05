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
__mu_event(MuTestToken* token, MuLogLevel level, const char* file, unsigned int line, const char* fmt, ...)
{
    MuLogEvent event;
    va_list ap;

    va_start(ap, fmt);

    event.level = level;
    event.file = file;
    event.line = line;
    event.message = formatv(fmt, ap);

    va_end(ap);

    token->event(token, &event);

    free((void*) event.message);
}

void
__mu_assert(MuTestToken* token, int result, const char* expr,
            const char* file, unsigned int line)
{

    if (result)
        return;
    else
    {
        MuTestResult summary;
        
        summary.status = MU_STATUS_ASSERTION;
        summary.reason = format("Assertion '%s' failed", expr);
        summary.line = line;
        
        token->result(token, &summary);

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
__mu_assert_equal(MuTestToken* token, const char* expr, const char* expected, 
                  const char* file, unsigned int line, MuType type, ...)
{
	int result;
	char* reason;
	
	va_list ap;
	
	va_start(ap, type);
	
	switch (type)
	{
    case MU_TYPE_INTEGER:
        assert_equal_integer(expr, expected, ap, &result, &reason);
        break;
    case MU_TYPE_STRING:
        assert_equal_string(expr, expected, ap, &result, &reason);
        break;
    case MU_TYPE_FLOAT:
        assert_equal_float(expr, expected, ap, &result, &reason);
    case MU_TYPE_POINTER:
    case MU_TYPE_BOOLEAN:
    case MU_TYPE_UNKNOWN:
    default:
        result = 0;
        reason = format("Unsupported type in equality assertion");
        break;
	}
	
    if (result)
        return;
    else
    {
        MuTestResult summary;
        
        summary.status = MU_STATUS_ASSERTION;
        summary.reason = reason;
        summary.line = line;
        
        token->result(token, &summary);

        free(reason);
    }
}

void
__mu_success(MuTestToken* token)
{
    MuTestResult summary;

    summary.status = MU_STATUS_SUCCESS;
    summary.reason = NULL;
    summary.line = 0;

    token->result(token, &summary);
}
 
void   
__mu_failure(MuTestToken* token, const char* file, unsigned int line, const char* message, ...)
{
    va_list ap;
    MuTestResult summary;

    va_start(ap, message);
    summary.status = MU_STATUS_FAILURE;
    summary.reason = formatv(message, ap);
    summary.line = line;
    summary.file = file;

    token->result(token, &summary);

    free((void*) summary.reason);
}

static MuTestMethods generic_methods =
{
    __mu_event,
	__mu_assert,
	__mu_assert_equal,
	__mu_success,
	__mu_failure
};

void Mu_TestToken_FillMethods(MuTestToken* token)
{
    token->method = generic_methods;
}
