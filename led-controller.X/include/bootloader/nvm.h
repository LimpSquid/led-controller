#ifndef NVM_H
#define	NVM_H

#include <stdbool.h>
#include <limits.h>

// All in bytes
#define NVM_WORD_SIZE       4
#define NVM_DWORD_SIZE      (NVM_WORD_SIZE * 2)
#define NVM_ROW_SIZE        512
#define NVM_PAGE_SIZE       (NVM_ROW_SIZE * 8)
#define NVM_ROW_BUFFER_SIZE (NVM_ROW_SIZE / NVM_WORD_SIZE)
#define NVM_WORD_MAX        UINT_MAX

typedef unsigned char nvm_byte_t;
typedef unsigned int nvm_word_t;

// Aligned buffer
extern nvm_word_t nvm_row_buffer[NVM_ROW_BUFFER_SIZE];

void nvm_init(void);
void nvm_buffer_reset(void);
bool nvm_erase_page_phys(void const * address);
bool nvm_erase_page_virt(void const * address);
bool nvm_write_row_phys(void const * address);
bool nvm_write_row_virt(void const * address);
bool nvm_write_word_phys(void const * address, nvm_word_t word);
bool nvm_write_word_virt(void const * address, nvm_word_t word);

#endif	/* NVM_H */

