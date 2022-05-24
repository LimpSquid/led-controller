#ifndef BOOTLOADER_H
#define	BOOTLOADER_H

#include <stdbool.h>

enum bootloader_info
{
    // Note: do not change the order, since this is used over the bus protocol
    BOOTLOADER_INFO_MEM_PHY_START   = 0,
    BOOTLOADER_INFO_MEM_PHY_END     = 1,
    BOOTLOADER_INFO_MEM_WORD_SIZE   = 2,
    BOOTLOADER_INFO_MEM_DWORD_SIZE  = 3,
    BOOTLOADER_INFO_MEM_ROW_SIZE    = 4,
    BOOTLOADER_INFO_MEM_PAGE_SIZE   = 5,
};

bool bootloader_busy(void);
bool bootloader_ready(void);
bool bootloader_error(void);
void bootloader_set_magic(unsigned int magic);
bool bootloader_info(enum bootloader_info info, unsigned int * out);
bool bootloader_erase(void);
bool bootloader_boot(unsigned short app_crc16);
bool bootloader_row_reset();
unsigned short bootloader_row_crc16();
bool bootloader_row_push_word(unsigned int word);
bool bootloader_row_burn(unsigned int phy_address);

#endif	/* BOOTLOADER_H */

