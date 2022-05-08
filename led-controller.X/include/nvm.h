#ifndef NVM_H
#define	NVM_H

#define NVM_WORD_SIZE       4
#define NVM_DWORD_SIZE      (NVM_WORD_SIZE * 2)
#define NVM_ROW_SIZE        512
#define NVM_PAGE_SIZE       (NVM_ROW_SIZE * 8)
#define NVM_ROW_BUFFER_SIZE (NVM_ROW_SIZE / NVM_WORD_SIZE)

typedef unsigned int nvm_word_t;

// Aligned buffer
extern nvm_word_t nvm_row_buffer[NVM_ROW_BUFFER_SIZE];

void nvm_init(void);
void nvm_buffer_reset(void);
int nvm_erase_page_phys(void const * address);
int nvm_erase_page_virt(void const * address);
int nvm_write_row_phys(void const * address);
int nvm_write_row_virt(void const * address);
int nvm_write_word_phys(void const * address, nvm_word_t word);
int nvm_write_word_virt(void const * address, nvm_word_t word);

#endif	/* NVM_H */

