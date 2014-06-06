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
#include <moonunit/resource.h>
#include <moonunit/library.h>
#include <moonunit/private/interface-private.h>
#include <moonunit/private/util.h>

static MuInterfaceToken* (*current_callback) (void* data) = NULL;
static void* current_callback_data = NULL;

void
mu_interface_expect(MuTestStatus status)
{
    MuInterfaceToken* token = mu_interface_current_token();
    token->meta(token, MU_META_EXPECT, status);
}

void
mu_interface_timeout(long ms)
{
    MuInterfaceToken* token = mu_interface_current_token();
    token->meta(token, MU_META_TIMEOUT, ms);
}

void
mu_interface_iterations(unsigned int count)
{
    MuInterfaceToken* token = mu_interface_current_token();
    token->meta(token, MU_META_ITERATIONS, count);
}

void
mu_interface_event(const char* file, unsigned int line, MuLogLevel level, const char* fmt, ...)
{
    MuInterfaceToken* token = mu_interface_current_token();
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

MuLogLevel
mu_interface_max_log_level(void)
{
    MuInterfaceToken* token = mu_interface_current_token();
    MuLogLevel level = 0;

    token->meta(token, MU_META_LOG_LEVEL, &level);

    return level;
}

void
mu_interface_assert(const char* file, unsigned int line, const char* expr, int sense, int result)
{
    MuInterfaceToken* token = mu_interface_current_token();
    result = result ? 1 : 0;
    sense = sense ? 1 : 0;
    
    if (result == sense)
    {
        return;
    }
    else
    {
        MuTestResult summary;
        
        summary.status = MU_STATUS_ASSERTION;
        summary.reason = sense ? format("Expression was false: %s", expr)
                               : format("Expression was true: %s", expr);
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
            *reason = format("%i didn't equal %i in comparison %s == %s ", a, b, expr, expected);
        else
            *reason = format("both values were %i in comparison %s != %s ", a, expr, expected);
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
            *reason = format("%s didn't equal %s in comparison: %s == %s ", a, b, expr, expected);
        else
            *reason = format("both values were %s in comparison: %s != %s ", a, expr, expected);
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
            *reason = format("%f didn't equal %f in comparison %s == %s ", a, b, expr, expected);
        else
            *reason = format("both values were %f in comparison %s != %s ", a, expr, expected);
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
            *reason = format("%p didn't equal %p in comparison: %s == %s ", a, b, expr, expected);
        else
            *reason = format("both values were %p in comparison: %s != %s ", a, expr, expected);
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
            *reason = format("%s didn't equal %s in comparison %s == %s ", a_str, b_str, expr, expected);
        else
            *reason = format("both values were %s in comparison %s != %s ", a_str, expr, expected);
    }
}

void
mu_interface_assert_equal(const char* file, unsigned int line, 
                         const char* expr1, const char* expr2, 
                         int sense, MuType type, ...)
{
    MuInterfaceToken* token = mu_interface_current_token();
	int result = 0;
	char* reason = NULL;
	
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
        MuTestResult summary = {};
        
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
mu_interface_result(const char* file, unsigned int line, MuTestStatus result, const char* message, ...)
{
    MuInterfaceToken* token = mu_interface_current_token();
    va_list ap;
    MuTestResult summary = {};

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
mu_interface_current_token()
{
    if (current_callback)
        return current_callback(current_callback_data);
    else
        return NULL;
}

void
mu_interface_set_current_token_callback(MuInterfaceToken* (*cb) (void* data), void* data)
{
    current_callback = cb;
    current_callback_data = data;
}

MuTest*
mu_interface_current_test(void)
{
    return mu_interface_current_token()->test;
}

const char*
mu_interface_get_resource(const char* file, unsigned int line, const char* key)
{
    MuInterfaceToken* token = mu_interface_current_token();
    MuTest* test = token->test;
    const char* value = NULL;
    MuTestResult summary;
    
    value = mu_resource_get_for_test(
        mu_library_name(test->library),
        mu_test_suite(test),
        mu_test_name(test),
        key);

    if (!value)
    {
        /* Resource was not available, so fail the test */
        summary.status = MU_STATUS_RESOURCE;
        summary.reason = format("Could not find resource '%s'", key);
        summary.line = line;
        summary.file = file;
        summary.backtrace = NULL;
        
        token->result(token, &summary);
        
        goto error;
    }

error:

    return value;
}

const char*
mu_interface_get_resource_in_section(const char* file, unsigned int line, const char* section, const char* key)
{
    MuInterfaceToken* token = mu_interface_current_token();
    const char* value;
    MuTestResult summary;

    value = mu_resource_get(section, key);

    if (value)
        return value;

    /* Resource was not available, so fail the test */
    summary.status = MU_STATUS_RESOURCE;
    summary.reason = format("Could not find resource in [%s]: %s", section, key);
    summary.line = line;
    summary.file = file;
    summary.backtrace = NULL;
    
    token->result(token, &summary);

    return NULL; 
}
