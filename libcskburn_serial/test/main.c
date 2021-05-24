#include <unity.h>

void
setUp(void)
{
}

void
tearDown(void)
{
}

void
test_slip_decode(void)
{
	TEST_ASSERT_EQUAL_HEX8(80, 70);
	TEST_ASSERT_EQUAL_HEX8(127, 127);
	TEST_ASSERT_EQUAL_HEX8(84, 84);
}

int
main(int argc, char const *argv[])
{
	UNITY_BEGIN();
	RUN_TEST(test_slip_decode);
	return UNITY_END();
}
