#include <moonunit/test.h>

static int x = 2;
static int y = 3;

MU_FIXTURE_SETUP(Math)
{
	x = 2;
	y = 3;
}

MU_TEST(Math, add)
{
	MU_ASSERT(x + y == 5);
}

MU_TEST(Math, subtract)
{
	MU_ASSERT(x - y == -1);
}

MU_TEST(Math, bad)
{
	MU_ASSERT(x + y == 4);	
}
