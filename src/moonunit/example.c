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

#include <moonunit/interface.h>

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

static int x = 0;
static int y = 0;

MU_FIXTURE_SETUP(Arithmetic)
{
	x = 2;
	y = 3;
}

MU_FIXTURE_TEARDOWN(Crash)
{
	if (!strcmp(Mu_Test_Name(MU_CURRENT_TEST), "segfault_teardown"))
		*(int*)0 = 42;
}

MU_TEST(Arithmetic, add)
{
	MU_ASSERT_EQUAL(MU_TYPE_INTEGER, x + y, 5);
}

MU_TEST(Arithmetic, add2)
{
    MU_ASSERT_NOT_EQUAL(MU_TYPE_INTEGER, x + y, 6);
}

MU_TEST(Arithmetic, subtract)
{
	MU_ASSERT_EQUAL(MU_TYPE_INTEGER, x - y, -1);
}

MU_TEST(Arithmetic, multiply)
{
	MU_ASSERT_EQUAL(MU_TYPE_INTEGER, x * y, 6);
}

MU_TEST(Arithmetic, divide)
{
	MU_ASSERT_EQUAL(MU_TYPE_INTEGER, y / x, 1);
}

MU_TEST(Arithmetic, skip)
{
    MU_SKIP("This test is skipped");
}

MU_TEST(Arithmetic, bad)
{
    MU_EXPECT(MU_STATUS_ASSERTION);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, x + y, 4);	
}

MU_TEST(Arithmetic, bad2)
{
    MU_EXPECT(MU_STATUS_ASSERTION);

    MU_ASSERT(x > y);
}

MU_TEST(Arithmetic, crash)
{
    MU_EXPECT(MU_STATUS_CRASH);
    
    MU_ASSERT(x / (y - 3) == x);
}

#if MU_LINK_STYLE != MU_LINK_NONE
unsigned int divide(int a, int b)
{
    MU_ASSERT(b != 0);
    
    return a / b;
}

MU_TEST(Arithmetic, bad_link)
{
    MU_EXPECT(MU_STATUS_ASSERTION);

    divide(5, 0);
}
#endif

MU_TEST(Crash, segfault)
{
    MU_EXPECT(MU_STATUS_CRASH);
    
    *(int*)0 = 42;
}

MU_TEST(Crash, segfault_teardown)
{
    MU_EXPECT(MU_STATUS_CRASH);
    // The teardown function for this fixture will crash
    // when this test is run
}

MU_TEST(Crash, pipe)
{
    int fd[2];
    
    MU_EXPECT(MU_STATUS_CRASH);
    
    pipe(fd);
    close(fd[0]);
    write(fd[1], fd, sizeof(int)*2);
}

MU_TEST(Crash, abort)
{
    MU_EXPECT(MU_STATUS_CRASH);
    abort();
}

MU_TEST(Crash, timeout)
{
    MU_EXPECT(MU_STATUS_TIMEOUT);
    MU_TIMEOUT(100);

    pause();
}

MU_TEST(Crash, not_reached)
{
    MU_EXPECT(MU_STATUS_ASSERTION);
    
    MU_ASSERT_NOT_REACHED;
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

MU_TEST(Log, debug)
{
    MU_DEBUG("This is debug output");
}

MU_TEST(Log, trace)
{
    MU_TRACE("This is trace output");
}

typedef struct
{
    int needed, waiting;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} barrier_t;

static void
barrier_init(barrier_t* barrier, int needed)
{
    barrier->needed = needed;
    barrier->waiting = 0;
    pthread_mutex_init(&barrier->lock, NULL);
    pthread_cond_init(&barrier->cond, NULL);
}

static int
barrier_wait(barrier_t* barrier)
{
    pthread_mutex_lock(&barrier->lock);

    barrier->waiting++;

    if (barrier->waiting == barrier->needed)
    {
        barrier->waiting = 0;
        pthread_cond_broadcast(&barrier->cond);
        pthread_mutex_unlock(&barrier->lock);
        return 1;
    }
    else
    {
        pthread_cond_wait(&barrier->cond, &barrier->lock);
        pthread_mutex_unlock(&barrier->lock);
        return 0;
    }
}

static barrier_t barrier;


static void*
racer(void* number)
{
    barrier_wait(&barrier);
    MU_INFO("Racer #%lu at the finish line", (unsigned long) number);
    MU_SUCCESS;
    return NULL;
}

MU_TEST(Thread, race)
{
    pthread_t racer1, racer2;

    MU_ITERATE(10);

    barrier_init(&barrier, 3);
    
    if (pthread_create(&racer1, NULL, racer, (void*) 0x1))
        MU_FAILURE("Failed to create thread: %s\n", strerror(errno));
    if (pthread_create(&racer2, NULL, racer, (void*) 0x2))
        MU_FAILURE("Failed to create thread: %s\n", strerror(errno));

    barrier_wait(&barrier);

    pthread_join(racer1, NULL);
    pthread_join(racer2, NULL);
}

/** \endcond */
