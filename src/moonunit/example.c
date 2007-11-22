#define MU_HIDE_TESTS

#include <moonunit/test.h>

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
	if (!strcmp(MU_CURRENT_TEST->name, "segfault_teardown"))
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
