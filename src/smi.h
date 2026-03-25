#ifndef SMI_H
#define SMI_H

#include <stdbool.h>

#include "gpio.h"
#include "dma.h"
#include "timeout.h"

/* Timeout */
#define DIRECT_READ_TIMEOUT_S 2
#define PROG_READ_TIMEOUT_S 50

#define DIRECT_WRITE_TIMEOUT_S 15
#define PROG_WRITE_TIMEOUT_S 50
#define DMA_WRITE_TIMEOUT_S 2

/* SMI Register Offsets */
#define SMI_BASE    (PHYS_REG_BASE + 0x600000)   /* Base address             */
#define SMIO_CS      0x00                        /* Control & status         */
#define SMIO_L       0x04                        /* Transfer length          */
#define SMIO_A       0x08                        /* Address                  */
#define SMIO_D       0x0c                        /* Data                     */
#define SMIO_DSR(dev) (0x10 + ((dev) * 0x08))    /* Device offset selector   */
#define SMIO_DSW(dev) (SMIO_DSR(dev) + 4)        /*                          */
#define SMIO_DSR0    0x10                        /* Read settings device 0   */
#define SMIO_DSW0    0x14                        /* Write settings device 0  */
#define SMIO_DSR1    0x18                        /* Read settings device 1   */
#define SMIO_DSW1    0x1c                        /* Write settings device 1  */
#define SMIO_DSR2    0x20                        /* Read settings device 2   */
#define SMIO_DSW2    0x24                        /* Write settings device 2  */
#define SMIO_DSR3    0x28                        /* Read settings device 3   */
#define SMIO_DSW3    0x2c                        /* Write settings device 3  */
#define SMIO_DC      0x30                        /* DMA control              */
#define SMIO_DCS     0x34                        /* Direct control/status    */
#define SMIO_DA      0x38                        /* Direct address           */
#define SMIO_DD      0x3c                        /* Direct data              */
#define SMIO_FD      0x40                        /* FIFO debug               */

/* SMI Interface Config */
#define SMI_8_BITS  0
#define SMI_16_BITS 1
#define SMI_18_BITS 2
#define SMI_9_BITS  3

#define SMI_RGB565 0
#define SMI_XRGB 1

#define SMI_DEVICE1 0
#define SMI_DEVICE2 1
#define SMI_DEVICE3 2
#define SMI_DEVICE4 3

#define SMI_DMA_L(l) (l*4)
#define SMI_DIV(count, ratio) ((float)count/(float)ratio + ((int)count % (int)ratio != 0));

typedef struct {
    uint8_t read;
    uint8_t out_pixels;
} smi_pack_ratio_t;

/* SMI Control and Status Register */
typedef struct {
    volatile uint32_t   enable : 1, /* LSB to MSB                                                   */
                        done   : 1, /* Bitfield order is compiler dependant                         */
                        active : 1, /* Not so portable but this code should only be run on this CPU */
                        start  : 1,
                        clear  : 1,
                        write  : 1,
                        pad    : 2,
                        teen   : 1,
                        intd   : 1,
                        intt   : 1,
                        intr   : 1,
                        pvmode : 1,
                        seterr : 1,
                        pxldat : 1,
                        edreq  : 1,
                        _res   : 7,
                        prdy   : 1,
                        aferr  : 1,
                        rxr    : 1,
                        txw    : 1,
                        rxd    : 1,
                        txd    : 1,
                        rxf    : 1,
                        txe    : 1;
} SMI_CS_BITFIELD;

typedef union {
    SMI_CS_BITFIELD fields;
    volatile uint32_t value; 
} SMI_CS __attribute__ ((aligned(32)));

/* SMI Length Register */
typedef struct {
    volatile uint32_t   length : 32;
} SMI_L_BITFIELD;

typedef union {
    SMI_L_BITFIELD fields;
    volatile uint32_t value;
} SMI_L __attribute__((aligned(32)));


/* SMI Address Register */
typedef struct {
    volatile uint32_t   
                        addr    : 6,
                        _x2     : 2,
                        device  : 2,
                        _x1     : 21;
} SMI_A_BITFIELD;

typedef union {
    SMI_A_BITFIELD fields;
    volatile uint32_t value;
} SMI_A __attribute__((aligned(32)));


/* SMI Data Register */
typedef struct {
    volatile uint32_t   data : 32;
} SMI_D_BITFIELD;

typedef union {
    SMI_D_BITFIELD fields;
    volatile uint32_t value;
} SMI_D __attribute__((aligned(32)));


/* SMI DMA Control Register */
typedef struct {
    volatile uint32_t   reqw    : 6,
                        reqr    : 6,
                        panicw  : 6,
                        panicr  : 6,
                        dmap    : 1,
                        _x2     : 3,
                        dmaen   : 1,
                        _x1     : 3;
} SMI_DC_BITFIELD;

typedef union {
    SMI_DC_BITFIELD fields;
    volatile uint32_t value;
} SMI_DC __attribute__((aligned(32)));


/* SMI Device Read Setting Register */
typedef struct {
    volatile uint32_t   rstrobe  : 7,
                        rdreq    : 1,
                        rpace    : 7,
                        rpaceall : 1,
                        rhold    : 6,
                        fsetup   : 1,
                        mode68   : 1,
                        rsetup   : 6,
                        rwidth   : 2;
} SMI_DSR_BITFIELD;

typedef union {
    SMI_DSR_BITFIELD fields;
    volatile uint32_t value;
} SMI_DSR __attribute__ ((aligned(32)));


/* SMI Device Write Setting Register */
typedef struct {
    volatile uint32_t   wstrobe  : 7,
                        wdreq    : 1,
                        wpace    : 7,
                        wpaceall : 1,
                        whold    : 6,
                        wswap    : 1,
                        wformat  : 1,
                        wsetup   : 6,
                        wwidth   : 2;
} SMI_DSW_BITFIELD;

typedef union {
    SMI_DSW_BITFIELD fields;
    volatile uint32_t value;
} SMI_DSW __attribute__ ((aligned(32)));


/* SMI Direct Control and Status Register */
typedef struct {
    volatile uint32_t   enable : 1,
                        start  : 1,
                        done   : 1,
                        write  : 1,
                        _res   : 28;
} SMI_DCS_BITFIELD;

typedef union {
    SMI_DCS_BITFIELD fields;
    volatile uint32_t value;
} SMI_DCS __attribute__((aligned(32)));


/* SMI Direct Mode Address Register */
typedef struct {
    volatile uint32_t   addr    : 6,
                        _x2     : 2,
                        device  : 2,
                        _x1     : 22;
} SMI_DA_BITFIELD;

typedef union {
    SMI_DA_BITFIELD fields;
    volatile uint32_t value;
} SMI_DA __attribute__((aligned(32)));


/* SMI Direct Mode Data Register */
typedef struct {
    volatile uint32_t   data : 18,
                        _res : 14;
} SMI_DD_BITFIELD;

typedef union {
    SMI_DD_BITFIELD fields;
    volatile uint32_t value;
} SMI_DD __attribute__((aligned(32)));


/* SMI FIFO Debug Register */
typedef struct {
    volatile uint32_t   fcnt    : 6,
                        _x2     : 2,
                        flvl    : 6,
                        _x1     : 18;
                        
}SMI_FD_BITFIELD;

typedef union {
    SMI_FD_BITFIELD fields;
    volatile uint32_t value;
}SMI_FD __attribute__((aligned(32)));


/* SMI Clock Config */
typedef struct
{
    uint8_t device_num;

    uint8_t rsetup;
    uint8_t rhold;
    uint8_t rstrobe;
    uint8_t rpace;

    uint8_t wsetup;
    uint8_t whold;
    uint8_t wstrobe;
    uint8_t wpace;
} SMI_CLK;


/* SMI Read Config */
typedef struct
{
    uint8_t rwidth;
    uint8_t rhold;
    uint8_t rpace;
    uint8_t rstrobe;
    uint8_t rsetup;

    bool mode68; /* 0 = System-80 ; 1 = System-68*/
    bool fsetup;
    bool rpaceall;
    bool rdreq;
} SMI_READ;


/* SMI Write Config */
typedef struct
{
    uint8_t wwidth;
    uint8_t wsetup;
    uint8_t whold;
    uint8_t wpace;
    uint8_t wstrobe;

    bool wformat;
    bool wswap;
    bool wpaceall;
    bool wdreq;
} SMI_WRITE;

/* SMI Read Write & Clock Context */
typedef struct
{
    uint8_t write_device_num;
    uint8_t read_device_num;
    
    SMI_CLK* clk;
    SMI_READ rconfig[4];
    SMI_WRITE wconfig[4];
} SMI_RW;

/* SMI DMA Config */
typedef struct
{
    bool dmap;
    
    bool panicr; /* RX Panic when RX FIFO exceeds threshold. Increase priority on bus*/
    bool panicw; /* TX Panic when TX FIFO drops below threshold. Increase priority on bus */

    bool reqr; /* RX DREQ when RX FIFO exceeds threshold. AXI RX DMA to read RX FIFO */
    bool reqw; /* TX DREQ when TX FIFO drops below threshold. AXI TX DMA to write TX FIFO */
} SMI_DMA;


/* SMI Context */

typedef struct {
    uint32_t* buf;
    size_t size;
} SMI_RAW_BUF;

typedef struct {
    uint8_t interface_width;
    uint8_t pixel_format;
    uint8_t pixel_value_mode;
    uint8_t device_settings_select; 
    
    bool pad;
    bool tear;
    bool prdy;
    bool pxldata;
    bool intr;
    bool intt;
    bool intd;

    bool dma;

    int fd_sync_dev;
    int fd_sync_cpu;

    SMI_RAW_BUF raw_buffer;

    SMI_DMA dma_config;

    MEM_MAP* smi_regs;
    MEM_MAP* gpio_regs;
    MEM_MAP* dma_regs;
    MEM_MAP* clk_regs;
    MEM_MAP* dma_buffer;

    SMI_RW* rw_config;

} SMI_CXT;

/* --- SMI Setup Helpers --- */
void smi_init_cxt_map(SMI_CXT* cxt, MEM_MAP* smi_regs, MEM_MAP* clk_regs, MEM_MAP* gpio_regs, MEM_MAP* dma_regs);
void smi_init_rw_config(SMI_CXT* cxt, SMI_RW* rw, SMI_CLK* clk, SMI_READ* rconfig, SMI_WRITE* wconfig, int read_device, int write_device);
void smi_sync_context_device(SMI_CXT* cxt);
void smi_configure_read_device(SMI_CXT* cxt, uint8_t n);
void smi_configure_write_device(SMI_CXT* cxt, uint8_t n);
int smi_init_udmabuf(SMI_CXT* cxt, MEM_MAP* dma_buffer);

void init_smi_clk(MEM_MAP clk_regs, MEM_MAP smi_regs, int ns);
void smi_8b_init(MEM_MAP gpio_map);
void smi_gpio_init(MEM_MAP gpio_map);

void smi_dma_setup(MEM_MAP smi_regs);
void smi_dma_write(MEM_MAP smi_regs, MEM_MAP dma_regs, MEM_MAP* dma_buffer, int fd_sync_dev, DMA_CB* cb, uint8_t channel);

/* --- SMI Destructors --- */
void smi_unmap_cxt(SMI_CXT* cxt);
void smi_unmap_udmabuf(SMI_CXT* cxt);

/* --- Interfaces --- */

#define SMI_ADDR_INC     1 /* Increment address */
#define SMI_ADDR_NO_INC  0
#define SMI_ADDR_DEC    -1 /* Decrement address */

/* Direct Write*/
int smi_direct_write(SMI_CXT* cxt, uint32_t data, uint8_t addr);
int smi_direct_write_arr(SMI_CXT* cxt, uint32_t* data, uint8_t addr, uint8_t len, int increment);

/* Programmed Write */
int smi_programmed_write(SMI_CXT* cxt, uint32_t data, uint8_t addr);
int smi_programmed_write_arr(SMI_CXT* cxt, uint32_t* data, uint8_t addr, int len);
int smi_programmed_write_dma(SMI_CXT* cxt, DMA_CB* cb, uint8_t addr, int len, int channel);


/* Direct Read */
int smi_direct_read(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr);
int smi_direct_read_arr(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len, int increment);

/* Programmed Read */
int smi_programmed_read(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr);
int smi_programmed_read_arr(SMI_CXT* cxt, void* ret_data, uint8_t addr, int len);
int smi_programmed_read_dma(SMI_CXT* cxt, DMA_CB* cb, uint8_t addr, int len, int channel);


/* Setup Interfaces */
int smi_clk_config(SMI_RW* rwconfig);
int smi_gpio_config(SMI_CXT* cxt);


/* --- Workers --- */
void smi_start(SMI_CXT* cxt);
int smi_write_await_direct(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len, int increment);
int smi_read_await_direct(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len, int increment);
int smi_write_await(SMI_CXT* cxt, uint32_t* data, uint8_t addr, int len);
int smi_read_await(SMI_CXT* cxt, uint32_t* ret_data, int len);
int smi_dma_await(SMI_CXT* cxt);

/* --- Bit Unpackers --- */
void smi_unpack_rgb565_8(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);
void smi_unpack_xrgb_8(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);

void smi_unpack_xrgb_9(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);
void smi_unpack_xrgb_9_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);
void smi_unpack_rgb565_9(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);
void smi_unpack_rgb565_9_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);

void smi_unpack_xrgb_16(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);
void smi_unpack_rgb565_16(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);

void smi_unpack_xrgb_18(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);
void smi_unpack_rgb565_18(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio);

void smi_unpack(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count, smi_pack_ratio_t ratio);
smi_pack_ratio_t smi_packed_ratio(SMI_CXT* cxt);

/* --- Word Truncators --- */
void smi_truncate(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count);
void smi_truncate_8(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count);
void smi_truncate_9(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count);
void smi_truncate_16(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count);
void smi_truncate_18(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count);

#endif