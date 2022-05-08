#include <bootloader/bootloader.h>
#include <kernel_task.h>
#include <bus.h>
#include <nvm.h>
#include <sys.h>
#include <util.h>
#include <assert.h>
#include <xc.h>

STATIC_ASSERT(sizeof(unsigned short) == sizeof(crc16_t));
STATIC_ASSERT(sizeof(unsigned int) == sizeof(nvm_word_t));

enum bootloader_state
{
    BOOTLOADER_IDLE,

    BOOTLOADER_ERASE,
    BOOTLOADER_ERASE_PAGE,
    BOOTLOADER_BURN_ROW,
    BOOTLOADER_BOOT,
    BOOTLOADER_BOOT_CRC_VERIFY,
    BOOTLOADER_BOOT_CRC_BURN,
    BOOTLOADER_BOOT_RUN_APP,
};

struct bootloader_flags
{
    // TODO: crc ok?
};

static int bootloader_rtask_init(void);
static void bootloader_rtask_execute(void);
KERN_SIMPLE_RTASK(bootloader, bootloader_rtask_init, bootloader_rtask_execute)

extern unsigned int const __app_mem_start;
extern unsigned int const __app_mem_end;
extern crc16_t const __app_crc16;

static unsigned int const * bootloader_app_mem_iterator = &__app_mem_start;
static unsigned int const * const bootloader_app_mem_end = &__app_mem_end;
static unsigned int const * bootloader_row_burn_address;
static unsigned int bootloader_row_cursor;
static crc16_t bootloader_app_boot_crc;
static crc16_t bootloader_app_mem_crc;
static struct bootloader_flags bootloader_flags;

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
            bootloader_row_cursor = 0;
            bootloader_state = BOOTLOADER_IDLE;
            break;
        case BOOTLOADER_BOOT:
            crc16_reset(&bootloader_app_mem_crc);
            bootloader_restore_app_mem_iterator();
            bootloader_state = BOOTLOADER_BOOT_CRC_VERIFY;
            // no break
        case BOOTLOADER_BOOT_CRC_VERIFY:
            crc16_update(&bootloader_app_mem_crc, bootloader_app_mem_iterator, NVM_PAGE_SIZE);
            bootloader_app_mem_iterator += NVM_PAGE_SIZE;
            if (bootloader_app_mem_iterator == bootloader_app_mem_end) {
                bootloader_state = (bootloader_app_mem_crc == bootloader_app_boot_crc)
                    ? BOOTLOADER_BOOT_CRC_BURN
                    : BOOTLOADER_IDLE; // TODO: set error
            }
            break;
        case BOOTLOADER_BOOT_CRC_BURN:
            nvm_write_word_virt(&__app_crc16, bootloader_app_mem_crc);
            bootloader_state = BOOTLOADER_BOOT_RUN_APP;
            break;
        case BOOTLOADER_BOOT_RUN_APP:
            if (bus_idle()) // Handle all in/out going bus messages before running the main app
                bootloader_run_app();
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
        case BOOTLOADER_INFO_MEM_PHY_START:     *out = PHY_ADDR(__app_mem_start);   break;
        case BOOTLOADER_INFO_MEM_PHY_END:       *out = PHY_ADDR(__app_mem_end);     break;
        case BOOTLOADER_INFO_MEM_WORD_SIZE:     *out = NVM_WORD_SIZE;               break;
        case BOOTLOADER_INFO_MEM_DWORD_SIZE:    *out = NVM_DWORD_SIZE;              break;
        case BOOTLOADER_INFO_MEM_ROW_SIZE:      *out = NVM_ROW_SIZE;                break;
        case BOOTLOADER_INFO_MEM_PAGE_SIZE:     *out = NVM_PAGE_SIZE;               break;
        default:                                                                    return false;
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

bool bootloader_boot(unsigned short app_crc16)
{
    if (bootloader_busy())
        return false;

    bootloader_app_boot_crc = app_crc16;
    bootloader_state = BOOTLOADER_BOOT;
    return true;
}

void bootloader_row_reset()
{
    bootloader_row_cursor = 0;
    nvm_buffer_reset();
}

unsigned short bootloader_row_crc16()
{
    crc16_t crc;
    crc16_reset(&crc);
    crc16_update(&crc, nvm_row_buffer, NVM_ROW_SIZE);
    return crc;
}

bool bootloader_row_push_word(unsigned int word)
{
    if (bootloader_row_cursor >= NVM_ROW_BUFFER_SIZE)
        return false;

    nvm_row_buffer[bootloader_row_cursor++] = word;
    return true;
}

bool bootloader_row_burn(unsigned int phy_address)
{
    if (bootloader_busy())
        return false;
    if (phy_address < PHY_ADDR(__app_mem_start) ||
        phy_address > (PHY_ADDR(__app_mem_end) - NVM_ROW_SIZE))
        return false;

    bootloader_row_burn_address = (unsigned int *)phy_address;
    bootloader_state = BOOTLOADER_BURN_ROW;
    return true;
}