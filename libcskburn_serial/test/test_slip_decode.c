#include <unity.h>
#include <slip.h>

#include "test_slip_decode.h"

void
test_slip_decode_C0_00_01_02_03_C0(void)
{
	uint8_t original[] = {0xC0, 0x00, 0x01, 0x02, 0x03, 0xC0};
	uint8_t expected[] = {0x00, 0x01, 0x02, 0x03};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original));
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(5, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, original + offset, len);
}

void
test_slip_decode_C0_$DB_DC$_01_02_03_C0(void)
{
	uint8_t original[] = {0xC0, 0xDB, 0xDC, 0x01, 0x02, 0x03, 0xC0};
	uint8_t expected[] = {0xC0, 0x01, 0x02, 0x03};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original));
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(6, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, original + offset, len);
}

void
test_slip_decode_C0_00_$DB_DC$_02_03_C0(void)
{
	uint8_t original[] = {0xC0, 0x00, 0xDB, 0xDC, 0x02, 0x03, 0xC0};
	uint8_t expected[] = {0x00, 0xC0, 0x02, 0x03};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original));
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(6, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, original + offset, len);
}

void
test_slip_decode_C0_00_01_$DB_DC$_03_C0(void)
{
	uint8_t original[] = {0xC0, 0x00, 0x01, 0xDB, 0xDC, 0x03, 0xC0};
	uint8_t expected[] = {0x00, 0x01, 0xC0, 0x03};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original));
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(6, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, original + offset, len);
}

void
test_slip_decode_C0_00_01_02_$DB_DC$_C0(void)
{
	uint8_t original[] = {0xC0, 0x00, 0x01, 0x02, 0xDB, 0xDC, 0xC0};
	uint8_t expected[] = {0x00, 0x01, 0x02, 0xC0};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original));
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(6, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, original + offset, len);
}

void
test_slip_decode_C0_00_$DB_DD$_02_03_C0(void)
{
	uint8_t original[] = {0xC0, 0x00, 0xDB, 0xDD, 0x02, 0x03, 0xC0};
	uint8_t expected[] = {0x00, 0xDB, 0x02, 0x03};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original));
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(6, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, original + offset, len);
}

void
test_slip_decode_C0_00_01_02_03_C0_$$_C0_04_05_06_07_C0(void)
{
	uint8_t original[] = {
			0xC0, 0x00, 0x01, 0x02, 0x03, 0xC0,  //
			0xC0, 0x04, 0x05, 0x06, 0x07, 0xC0,  //
	};

	uint8_t expected[][4] = {
			{0x00, 0x01, 0x02, 0x03},
			{0x04, 0x05, 0x06, 0x07},
	};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(5, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected[0], original + offset, len);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(5, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected[1], original + offset, len);
}

void
test_slip_decode_C0_00_01_02(void)
{
	uint8_t original[] = {0xC0, 0x00, 0x01, 0x02};
	uint8_t remained[] = {0x00, 0x01, 0x02};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(0, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(remained, original + offset, sizeof(remained));
}

void
test_slip_decode_C0_00_01_02_03_C0_$$_C0_04_05(void)
{
	uint8_t original[] = {
			0xC0, 0x00, 0x01, 0x02, 0x03, 0xC0,  //
			0xC0, 0x04, 0x05,  //
	};

	uint8_t expected[] = {0x00, 0x01, 0x02, 0x03};
	uint8_t remained[] = {0x04, 0x05};

	uint32_t len = 0, offset = 0;
	uint32_t step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(4, len);
	TEST_ASSERT_EQUAL_UINT32(5, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, original + offset, len);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(1, step);

	offset += step;

	step = slip_decode(original + offset, &len, sizeof(original) - offset);
	TEST_ASSERT_EQUAL_UINT32(0, len);
	TEST_ASSERT_EQUAL_UINT32(0, step);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(remained, original + offset, sizeof(remained));
}
