#include <unity.h>

#include "test_slip_decode.h"
#include "test_slip_encode.h"

void
setUp(void)
{
}

void
tearDown(void)
{
}

int
main(int argc, char const *argv[])
{
	UNITY_BEGIN();

	RUN_TEST(test_slip_decode_C0_00_01_02_03_C0);
	RUN_TEST(test_slip_decode_C0_$DB_DC$_01_02_03_C0);
	RUN_TEST(test_slip_decode_C0_00_$DB_DC$_02_03_C0);
	RUN_TEST(test_slip_decode_C0_00_01_$DB_DC$_03_C0);
	RUN_TEST(test_slip_decode_C0_00_01_02_$DB_DC$_C0);
	RUN_TEST(test_slip_decode_C0_00_$DB_DD$_02_03_C0);
	RUN_TEST(test_slip_decode_C0_00_01_02_03_C0_$$_C0_04_05_06_07_C0);
	RUN_TEST(test_slip_decode_C0_00_01_02);
	RUN_TEST(test_slip_decode_C0_00_01_02_03_C0_$$_C0_04_05);

	RUN_TEST(test_slip_encode_00_01_02_03);
	RUN_TEST(test_slip_encode_$C0$_01_02_03);
	RUN_TEST(test_slip_encode_00_$C0$_02_03);
	RUN_TEST(test_slip_encode_00_01_$C0$_03);
	RUN_TEST(test_slip_encode_00_01_02_$C0$);
	RUN_TEST(test_slip_encode_00_$DB$_02_03);

	return UNITY_END();
}
