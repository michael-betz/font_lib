#include "print.h"
#include <stdint.h>

// returns number of characters written to buf
static unsigned udec(uint32_t val, char *buf) {
    char buffer[16];
    char *p = buffer;
    while (val || p == buffer) {
        *(p++) = val % 10;
        val = val / 10;
    }
    unsigned ret = 0;
    while (p != buffer) {
        *buf++ = '0' + *(--p);
        ret++;
    }
    *buf = '\0';
    return ret;
}

static void dec(int32_t val, char *buf) {
    if (val < 0) {
        val = -val;
        *buf++ = '-';
    }
    udec(val, buf);
}

// val 130, n 4
// dp 0:   130
// dp 1:  13.0
// dp 2:  1.30
// dp 3: 0.130
// dp 4: .0130
void udec_dp(uint32_t val, const uint8_t n, const uint8_t dp, char *buf) {
    char buffer[16], *p = buffer;
    unsigned i;
    for (i = 0; i < n; i++) {
        if (i > 0 && i == dp)
            *p++ = '.';
        if (val == 0 && i > dp)  // suppress leading zeros
            *p++ = ' ';
        else
            *p++ = '0' + val % 10;
        val = val / 10;
    }
    if (i == dp)
        *p++ = '.';
    while (p != buffer)
        *buf++ = *(--p);  // reverse the string
    *buf++ = '\0';
}

void dec_dp(int32_t val, const uint8_t n, const uint8_t dp, char *buf) {
    if (val < 0) {
        *buf++ = '-';
        val = -val;
    } else {
        *buf++ = ' ';
    }
    udec_dp(val, n, dp, buf);
}

void udec_fix(uint32_t val, const uint8_t nFract, uint8_t nDigits, char *buf) {
    uint32_t fractMask = ((1 << nFract) - 1);  // mask the fractional part
    unsigned ret = udec(val >> nFract, buf);   // Print the integer part
    buf += ret;
    *buf++ = '.';
    val &= fractMask;  // Convert to fractional part
    while (nDigits-- > 0) {
        val *= 10;
        *buf++ = '0' + (val >> nFract);  // Print digit
        val &= fractMask;                // Convert to fractional part
    }
    *buf++ = '\0';
}

void dec_fix(int32_t val, const uint8_t nFract, uint8_t nDigits, char *buf) {
    if (val < 0) {
        *buf++ = '-';
        val = -val;
    } else {
        *buf++ = ' ';
    }
    udec_fix(val, nFract, nDigits, buf);
}

void udec_hex(uint32_t val, uint8_t digits, char *buf) {
    for (int i = (4 * digits) - 4; i >= 0; i -= 4)
        *buf++ = "0123456789ABCDEF"[(val >> i) % 16];
    *buf++ = '\0';
}
