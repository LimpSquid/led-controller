#include <nvm.h>
#include <xc.h>
#include <sys.h>
#include <util.h>
#include <assert.h>
#include <string.h>

nvm_word_t nvm_row_buffer[NVM_ROW_BUFFER_SIZE] __attribute__((aligned(NVM_WORD_SIZE)));
STATIC_ASSERT(sizeof(nvm_word_t) == NVM_WORD_SIZE);

static int nvm_unlock(unsigned int nvmop)
{
    sys_disable_global_interrupt();

    NVMCON = nvmop;
    // Write Keys
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    // Start the operation using the Set Register
    NVMCONSET = 0x8000;
    // Wait for operation to complete
    while (NVMCON & 0x8000);

    sys_enable_global_interrupt();

    // Disable NVM write enable
    NVMCONCLR = 0x0004000;
    // Return WRERR and LVDERR Error Status Bits
    return (NVMCON & 0x3000);
}

void nvm_init(void)
{
    nvm_buffer_reset();
}

void nvm_buffer_reset(void)
{
    memset(nvm_row_buffer, 0xff, NVM_ROW_SIZE);
}

int nvm_erase_page_phy(void const * address)
{
    NVMADDR = (int)address;
    return nvm_unlock(0x4004);
}

int nvm_erase_page_virt(void const * address)
{
    NVMADDR = PHY_ADDR(address);
    return nvm_unlock(0x4004);
}

int nvm_write_row_phys(void const * address)
{
    NVMADDR = (int)address;
    NVMSRCADDR = (int)nvm_row_buffer;
    int result = nvm_unlock(0x4003);
    nvm_buffer_reset();
    return result;
}

int nvm_write_row_virt(void const * address)
{
    NVMADDR = PHY_ADDR(address);
    NVMSRCADDR = (int)nvm_row_buffer;
    int result = nvm_unlock(0x4003);
    nvm_buffer_reset();
    return result;
}

int nvm_write_word_phys(void const * address, nvm_word_t word)
{
    NVMADDR = (int)address;
    NVMDATA = word;
    return nvm_unlock(0x4001);
}

int nvm_write_word_virt(void const * address, nvm_word_t word)
{
    NVMADDR = PHY_ADDR(address);
    NVMDATA = word;
    return nvm_unlock(0x4001);
}