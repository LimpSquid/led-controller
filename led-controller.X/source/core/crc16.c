#include <core/util.h>
#include <core/assert.h>

#define CRC16_POLYNOMIAL    0x1021
#define CRC16_SEED          0xffff

void crc16_reset(crc16_t * crc)
{
    ASSERT_NOT_NULL(crc);

    *crc = CRC16_SEED;
}

void crc16_update(crc16_t * crc, void const * data, unsigned int size)
{
    ASSERT_NOT_NULL(crc);
    ASSERT_NOT_NULL(data);
    ASSERT(size > 0);

    unsigned char const * byte = (unsigned char const *)data;

    bool b;
    for (unsigned int i = 0; i < size; ++i) {
        *crc ^= *byte;
        for (unsigned int j = 0; j < 8; ++j) {
            b = (*crc & 0x0001);
            *crc >>= 1;
            if (b)
                *crc ^= CRC16_POLYNOMIAL;
        }
        ++byte;
    }
}