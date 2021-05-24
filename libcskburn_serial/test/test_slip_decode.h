#ifndef __TEST_SLIP_DECODE__
#define __TEST_SLIP_DECODE__

void test_slip_decode_C0_00_01_02_03_C0(void);
void test_slip_decode_C0_$DB_DC$_01_02_03_C0(void);
void test_slip_decode_C0_00_$DB_DC$_02_03_C0(void);
void test_slip_decode_C0_00_01_$DB_DC$_03_C0(void);
void test_slip_decode_C0_00_01_02_$DB_DC$_C0(void);
void test_slip_decode_C0_00_$DB_DD$_02_03_C0(void);
void test_slip_decode_C0_00_01_02_03_C0_$$_C0_04_05_06_07_C0(void);
void test_slip_decode_C0_00_01_02(void);
void test_slip_decode_C0_00_01_02_03_C0_$$_C0_04_05(void);

#endif  // __TEST_SLIP_DECODE__
