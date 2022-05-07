#include <bootloader/bootloader.h>
#include <kernel_task.h>
#include <nvm.h>
#include <sys.h>
#include <util.h>
#include <assert.h>
#include <xc.h>

enum bootloader_state
{
    BOOTLOADER_IDLE,

    BOOTLOADER_ERASE,
    BOOTLOADER_ERASE_PAGE,
    BOOTLOADER_BURN_ROW,
    BOOTLOADER_BOOT,
};

static int bootloader_rtask_init(void);
static void bootloader_rtask_execute(void);
KERN_SIMPLE_RTASK(bootloader, bootloader_rtask_init, bootloader_rtask_execute)

extern unsigned int const __app_mem_start;
extern unsigned int const __app_mem_end;

static unsigned int const * bootloader_app_mem_iterator = &__app_mem_start;
static unsigned int const * const bootloader_app_mem_end = &__app_mem_end;
static unsigned int const * bootloader_row_burn_address;
static unsigned int bootloader_row_offset;

static enum bootloader_state bootloader_state = BOOTLOADER_IDLE;

inline static void __attribute__((always_inline)) bootloader_restore_app_mem_iterator(void)
{
    bootloader_app_mem_iterator = &__app_mem_start;
}

inline static void __attribute__((always_inline)) bootloader_run_app(void)
{
    sys_disable_global_interrupt();

    void (*app_main)(void) = (void (*)(void))__app_mem_start;
    app_main();
}

int bootloader_rtask_init(void)
{
    return KERN_INIT_SUCCESS;
}

void bootloader_rtask_execute(void)
{
    // TODO: eventually check the return code of NVM functions

    switch (bootloader_state) {
        default: // TODO, some other default state
        case BOOTLOADER_IDLE:
            break;

        case BOOTLOADER_ERASE:
            bootloader_restore_app_mem_iterator();
            bootloader_state = BOOTLOADER_ERASE_PAGE;
            // no break
        case BOOTLOADER_ERASE_PAGE:
            nvm_erase_page_virt(bootloader_app_mem_iterator);
            bootloader_app_mem_iterator += NVM_PAGE_SIZE;
            if (bootloader_app_mem_iterator == bootloader_app_mem_end)
                bootloader_state = BOOTLOADER_IDLE;
            break;
        case BOOTLOADER_BURN_ROW:
            nvm_write_row_phys(bootloader_row_burn_address);
            bootloader_row_offset = 0;
            bootloader_state = BOOTLOADER_IDLE;
            break;
        case BOOTLOADER_BOOT:
            bootloader_run_app(); // Never returns
            break;
    }
}

bool bootloader_busy(void)
{
    return bootloader_state != BOOTLOADER_IDLE;
}

bool bootloader_ready(void)
{
    return !bootloader_busy();
}

bool bootloader_info(enum bootloader_info info, unsigned int * out)
{
    switch (info) {
        case BOOTLOADER_INFO_MEM_PHY_START: *out = PHY_ADDR(__app_mem_start);   break;
        case BOOTLOADER_INFO_MEM_PHY_END:   *out = PHY_ADDR(__app_mem_end);     break;
        case BOOTLOADER_INFO_MEM_ROW_SIZE:  *out = NVM_ROW_SIZE;                break;
        default:                                                                return false;
    }

    return true;
}

bool bootloader_erase(void)
{
    if (bootloader_busy())
        return false;

    bootloader_state = BOOTLOADER_ERASE;
    return true;
}

bool bootloader_boot(void)
{
    if (bootloader_busy())
        return false;

    bootloader_state = BOOTLOADER_BOOT;
    return true;
}

void bootloader_row_set_offset(unsigned int offset)
{
    bootloader_row_offset = offset;
}

bool bootloader_row_push_word(unsigned int word)
{
    if (bootloader_row_offset > (NVM_ROW_SIZE - NVM_WORD_SIZE))
        return false;

    nvm_row_buffer[bootloader_row_offset++] = word;
    return true;
}

bool bootloader_row_burn(unsigned int phy_address)
{
    if (bootloader_busy())
        return false;
    if (phy_address < PHY_ADDR(__app_mem_start) ||
        phy_address > (PHY_ADDR(__app_mem_start) - NVM_ROW_SIZE))
        return false;

    bootloader_row_burn_address = (unsigned int *)phy_address;
    bootloader_state = BOOTLOADER_BURN_ROW;
    return true;
}