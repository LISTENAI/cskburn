#ifndef __TEST_SLIP_ENCODE__
#define __TEST_SLIP_ENCODE__

void test_slip_encode_00_01_02_03(void);
void test_slip_encode_$C0$_01_02_03(void);
void test_slip_encode_00_$C0$_02_03(void);
void test_slip_encode_00_01_$C0$_03(void);
void test_slip_encode_00_01_02_$C0$(void);
void test_slip_encode_00_$DB$_02_03(void);

#endif  // __TEST_SLIP_ENCODE__
