#include <spi.h>
#include <assert_util.h>
#include <util.h>
#include <sys.h>
#include <dma.h>
#include <stddef.h>
#include <xc.h>

#define SPI_BRG(baudrate)           (SYS_PB_CLOCK / (baudrate << 1) - 1)
#define SPI_FIFO_DEPTH_MODE32       4
#define SPI_FIFO_DEPTH_MODE16       8
#define SPI_FIFO_DEPTH_MODE8        16
#define SPI_FIFO_SIZE_MODE32        4 // In bytes
#define SPI_FIFO_SIZE_MODE16        2 // In bytes
#define SPI_FIFO_SIZE_MODE8         1 // In bytes

#define SPI_SPISTAT_SPITBF_MASK     BIT(1)

struct spi_register_map
{
    ATOMIC_REG(spicon);
    ATOMIC_REG(spistat);
    ATOMIC_REG(spibuf);
    ATOMIC_REG(spibrg);
    ATOMIC_REG(spicon2);
};

struct spi_interrupt_map
{
    ATOMIC_REG_PTR(ifs);
    ATOMIC_REG_PTR(iec);

    unsigned int fault_mask;
    unsigned int receive_mask;
    unsigned int transfer_mask;
    unsigned char fault_irq;
    unsigned char receive_irq;
    unsigned char transfer_irq;
};

static const struct spi_interrupt_map spi_module_interrupts[] =
{
    [SPI_CHANNEL1] = {
        .ifs = ATOMIC_REG_PTR_CAST(&IFS1),
        .iec = ATOMIC_REG_PTR_CAST(&IEC1),
        .fault_mask = BIT(3),
        .receive_mask = BIT(4),
        .transfer_mask = BIT(5),
        .fault_irq = _SPI1_ERR_IRQ,
        .receive_irq = _SPI1_RX_IRQ,
        .transfer_irq = _SPI1_TX_IRQ,
    },
    [SPI_CHANNEL2] = {
        .ifs = ATOMIC_REG_PTR_CAST(&IFS2),
        .iec = ATOMIC_REG_PTR_CAST(&IEC2),
        .fault_mask = BIT(21),
        .receive_mask = BIT(22),
        .transfer_mask = BIT(23),
        .fault_irq = _SPI2_ERR_IRQ,
        .receive_irq = _SPI2_RX_IRQ,
        .transfer_irq = _SPI2_TX_IRQ,
    },
};

struct spi_module
{
    struct spi_register_map const * const spi_reg;
    struct spi_interrupt_map const * const spi_int;

    unsigned char fifo_size;
    bool assigned;
};

struct spi_module spi_modules[] =
{
    [SPI_CHANNEL1] = {
        .spi_reg = (struct spi_register_map const * const)(&SPI1CON),
        .spi_int = &spi_module_interrupts[SPI_CHANNEL1],
        .fifo_size = SPI_FIFO_SIZE_MODE8,
        .assigned = false,
    },
    [SPI_CHANNEL2] = {
        .spi_reg = (struct spi_register_map const * const)(&SPI2CON),
        .spi_int = &spi_module_interrupts[SPI_CHANNEL2],
        .fifo_size = SPI_FIFO_SIZE_MODE8,
        .assigned = false,
    }
};

struct spi_module * spi_construct(enum spi_channel channel, struct spi_config config)
{
    struct spi_module * module = &spi_modules[channel];

    // Already assigned
    if (module->assigned)
        return NULL;

    // Assign and configure module
    module->assigned = true;
    spi_configure(module, config);
    return module;
}

void spi_destruct(struct spi_module * module)
{
    ASSERT_NOT_NULL(module);

    ATOMIC_REG_CLR(module->spi_reg->spicon, SPI_ON);
    module->assigned = false;
}

void spi_configure(struct spi_module * module, struct spi_config config)
{
    ASSERT_NOT_NULL(module);
    struct spi_register_map const * const spi_reg = module->spi_reg;
    struct spi_interrupt_map const * const spi_int = module->spi_int;

    // Disable module first
    ATOMIC_REG_CLR(spi_reg->spicon, SPI_ON);

    // For now interrupts are not supported, always disable them
    ATOMIC_REG_PTR_CLR(spi_int->iec, spi_int->fault_mask);
    ATOMIC_REG_PTR_CLR(spi_int->iec, spi_int->receive_mask);
    ATOMIC_REG_PTR_CLR(spi_int->iec, spi_int->transfer_mask);
    ATOMIC_REG_PTR_CLR(spi_int->ifs, spi_int->fault_mask);
    ATOMIC_REG_PTR_CLR(spi_int->ifs, spi_int->receive_mask);
    ATOMIC_REG_PTR_CLR(spi_int->ifs, spi_int->transfer_mask);

    // Configure SPI
    ATOMIC_REG_VALUE(spi_reg->spibrg) = config.baudrate > 0 ? SPI_BRG(config.baudrate) : 0;
    ATOMIC_REG_VALUE(spi_reg->spicon) = config.spi_con_flags;

    if (config.spi_con_flags & SPI_MODE32)
        module->fifo_size = SPI_FIFO_SIZE_MODE32;
    else if (config.spi_con_flags & SPI_MODE16)
        module->fifo_size = SPI_FIFO_SIZE_MODE16;
    else
        module->fifo_size = SPI_FIFO_SIZE_MODE8;
}

void spi_configure_dma_src(struct spi_module * module, struct dma_channel * channel)
{
    ASSERT_NOT_NULL(module);
    ASSERT_NOT_NULL(channel);
    struct dma_event start_event =
    {
        .enable = true,
        .irq_vector = module->spi_int->receive_irq,
    };

    struct dma_event abort_event =
    {
        .enable = true,
        .irq_vector = module->spi_int->fault_irq,
    };

    dma_configure_src(channel, &module->spi_reg->spibuf, module->fifo_size);
    dma_configure_cell(channel, 1); // one fifo_size per transfer
    dma_configure_start_event(channel, start_event);
    dma_configure_abort_event(channel, abort_event);
}

void spi_configure_dma_dst(struct spi_module * module, struct dma_channel * channel)
{
    ASSERT_NOT_NULL(module);
    ASSERT_NOT_NULL(channel);
    struct dma_event start_event =
    {
        .enable = true,
        .irq_vector = module->spi_int->transfer_irq,
    };

    struct dma_event abort_event =
    {
        .enable = true,
        .irq_vector = module->spi_int->fault_irq,
    };

    dma_configure_dst(channel, &module->spi_reg->spibuf, module->fifo_size);
    dma_configure_cell(channel, 1); // one fifo_size per transfer
    dma_configure_start_event(channel, start_event);
    dma_configure_abort_event(channel, abort_event);
}

void spi_enable(struct spi_module * module)
{
    ASSERT_NOT_NULL(module);

    ATOMIC_REG_SET(module->spi_reg->spicon, SPI_ON);
}

void spi_disable(struct spi_module * module)
{
    ASSERT_NOT_NULL(module);

    ATOMIC_REG_CLR(module->spi_reg->spicon, SPI_ON);
}

void spi_transmit_mode32(struct spi_module * module, unsigned int * buffer, unsigned int size)
{
    ASSERT_NOT_NULL(module);
    ASSERT(ATOMIC_REG_VALUE(module->spi_reg->spicon) & SPI_ON);
    struct spi_register_map const * const spi_reg = module->spi_reg;

    if (size && ATOMIC_REG_VALUE(spi_reg->spicon) & SPI_MSTEN) {
        while (size--) {
            while(ATOMIC_REG_VALUE(spi_reg->spistat) & SPI_SPISTAT_SPITBF_MASK);
            ATOMIC_REG_VALUE(spi_reg->spibuf) = *buffer++;
        }
    }
}

void spi_transmit_mode8(struct spi_module * module, unsigned char * buffer, unsigned int size)
{
    ASSERT_NOT_NULL(module);
    ASSERT(ATOMIC_REG_VALUE(module->spi_reg->spicon) & SPI_ON);
    struct spi_register_map const * const spi_reg = module->spi_reg;

    if (size && ATOMIC_REG_VALUE(spi_reg->spicon) & SPI_MSTEN) {
        while (size--) {
            while(ATOMIC_REG_VALUE(spi_reg->spistat) & SPI_SPISTAT_SPITBF_MASK);
            ATOMIC_REG_VALUE(spi_reg->spibuf) = *buffer++;
        }
    }
}


