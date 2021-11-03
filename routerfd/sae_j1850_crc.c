// gcc crc.c -std=c99 -o crctest

#include <inttypes.h>
#include <stdio.h>


uint8_t crcTable[256];

uint8_t CalcCRC(uint8_t * buf, uint8_t len);
void CRCInit(void);

uint8_t buf[] = {0x34, 0x28, 0x00, 0x02, 0x71};

int main() {
        CRCInit();

        printf("0x%2X\n", CalcCRC(buf,sizeof(buf)));

        return 0;
}

uint8_t CalcCRC(uint8_t * buf, uint8_t len) {
        const uint8_t * ptr = buf;
        uint8_t _crc = 0xFF;

        while(len--) _crc = crcTable[_crc ^ *ptr++];

        return ~_crc;
}

void CRCInit(void) {
        uint8_t _crc;
        for (int i = 0; i < 0x100; i++) {
                _crc = i;

                for (uint8_t bit = 0; bit < 8; bit++) _crc = (_crc & 0x80) ? ((_crc << 1) ^ 0x1D) : (_crc << 1);

                crcTable[i] = _crc;
        }
}