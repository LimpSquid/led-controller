#include <core/nvm.h>
#include <core/sys.h>
#include <core/util.h>
#include <core/assert.h>
#include <xc.h>
#include <string.h>

#define NVM_KEY_MAGIC_WORD_1        0xAA996655
#define NVM_KEY_MAGIC_WORD_2        0x556699AA

#define NVM_NVMCON_WREN_MASK        BIT(14)
#define NVM_NVMCON_WR_MASK          BIT(15)
#define NVM_NVMCON_OP_MASK          MASK(0xf, 0)
#define NVM_NVMCON_ERR_MASK         (BIT(13) | BIT(12))

enum nvm_operation
{
    nvm_write_word  = 1,
    nvm_write_row   = 3,
    nvm_page_erase  = 4,
};

nvm_word_t nvm_row_buffer[NVM_ROW_BUFFER_SIZE] __attribute__((aligned(NVM_WORD_SIZE)));
STATIC_ASSERT(sizeof(nvm_word_t) == NVM_WORD_SIZE);

static bool nvm_unlock(enum nvm_operation op)
{
    sys_disable_global_interrupt();

    NVMCON = op & NVM_NVMCON_OP_MASK;
    REG_SET(NVMCON, NVM_NVMCON_WREN_MASK);
    NVMKEY = NVM_KEY_MAGIC_WORD_1;
    NVMKEY = NVM_KEY_MAGIC_WORD_2;

    // Start operation and wait until completion
    // Note: it's important to use the atomic SET shadow register
    //       to start the operation, a normal bitwise OR assignment
    //       results in a WRERR
    ATOMIC_REG_SET(NVMCON, NVM_NVMCON_WR_MASK);
    while (NVMCON & NVM_NVMCON_WR_MASK);
    ATOMIC_REG_CLR(NVMCON, NVM_NVMCON_WREN_MASK);

    sys_enable_global_interrupt();

    return !(NVMCON & NVM_NVMCON_ERR_MASK);
}

void nvm_init(void)
{
    nvm_buffer_reset();
}

void nvm_buffer_reset(void)
{
    memset(nvm_row_buffer, 0xff, NVM_ROW_SIZE);
}

bool nvm_erase_page_phy(void const * address)
{
    NVMADDR = (int)address;
    return nvm_unlock(nvm_page_erase);
}

bool nvm_erase_page_virt(void const * address)
{
    NVMADDR = PHY_ADDR(address);
    return nvm_unlock(nvm_page_erase);
}

bool nvm_write_row_phys(void const * address)
{
    NVMADDR = (int)address;
    NVMSRCADDR = PHY_ADDR(nvm_row_buffer);
    bool result = nvm_unlock(nvm_write_row);
    nvm_buffer_reset();
    return result;
}

bool nvm_write_row_virt(void const * address)
{
    NVMADDR = PHY_ADDR(address);
    NVMSRCADDR = PHY_ADDR(nvm_row_buffer);
    bool result = nvm_unlock(nvm_write_row);
    nvm_buffer_reset();
    return result;
}

bool nvm_write_word_phys(void const * address, nvm_word_t word)
{
    NVMADDR = (int)address;
    NVMDATA = word;
    return nvm_unlock(nvm_write_word);
}

bool nvm_write_word_virt(void const * address, nvm_word_t word)
{
    NVMADDR = PHY_ADDR(address);
    NVMDATA = word;
    return nvm_unlock(nvm_write_word);
}