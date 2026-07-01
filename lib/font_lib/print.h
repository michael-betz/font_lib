#pragma once
//-------------------------------------------------------------
// Fixed point print functions
//-------------------------------------------------------------
#include <stdint.h>

// convert to decimal with fixed number of digits + decimal point
// example for val: 130, n: 4
// dp 0:   130
// dp 1:  13.0
// dp 2:  1.30
// dp 3: 0.130
// dp 4: .0130
void udec_dp(uint32_t val, const uint8_t n, const uint8_t dp, char *buf);
void dec_dp(int32_t val, const uint8_t n, const uint8_t dp, char *buf);

// Write a fixed point fractional number to buf
// with `nFract` fractional bits and print only `nDigits` after the .
char *udec_fix(uint32_t val, const uint8_t nFract, uint8_t nDigits, char *buf);
char *dec_fix(int32_t val, const uint8_t nFract, uint8_t nDigits, char *buf);

// Write a hex number to buf
char *udec_hex(uint32_t val, uint8_t digits, char *buf);
