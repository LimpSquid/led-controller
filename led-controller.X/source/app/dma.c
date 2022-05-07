#include <app/dma.h>
#include <assert_util.h>
#include <util.h>
#include <xc.h>
#include <sys/attribs.h>

#define DMA_NUMBER_OF_CHANNELS          (sizeof(dma_channels) / sizeof(dma_channels[0]))
#define DMA_INTERRUPT_PRIORITY          0x3 // Interrupt handlers must use IPL3SOFT

#define DMA_DMACON_REG                  DMACON

#define DMA_DMACON_WORD                 BIT(15)

#define DMA_DCHCON_CHEN_MASK            BIT(7)
#define DMA_DCHCON_CHAEN_MASK           BIT(4)
#define DMA_DCHCON_CHBUSY_MASK          BIT(15)
#define DMA_DCHECON_CHAIRQ_MASK         MASK(0xff, 16)
#define DMA_DCHECON_CHSIRQ_MASK         MASK(0xff, 8)
#define DMA_DCHECON_AIRQEN_MASK         BIT(3)
#define DMA_DCHECON_SIRQEN_MASK         BIT(4)
#define DMA_DCHECON_CFORCE_MASK         BIT(7)
#define DMA_DCHINT_CHBCIE_MASK          BIT(19)
#define DMA_DCHINT_CHTAIE_MASK          BIT(17)
#define DMA_DCHINT_CHBCIF_MASK          BIT(3)
#define DMA_DCHINT_CHTAIF_MASK          BIT(1)
#define DMA_DCHINT_IFS_MASK             MASK(0xff, 0)
#define DMA_DCHINT_ENABLE_BITS_MASK     MASK(0xffff, 8)

#define DMA_DCHECON_CHAIRQ_SHIFT        16
#define DMA_DCHECON_CHSIRQ_SHIFT        8

struct dma_register_map
{
    ATOMIC_REG(dchcon);
    ATOMIC_REG(dchecon);
    ATOMIC_REG(dchint);
    ATOMIC_REG(dchssa);
    ATOMIC_REG(dchdsa);
    ATOMIC_REG(dchssiz);
    ATOMIC_REG(dchdsiz);
    ATOMIC_REG(dchsptr);
    ATOMIC_REG(dchdptr);
    ATOMIC_REG(dchcsiz);
    ATOMIC_REG(dchcptr);
    ATOMIC_REG(dchdat);
};

struct dma_interrupt_map
{
    ATOMIC_REG_PTR(ifs);
    ATOMIC_REG_PTR(iec);
    ATOMIC_REG_PTR(ipc);

    unsigned int mask;
    unsigned char priority_shift;
};

struct dma_channel
{
    struct dma_register_map const * const dma_reg;
    struct dma_interrupt_map const * const dma_int;

    void (*block_transfer_complete)(struct dma_channel *);
    void (*transfer_abort)(struct dma_channel *);
    bool assigned;
};

static const struct dma_interrupt_map dma_channel_interrupts[] =
{
    {
        .ifs = ATOMIC_REG_PTR_CAST(&IFS2),
        .iec = ATOMIC_REG_PTR_CAST(&IEC2),
        .ipc = ATOMIC_REG_PTR_CAST(&IPC10),
        .mask = BIT(8),
        .priority_shift = 18,
    },
    {
        .ifs = ATOMIC_REG_PTR_CAST(&IFS2),
        .iec = ATOMIC_REG_PTR_CAST(&IEC2),
        .ipc = ATOMIC_REG_PTR_CAST(&IPC10),
        .mask = BIT(9),
        .priority_shift = 26,
    },
    {
        .ifs = ATOMIC_REG_PTR_CAST(&IFS2),
        .iec = ATOMIC_REG_PTR_CAST(&IEC2),
        .ipc = ATOMIC_REG_PTR_CAST(&IPC11),
        .mask = BIT(10),
        .priority_shift = 2,
    },
    {
        .ifs = ATOMIC_REG_PTR_CAST(&IFS2),
        .iec = ATOMIC_REG_PTR_CAST(&IEC2),
        .ipc = ATOMIC_REG_PTR_CAST(&IPC11),
        .mask = BIT(11),
        .priority_shift = 10,
    },
};

static struct dma_channel dma_channels[] =
{
    {
        .dma_reg = (const struct dma_register_map* const)(&DCH0CON),
        .dma_int = &dma_channel_interrupts[0],
        .assigned = false,
    },
    {
        .dma_reg = (const struct dma_register_map* const)(&DCH1CON),
        .dma_int = &dma_channel_interrupts[1],
        .assigned = false,
    },
    {
        .dma_reg = (const struct dma_register_map* const)(&DCH2CON),
        .dma_int = &dma_channel_interrupts[2],
        .assigned = false,
    },
    {
        .dma_reg = (const struct dma_register_map* const)(&DCH3CON),
        .dma_int = &dma_channel_interrupts[3],
        .assigned = false,
    }
};

void dma_init(void)
{
    // Disable interrupt on each channel
    for (unsigned int i = 0; i < DMA_NUMBER_OF_CHANNELS; ++i)
        ATOMIC_REG_PTR_CLR(dma_channels[i].dma_int->iec, dma_channels[i].dma_int->mask);

    // Configure DMA
    DMA_DMACON_REG = DMA_DMACON_WORD;
}

struct dma_channel * dma_construct(struct dma_config config)
{
    struct dma_channel * channel = NULL;

    // Search for an unused channel
    for (unsigned int i = 0; i < DMA_NUMBER_OF_CHANNELS; ++i) {
        if (!dma_channels[i].assigned) {
            channel = &dma_channels[i];
            break;
        }
    }

    // Assign and configure channel, if found
    if (channel != NULL) {
        channel->assigned = true;
        dma_configure(channel, config);
    }
    return channel;
}

void dma_destruct(struct dma_channel * channel)
{
    ASSERT_NOT_NULL(channel);

    ATOMIC_REG_CLR(channel->dma_reg->dchcon, DMA_DCHCON_CHEN_MASK);
    channel->assigned = false;
}

void dma_configure(struct dma_channel * channel, struct dma_config config)
{
    ASSERT_NOT_NULL(channel);
    struct dma_register_map const * const dma_reg = channel->dma_reg;
    struct dma_interrupt_map const * const dma_int = channel->dma_int;

    // Disable channel first
    ATOMIC_REG_CLR(dma_reg->dchcon, DMA_DCHCON_CHEN_MASK);

    // Configure DMA
    if (config.auto_enable)
        ATOMIC_REG_SET(channel->dma_reg->dchcon, DMA_DCHCON_CHAEN_MASK);
    else
        ATOMIC_REG_CLR(channel->dma_reg->dchcon, DMA_DCHCON_CHAEN_MASK);
    ATOMIC_REG_VALUE(dma_reg->dchssa) = PHY_ADDR(config.src_mem);
    ATOMIC_REG_VALUE(dma_reg->dchdsa) = PHY_ADDR(config.dst_mem);
    ATOMIC_REG_VALUE(dma_reg->dchssiz) = config.src_size;
    ATOMIC_REG_VALUE(dma_reg->dchdsiz) = config.dst_size;
    ATOMIC_REG_VALUE(dma_reg->dchcsiz) = config.cell_size;

    // Configure events
    dma_configure_start_event(channel, config.start_event);
    dma_configure_abort_event(channel, config.abort_event);

    // Configure interrupts
    ATOMIC_REG_PTR_CLR(dma_int->iec, dma_int->mask);
    ATOMIC_REG_PTR_CLR(dma_int->ifs, dma_int->mask);
    ATOMIC_REG_PTR_CLR(dma_int->ipc, MASK(0x7, dma_int->priority_shift));
    ATOMIC_REG_CLR(dma_reg->dchint, DMA_DCHINT_CHBCIE_MASK);

    if (config.block_transfer_complete != NULL) {
        channel->block_transfer_complete = config.block_transfer_complete;
        ATOMIC_REG_SET(dma_reg->dchint, DMA_DCHINT_CHBCIE_MASK);
    }

    if (config.transfer_abort != NULL) {
        channel->transfer_abort = config.transfer_abort;
        ATOMIC_REG_SET(dma_reg->dchint, DMA_DCHINT_CHTAIE_MASK);
    }

    // Has interrupts enabled?
    if (ATOMIC_REG_VALUE(dma_reg->dchint) & DMA_DCHINT_ENABLE_BITS_MASK) {
        STATIC_ASSERT(DMA_INTERRUPT_PRIORITY >= 0 && DMA_INTERRUPT_PRIORITY <= 0x7);
        ATOMIC_REG_PTR_SET(dma_int->ipc, MASK(DMA_INTERRUPT_PRIORITY, dma_int->priority_shift));
        ATOMIC_REG_PTR_SET(dma_int->iec, dma_int->mask);
    }
}
void dma_configure_src(struct dma_channel * channel, void const * mem, unsigned short size)
{
    ASSERT_NOT_NULL(channel);
    struct dma_register_map const * const dma_reg = channel->dma_reg;

    ATOMIC_REG_VALUE(dma_reg->dchssa) = PHY_ADDR(mem);
    ATOMIC_REG_VALUE(dma_reg->dchssiz) = size;
}

void dma_configure_dst(struct dma_channel * channel, void const * mem, unsigned short size)
{
    ASSERT_NOT_NULL(channel);
    struct dma_register_map const * const dma_reg = channel->dma_reg;

    ATOMIC_REG_VALUE(dma_reg->dchdsa) = PHY_ADDR(mem);
    ATOMIC_REG_VALUE(dma_reg->dchdsiz) = size;
}

void dma_configure_cell(struct dma_channel * channel, unsigned short size)
{
    ASSERT_NOT_NULL(channel);

    ATOMIC_REG_VALUE(channel->dma_reg->dchcsiz) = size;
}

void dma_configure_start_event(struct dma_channel * channel, struct dma_event event)
{
    ASSERT_NOT_NULL(channel);
    struct dma_register_map const * const dma_reg = channel->dma_reg;

    ATOMIC_REG_CLR(dma_reg->dchecon, DMA_DCHECON_CHSIRQ_MASK);
    ATOMIC_REG_CLR(dma_reg->dchecon, DMA_DCHECON_SIRQEN_MASK);
    if (event.enable) {
        ATOMIC_REG_SET(dma_reg->dchecon, MASK_SHIFT(event.irq_vector, DMA_DCHECON_CHSIRQ_SHIFT) & DMA_DCHECON_CHSIRQ_MASK);
        ATOMIC_REG_SET(dma_reg->dchecon, DMA_DCHECON_SIRQEN_MASK);
    }
}

void dma_configure_abort_event(struct dma_channel * channel, struct dma_event event)
{
    ASSERT_NOT_NULL(channel);
    struct dma_register_map const * const dma_reg = channel->dma_reg;

    ATOMIC_REG_CLR(dma_reg->dchecon, DMA_DCHECON_CHAIRQ_MASK);
    ATOMIC_REG_CLR(dma_reg->dchecon, DMA_DCHECON_AIRQEN_MASK);
    if (event.enable) {
        ATOMIC_REG_SET(dma_reg->dchecon, MASK_SHIFT(event.irq_vector, DMA_DCHECON_CHAIRQ_SHIFT) & DMA_DCHECON_CHAIRQ_MASK);
        ATOMIC_REG_SET(dma_reg->dchecon, DMA_DCHECON_AIRQEN_MASK);
    }
}

void dma_enable_transfer(struct dma_channel * channel)
{
    ASSERT_NOT_NULL(channel);

    ATOMIC_REG_CLR(channel->dma_reg->dchecon, DMA_DCHECON_CFORCE_MASK);
    ATOMIC_REG_SET(channel->dma_reg->dchcon, DMA_DCHCON_CHEN_MASK);
}

void dma_enable_force_transfer(struct dma_channel * channel)
{
    ASSERT_NOT_NULL(channel);

    ATOMIC_REG_SET(channel->dma_reg->dchcon, DMA_DCHCON_CHEN_MASK);
    ATOMIC_REG_SET(channel->dma_reg->dchecon, DMA_DCHECON_CFORCE_MASK);
}

void dma_disable_transfer(struct dma_channel * channel)
{
    ASSERT_NOT_NULL(channel);

    ATOMIC_REG_CLR(channel->dma_reg->dchcon, DMA_DCHCON_CHEN_MASK);
}

bool dma_busy(struct dma_channel * channel)
{
    ASSERT_NOT_NULL(channel);

    return ATOMIC_REG_VALUE(channel->dma_reg->dchcon) & DMA_DCHCON_CHBUSY_MASK;
}

bool dma_ready(struct dma_channel * channel)
{
    return !dma_busy(channel);
}

static void dma_handle_interrupt(struct dma_channel * channel)
{
    // Read the DCHINT register for every if statement, in case an interrupt flag is set
    // inside one of the interrupt handlers.

    if (ATOMIC_REG_VALUE(channel->dma_reg->dchint) & DMA_DCHINT_CHBCIF_MASK)
        channel->block_transfer_complete(channel);
    if (ATOMIC_REG_VALUE(channel->dma_reg->dchint) & DMA_DCHINT_CHTAIF_MASK)
        channel->transfer_abort(channel);

    ATOMIC_REG_CLR(channel->dma_reg->dchint, DMA_DCHINT_IFS_MASK);
    ATOMIC_REG_PTR_CLR(channel->dma_int->ifs, channel->dma_int->mask);
}

void __ISR(_DMA_0_VECTOR, IPL3SOFT) dma_interrupt0(void)
{
    static struct dma_channel * channel = &dma_channels[0];
    dma_handle_interrupt(channel);
}

void __ISR(_DMA_1_VECTOR, IPL3SOFT) dma_interrupt1(void)
{
    static struct dma_channel * channel = &dma_channels[1];
    dma_handle_interrupt(channel);
}

void __ISR(_DMA_2_VECTOR, IPL3SOFT) dma_interrupt2(void)
{
    static struct dma_channel * channel = &dma_channels[2];
    dma_handle_interrupt(channel);
}

void __ISR(_DMA_3_VECTOR, IPL3SOFT) dma_interrupt3(void)
{
    static struct dma_channel * channel = &dma_channels[3];
    dma_handle_interrupt(channel);
}
