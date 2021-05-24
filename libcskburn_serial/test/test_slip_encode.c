#include <unity.h>
#include <slip.h>

#include "test_slip_encode.h"

void
test_slip_encode_00_01_02_03(void)
{
	uint8_t original[] = {0x00, 0x01, 0x02, 0x03};
	uint8_t expected[] = {0xC0, 0x00, 0x01, 0x02, 0x03, 0xC0};

	uint8_t actual[sizeof(original) * 2];

	uint32_t len = slip_encode(original, actual, sizeof(original));

	TEST_ASSERT_EQUAL_UINT32(sizeof(expected), len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, len);
}

void
test_slip_encode_$C0$_01_02_03(void)
{
	uint8_t original[] = {0xC0, 0x01, 0x02, 0x03};
	uint8_t expected[] = {0xC0, 0xDB, 0xDC, 0x01, 0x02, 0x03, 0xC0};

	uint8_t actual[sizeof(original) * 2];

	uint32_t len = slip_encode(original, actual, sizeof(original));

	TEST_ASSERT_EQUAL_UINT32(sizeof(expected), len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, len);
}

void
test_slip_encode_00_$C0$_02_03(void)
{
	uint8_t original[] = {0x00, 0xC0, 0x02, 0x03};
	uint8_t expected[] = {0xC0, 0x00, 0xDB, 0xDC, 0x02, 0x03, 0xC0};

	uint8_t actual[sizeof(original) * 2];

	uint32_t len = slip_encode(original, actual, sizeof(original));

	TEST_ASSERT_EQUAL_UINT32(sizeof(expected), len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, len);
}

void
test_slip_encode_00_01_$C0$_03(void)
{
	uint8_t original[] = {0x00, 0x01, 0xC0, 0x03};
	uint8_t expected[] = {0xC0, 0x00, 0x01, 0xDB, 0xDC, 0x03, 0xC0};

	uint8_t actual[sizeof(original) * 2];

	uint32_t len = slip_encode(original, actual, sizeof(original));

	TEST_ASSERT_EQUAL_UINT32(sizeof(expected), len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, len);
}

void
test_slip_encode_00_01_02_$C0$(void)
{
	uint8_t original[] = {0x00, 0x01, 0x02, 0xC0};
	uint8_t expected[] = {0xC0, 0x00, 0x01, 0x02, 0xDB, 0xDC, 0xC0};

	uint8_t actual[sizeof(original) * 2];

	uint32_t len = slip_encode(original, actual, sizeof(original));

	TEST_ASSERT_EQUAL_UINT32(sizeof(expected), len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, len);
}

void
test_slip_encode_00_$DB$_02_03(void)
{
	uint8_t original[] = {0x00, 0xDB, 0x02, 0x03};
	uint8_t expected[] = {0xC0, 0x00, 0xDB, 0xDD, 0x02, 0x03, 0xC0};

	uint8_t actual[sizeof(original) * 2];

	uint32_t len = slip_encode(original, actual, sizeof(original));

	TEST_ASSERT_EQUAL_UINT32(sizeof(expected), len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, len);
}
