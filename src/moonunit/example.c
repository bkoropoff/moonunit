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

/**
 * @file example.c
 * @brief Moonunit test structures, constants, and macros
 */

/** \cond SKIP */

//#define MU_HIDE_TESTS

#include <moonunit/interface.h>

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static int x = 0;
static int y = 0;

MU_FIXTURE_SETUP(Arithmetic)
{
	x = 2;
	y = 3;
}

MU_FIXTURE_TEARDOWN(Crash)
{
	if (!strcmp(MU_TOKEN->test->name, "segfault_teardown"))
		*(int*)0 = 42;
}

MU_TEST(Arithmetic, add)
{
	MU_ASSERT_EQUAL(MU_INTEGER, x + y, 5);
}

MU_TEST(Arithmetic, subtract)
{
	MU_ASSERT_EQUAL(MU_INTEGER, x - y, -1);
}

MU_TEST(Arithmetic, multiply)
{
	MU_ASSERT_EQUAL(MU_INTEGER, x * y, 6);
}

MU_TEST(Arithmetic, divide)
{
	MU_ASSERT_EQUAL(MU_INTEGER, y / x, 1);
}

MU_TEST(Arithmetic, bad)
{
	MU_ASSERT_EQUAL(MU_INTEGER, x + y, 4);	
}

MU_TEST(Arithmetic, bad2)
{
	MU_ASSERT(x > y);
}

MU_TEST(Arithmetic, crash)
{
	MU_ASSERT(x / (y - 3) == x);
}

MU_TEST(Crash, segfault)
{
	*(int*)0 = 42;
}

MU_TEST(Crash, segfault_teardown)
{
	// The teardown function for this harness will crash
	// when this test is run
}

MU_TEST(Crash, pipe)
{
	int fd[2];
	
	pipe(fd);
	
	close(fd[0]);
	
	write(fd[1], fd, sizeof(int)*2);
}

MU_TEST(Crash, abort)
{
	abort();
}

MU_TEST(Crash, timeout)
{
    pause();
}

MU_TEST(Log, warning)
{
    MU_WARNING("This is a warning");
}

MU_TEST(Log, info)
{
    MU_INFO("This is informational");
}

MU_TEST(Log, verbose)
{
    MU_VERBOSE("This is verbose output");
}

MU_TEST(Log, trace)
{
    MU_TRACE("This is trace output");
}

/** \endcond */
