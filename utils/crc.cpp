//
// Created by kwins on 2020/10/28.
//

#include "crc.h"
namespace naruto::utils {

uint64_t Crc::checksum(uint64_t crc, const unsigned char *s, uint64_t l) {
    uint64_t j;

    for (j = 0; j < l; j++) {
        uint8_t byte = s[j];
        crc = crc64_tab[(uint8_t) crc ^ byte] ^ (crc >> 8);
    }
    return crc;
}

uint16_t Crc::crc16(const char *s, int len) {
    int counter;
    uint16_t crc = 0;
    uint16_t bit = 8;
    for (counter = 0; counter < len; counter++)
        crc = (crc << bit) ^ crc16tab[((crc >> bit) ^ *s++) & 0x00FF];
    return crc;
}

}