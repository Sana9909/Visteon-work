#include <stdio.h>
#include <stdint.h>

typedef union {
    uint32_t raw;     // full 32-bit word
    uint8_t  bytes[4]; // individual bytes
} CANFrame;

int main() {
    CANFrame f;
    scanf("%x", &f.raw);  // read hex value into raw

    // print each byte in little-endian order
    for (int i = 0; i < 4; i++) {
        printf("Byte%d: 0x%02X\n", i, f.bytes[i]);
    }

    return 0;
}
