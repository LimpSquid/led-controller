#ifndef BOOTLOADER_H
#define	BOOTLOADER_H

#include <stdbool.h>

bool bootloader_busy(void);
bool bootloader_ready(void);
bool bootloader_erase(void);
bool bootloader_boot(void);
void bootloader_row_set_offset(unsigned int offset);
bool bootloader_row_push_word(unsigned int word);
bool bootloader_row_burn(unsigned int phy_address);

#endif	/* BOOTLOADER_H */

