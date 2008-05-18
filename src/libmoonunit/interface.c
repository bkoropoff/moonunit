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

#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_STRING_H
#    include <string.h>
#endif

#include <moonunit/interface.h>
#include <private/interface-private.h>
#include <moonunit/util.h>

static MuInterfaceToken* (*current_callback) (void* data) = NULL;
static void* current_callback_data = NULL;

void
Mu_Interface_Expect(MuTestStatus status)
{
    MuInterfaceToken* token = Mu_Interface_CurrentToken();
    token->meta(token, MU_META_EXPECT, status);
}

void
Mu_Interface_Timeout(long ms)
{
    MuInterfaceToken* token = Mu_Interface_CurrentToken();
    token->meta(token, MU_META_TIMEOUT, ms);
}

void
Mu_Interface_Iterations(unsigned int count)
{
    MuInterfaceToken* token = Mu_Interface_CurrentToken();
    token->meta(token, MU_META_ITERATIONS, count);
}

void
Mu_Interface_Event(const char* file, unsigned int line, MuLogLevel level, const char* fmt, ...)
{
    MuInterfaceToken* token = Mu_Interface_CurrentToken();
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
Mu_Interface_Assert(const char* file, unsigned int line, const char* expr, int sense, int result)
{
    MuInterfaceToken* token = Mu_Interface_CurrentToken();
    result = result ? 1 : 0;
    sense = sense ? 1 : 0;
    
    if (result == sense)
    {
        return;
    }
    else
    {
        MuTestResult summary;
        
        /* Normalize booleans */
        result = result ? 1 : 0;
        sense = sense ? 1 : 0;
            
        summary.status = MU_STATUS_ASSERTION;
        summary.reason = sense ? format("Assertion %s failed", expr)
                               : format("Assertion (not) %s failed", expr);
        summary.file = file;
        summary.line = line;
        summary.backtrace = NULL;

        token->result(token, &summary);

        free((void*) summary.reason);
    }
}

static void
assert_equal_integer(const char* expr, const char* expected, va_list ap, int* result, int sense, char** reason)
{
	int a = va_arg(ap, int);
	int b = va_arg(ap, int);
	
	*result = a == b;
	if (*result != sense)
    {
        if (sense)
            *reason = format("Assertion %s == %s failed (%i != %i)", expr, expected, a, b);
        else
            *reason = format("Assertion %s != %s failed (both %i)", expr, expected, a);
    }
}

static void
assert_equal_string(const char* expr, const char* expected, va_list ap, int* result, int sense, char** reason)
{
	const char* a = va_arg(ap, char*);
	const char* b = va_arg(ap, char*);
	
	*result = !strcmp(a,b);
	if (*result != sense)
    {
        if (sense)
            *reason = format("Assertion %s == %s failed (%s != %s)", expr, expected, a, b);
        else
            *reason = format("Assertion %s != %s failed (both %s)", expr, expected, a);
    }        
}

static void
assert_equal_float(const char* expr, const char* expected, va_list ap, int* result, int sense, char** reason)
{
	double a = va_arg(ap, double);
	double b = va_arg(ap, double);
	
	*result = a == b;
	if (*result != sense)
    {
        if (sense)
            *reason = format("Assertion '%s == %s' failed (%f != %f)", expr, expected, a, b);
        else
            *reason = format("Assertion '%s != %s' failed (both %f)", expr, expected, a);
    }
}

static void
assert_equal_pointer(const char* expr, const char* expected, va_list ap, int* result, int sense, char** reason)
{
	void* a = va_arg(ap, void*);
	void* b = va_arg(ap, void*);
	
	*result = a == b;
	if (*result != sense)
    {
        if (sense)
            *reason = format("Assertion '%s == %s' failed (%p != %p)", expr, expected, a, b);
        else
            *reason = format("Assertion '%s != %s' failed (both %p)", expr, expected, a);
    }
}

static void
assert_equal_boolean(const char* expr, const char* expected, va_list ap, int* result, int sense, char** reason)
{
	bool a = (bool) va_arg(ap, int);
	bool b = (bool) va_arg(ap, int);
    const char* a_str = a ? "true" : "false";
    const char* b_str = b ? "true" : "false";

	*result = a == b;
	if (*result != sense)
    {
        if (sense)
            *reason = format("Assertion '%s == %s' failed (%s != %s)", expr, expected, a_str, b_str);
        else
            *reason = format("Assertion '%s != %s' failed (both %s)", expr, expected, a_str);
    }
}

void
Mu_Interface_AssertEqual(const char* file, unsigned int line, 
                         const char* expr1, const char* expr2, 
                         int sense, MuType type, ...)
{
    MuInterfaceToken* token = Mu_Interface_CurrentToken();
	int result;
	char* reason;
	
	va_list ap;
	
	va_start(ap, type);
	
	switch (type)
	{
    case MU_TYPE_INTEGER:
        assert_equal_integer(expr1, expr2, ap, &result, sense, &reason);
        break;
    case MU_TYPE_STRING:
        assert_equal_string(expr1, expr2, ap, &result, sense, &reason);
        break;
    case MU_TYPE_FLOAT:
        assert_equal_float(expr1, expr2, ap, &result, sense, &reason);
        break;
    case MU_TYPE_POINTER:
        assert_equal_pointer(expr1, expr2, ap, &result, sense, &reason);
        break;
    case MU_TYPE_BOOLEAN:
        assert_equal_boolean(expr1, expr2, ap, &result, sense, &reason);
        break;
    case MU_TYPE_UNKNOWN:
    default:
        result = !sense;
        reason = format("Unsupported type in equality assertion");
        break;
	}
	
    if (result == sense)
    {
        return;
    }
    else
    {
        MuTestResult summary;
        
        summary.status = MU_STATUS_ASSERTION;
        summary.reason = reason;
        summary.line = line;
        summary.file = file;
        summary.backtrace = NULL;

        token->result(token, &summary);

        free(reason);
    }
}

void   
Mu_Interface_Result(const char* file, unsigned int line, MuTestStatus result, const char* message, ...)
{
    MuInterfaceToken* token = Mu_Interface_CurrentToken();
    va_list ap;
    MuTestResult summary;

    va_start(ap, message);
    summary.status = result;
    summary.reason = message ? formatv(message, ap) : NULL;
    summary.line = line;
    summary.file = file;
    summary.backtrace = NULL;

    token->result(token, &summary);
    free((void*) summary.reason);
    va_end(ap);
}

MuInterfaceToken*
Mu_Interface_CurrentToken()
{
    if (current_callback)
        return current_callback(current_callback_data);
    else
        return NULL;
}

void
Mu_Interface_SetCurrentTokenCallback(MuInterfaceToken* (*cb) (void* data), void* data)
{
    current_callback = cb;
    current_callback_data = data;
}

MuTest*
Mu_Interface_CurrentTest(void)
{
    return Mu_Interface_CurrentToken()->test;
}
