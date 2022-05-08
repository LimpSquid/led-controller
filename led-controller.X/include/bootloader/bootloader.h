#ifndef BOOTLOADER_H
#define	BOOTLOADER_H

#include <stdbool.h>

enum bootloader_info
{
    BOOTLOADER_INFO_MEM_PHY_START,
    BOOTLOADER_INFO_MEM_PHY_END,
    BOOTLOADER_INFO_MEM_WORD_SIZE,
    BOOTLOADER_INFO_MEM_DWORD_SIZE,
    BOOTLOADER_INFO_MEM_ROW_SIZE,
    BOOTLOADER_INFO_MEM_PAGE_SIZE,
};

bool bootloader_busy(void);
bool bootloader_ready(void);
bool bootloader_info(enum bootloader_info info, unsigned int * out);
bool bootloader_erase(void);
bool bootloader_boot(unsigned short app_crc16);
void bootloader_row_reset();
unsigned short bootloader_row_crc16();
bool bootloader_row_push_word(unsigned int word);
bool bootloader_row_burn(unsigned int phy_address);

#endif	/* BOOTLOADER_H */

