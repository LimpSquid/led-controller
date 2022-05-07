#ifndef NVM_H
#define	NVM_H

#define NVM_WORD_SIZE   4
#define NVM_DWORD_SIZE  (NVM_WORD_SIZE * 2)
#define NVM_ROW_SIZE    512
#define NVM_PAGE_SIZE   (NVM_ROW_SIZE * 8)

// Aligned buffer
extern unsigned char nvm_row_buffer[NVM_ROW_SIZE];

void nvm_init(void);
int nvm_erase_page_phys(void const * address);
int nvm_erase_page_virt(void const * address);
int nvm_write_row_phys(void const * address);
int nvm_write_row_virt(void const * address);

#endif	/* NVM_H */

