#include <bootloader/bootloader.h>
#include <core/kernel_task.h>
#include <core/bus.h>
#include <core/nvm.h>
#include <core/timer.h>
#include <core/sys.h>
#include <core/util.h>
#include <core/assert.h>
#include <xc.h>

STATIC_ASSERT(sizeof(unsigned short) == sizeof(crc16_t));
STATIC_ASSERT(sizeof(unsigned int) == sizeof(nvm_word_t));

#define BOOTLOADER_BOOT_MAGIC_WINDOW    500 // In ms
#define BOOTLOADER_BOOT_MAGIC           0x0B00B1E5
#define BOOTLOADER_CRC_STEP_SIZE        (NVM_WORD_SIZE * 8)

enum bootloader_state
{
    BOOTLOADER_INIT,
    BOOTLOADER_WAIT_BOOT_MAGIC,
    BOOTLOADER_IDLE,

    BOOTLOADER_ERASE,
    BOOTLOADER_ERASE_PAGE,
    BOOTLOADER_BURN_ROW,
    BOOTLOADER_BOOT,
    BOOTLOADER_BOOT_CRC_VERIFY,
    BOOTLOADER_BOOT_CRC_BURN,
    BOOTLOADER_BOOT_RUN_APP,

    BOOTLOADER_ERROR,
};

static int bootloader_rtask_init(void);
static void bootloader_rtask_execute(void);
KERN_SIMPLE_RTASK(bootloader, bootloader_rtask_init, bootloader_rtask_execute) // FIXME: uncommenting this will crash the CPU, investigate...

extern unsigned int const __app_mem_start;
extern unsigned int const __app_mem_end;
extern unsigned int const __app_entry_addr;
extern crc16_t const __app_crc16;

static nvm_byte_t const * bootloader_app_mem_iterator;
static nvm_byte_t const * bootloader_app_mem_end;
static void const * bootloader_row_burn_address;
static unsigned int bootloader_row_cursor;
static unsigned int bootloader_magic;
static crc16_t bootloader_app_boot_crc;
static crc16_t bootloader_app_mem_crc;
static crc16_t bootloader_nvm_row_crc;
static struct timer_module * bootloader_timer;

static enum bootloader_state bootloader_state = BOOTLOADER_INIT;

inline static void __attribute__((always_inline)) bootloader_restore_app_mem_iterator(void)
{
    bootloader_app_mem_iterator = (nvm_byte_t const *)__app_mem_start;
}

inline static void __attribute__((always_inline)) bootloader_run_app(void)
{
    SYS_TUCK_IN_BONZO();
    sys_disable_global_interrupt();

    void (*app)(void) = (void (*)(void))__app_entry_addr;
    app();
}

int bootloader_rtask_init(void)
{
    // FIXME: Not ideal to do this here, but needed since this is not a constant expression
    bootloader_app_mem_iterator = (nvm_byte_t const *)__app_mem_start;
    bootloader_app_mem_end = (nvm_byte_t const *)__app_mem_end;

    // Initialize NVM row crc
    crc16_reset(&bootloader_nvm_row_crc);

    // Initialize timer
    bootloader_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if (bootloader_timer == NULL)
        goto fail_timer;

    return KERN_INIT_SUCCESS;

fail_timer:

    return KERN_INIT_FAILED;
}

void bootloader_rtask_execute(void)
{
    switch (bootloader_state) {
        case BOOTLOADER_INIT:
            timer_start(bootloader_timer, BOOTLOADER_BOOT_MAGIC_WINDOW, TIMER_TIME_UNIT_MS);
            bootloader_state = BOOTLOADER_WAIT_BOOT_MAGIC;
            break;
        case BOOTLOADER_WAIT_BOOT_MAGIC:
            if (bootloader_magic == BOOTLOADER_BOOT_MAGIC) { // Magic received within window, go into bootloader mode
                timer_stop(bootloader_timer);
                bootloader_state = BOOTLOADER_IDLE;
            } else if (!timer_is_running(bootloader_timer)) { // No magic received within window, try to boot app
                bootloader_app_boot_crc = __app_crc16;
                bootloader_state = BOOTLOADER_BOOT;
            }
            break;
        case BOOTLOADER_IDLE:
            break;

        case BOOTLOADER_ERASE:
            bootloader_restore_app_mem_iterator();
            bootloader_state = BOOTLOADER_ERASE_PAGE;
            // no break
        case BOOTLOADER_ERASE_PAGE:
            if (bus_idle()) { // Since erasing a page is a blocking operation
                bool ok = nvm_erase_page_virt(bootloader_app_mem_iterator);
                bootloader_app_mem_iterator += NVM_PAGE_SIZE;

                if (!ok)
                    bootloader_state = BOOTLOADER_ERROR;
                else if (bootloader_app_mem_iterator == bootloader_app_mem_end)
                    bootloader_state = BOOTLOADER_IDLE;
            }
            break;
        case BOOTLOADER_BURN_ROW:
            if (bus_idle()) {
                bool ok = nvm_write_row_phys(bootloader_row_burn_address);
                bootloader_row_cursor = 0;
                crc16_reset(&bootloader_nvm_row_crc);
                // Note buffer is automatically cleared after a successful write
                bootloader_state = ok ? BOOTLOADER_IDLE : BOOTLOADER_ERROR;
            }
            break;
        case BOOTLOADER_BOOT:
            crc16_reset(&bootloader_app_mem_crc);
            bootloader_restore_app_mem_iterator();
            bootloader_state = BOOTLOADER_BOOT_CRC_VERIFY;
            // no break
        case BOOTLOADER_BOOT_CRC_VERIFY:
            crc16_update(&bootloader_app_mem_crc, bootloader_app_mem_iterator, BOOTLOADER_CRC_STEP_SIZE);
            bootloader_app_mem_iterator += BOOTLOADER_CRC_STEP_SIZE;
            if (bootloader_app_mem_iterator == bootloader_app_mem_end) {
                if (bootloader_app_mem_crc != bootloader_app_boot_crc)
                    bootloader_state = BOOTLOADER_IDLE; // If we can't boot because of CRC mismatch, just go back into idle mode
                else if(__app_crc16 == bootloader_app_mem_crc)
                    bootloader_state = BOOTLOADER_BOOT_RUN_APP;
                else
                    bootloader_state = BOOTLOADER_BOOT_CRC_BURN;
            }
            break;
        case BOOTLOADER_BOOT_CRC_BURN:
            bootloader_state = nvm_write_word_virt(&__app_crc16, bootloader_app_mem_crc) // FIXME: doesn't "match" after restart?
                ? BOOTLOADER_BOOT_RUN_APP
                : BOOTLOADER_ERROR;
            break;
        case BOOTLOADER_BOOT_RUN_APP:
            if (bus_idle()) // Handle all in/out going bus messages before running the main app
                bootloader_run_app();
            break;

        default:
        case BOOTLOADER_ERROR:
            // Do nothing until someone takes me out of error mode
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

bool bootloader_error(void)
{
    return bootloader_state == BOOTLOADER_ERROR;
}

void bootloader_set_magic(unsigned int magic)
{
    bootloader_magic = magic;
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

bool bootloader_row_reset()
{
     if (bootloader_busy())
        return false;

    bootloader_row_cursor = 0;
    nvm_buffer_reset();
    crc16_reset(&bootloader_nvm_row_crc);
    return true;
}

unsigned short bootloader_row_crc16()
{
    return bootloader_nvm_row_crc;
}

bool bootloader_row_push_word(unsigned int word)
{
    if (bootloader_busy())
        return false;
    if (bootloader_row_cursor >= NVM_ROW_BUFFER_SIZE)
        return false;

    nvm_row_buffer[bootloader_row_cursor++] = word;
    crc16_update(&bootloader_nvm_row_crc, &word, sizeof(word));
    return true;
}

bool bootloader_row_burn(unsigned int phy_address)
{
    if (bootloader_busy())
        return false;
    if (bootloader_row_cursor < NVM_ROW_BUFFER_SIZE)
        return false;
    if (phy_address < PHY_ADDR(__app_mem_start) ||
        phy_address > (PHY_ADDR(__app_mem_end) - NVM_ROW_SIZE))
        return false;

    bootloader_row_burn_address = (void const *)phy_address;
    bootloader_state = BOOTLOADER_BURN_ROW;
    return true;
}