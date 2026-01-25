#ifndef SMI_H
#define SMI_H

#include <stdbool.h>

#include "gpio.h"
#include "dma.h"

#define WRITE_TIMEOUT 1000000

/* SMI Register Offsets */
#define SMI_BASE    (PHYS_REG_BASE + 0x600000)   /* Base address             */
#define SMIO_CS      0x00                        /* Control & status         */
#define SMIO_L       0x04                        /* Transfer length          */
#define SMIO_A       0x08                        /* Address                  */
#define SMIO_D       0x0c                        /* Data                     */
#define SMIO_DSR0    0x10                        /* Read settings device 0   */
#define SMIO_DSW0    0x14                        /* Write settings device 0  */
#define SMIO_DSR1    0x18                        /* Read settings device 1   */
#define SMIO_DSW1    0x1c                        /* Write settings device 1  */
#define SMIO_DSR2    0x20                        /* Read settings device 2   */
#define SMIO_DSW2    0x24                        /* Write settings device 2  */
#define SMIO_DSR3    0x28                        /* Read settings device 3   */
#define SMIO_DSW3    0x2c                        /* Write settings device 3  */
#define SMIO_DMC     0x30                        /* DMA control              */
#define SMIO_DCS     0x34                        /* Direct control/status    */
#define SMIO_DA      0x38                        /* Direct address           */
#define SMIO_DD      0x3c                        /* Direct data              */
#define SMIO_FD      0x40                        /* FIFO debug               */
#define SMIO_REGLEN  (SMI_FD * 4)

/* SMI Width Values */
#define SMI_8_BITS  0
#define SMI_16_BITS 1
#define SMI_18_BITS 2
#define SMI_9_BITS  3


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
                        txw    : 1,
                        rxr    : 1,
                        txd    : 1,
                        rxd    : 1,
                        txe    : 1,
                        rxf    : 1;
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
    volatile uint32_t   _x1     : 22,                        
                        addr    : 6,
                        _x2     : 2,
                        device  : 2;
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

void init_smi_clk(volatile SMI_CS* cs, MEM_MAP clk_regs, MEM_MAP smi_regs, volatile SMI_DSR* dsr, volatile SMI_DSW* dsw, int ns, int setup, int strobe, int hold);
void smi_gpio_init(MEM_MAP gpio_map);

void smi_cs_init(volatile SMI_CS* cs);


void smi_8b_init(MEM_MAP gpio_map);
void smi_8b_write(MEM_MAP smi_regs, uint8_t data, uint8_t addr);
void smi_8b_direct_write(MEM_MAP smi_regs, uint8_t data, uint8_t addr);
void smi_8byte_write(MEM_MAP smi_regs, uint8_t addr, uint8_t* data, int len);

void smi_dma_setup(MEM_MAP smi_regs);
void smi_dma_write(MEM_MAP smi_regs, MEM_MAP dma_regs, MEM_MAP* dma_buffer, DMA_CB* cb, uint8_t channel);

int smi_8b_read(MEM_MAP smi_regs, uint8_t addr);
int smi_programmed_read_old(MEM_MAP smi_regs, uint8_t addr, uint8_t* ret_data, uint8_t len);


/* SMI Context */

typedef struct {
    uint8_t interface_width;
    uint8_t pixel_format;
    uint8_t pixel_value_mode;
    uint8_t device_settings_select; 
    uint8_t interrupts;
    
    bool pad;
    bool tear;
    bool prdy;
    bool pxldata;

    MEM_MAP* smi_regs;
    MEM_MAP* gpio_regs;
    MEM_MAP* dma_regs;
    MEM_MAP* dma_buffer;
} SMI_CXT;

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
    uint8_t fsetup;
    uint8_t rpaceall;

    bool rexreq;
    bool mode_80;
} SMI_READ;


/* SMI Write Config */
typedef struct
{
    uint8_t wwidth;
    uint8_t wformat;
    uint8_t wswap;
    uint8_t wpaceall;
    bool wexreg;
} SMI_WRITE;

/* SMI Read Write & Clock Context */
typedef struct
{
    uint8_t device_num;
    
    SMI_CLK* clk;
    SMI_READ* rconfig;
    SMI_WRITE* wconfig;
} SMI_RW;


/* --- Interfaces --- */

/* Direct Write*/
int smi_direct_write(SMI_CXT* cxt);
int smi_direct_write_arr(SMI_CXT* cxt);
int smi_direct_write_arr_addr(SMI_CXT* cxt);

/* Programmed Write */
int smi_programmed_write(SMI_CXT* cxt);
int smi_programmed_write_arr(SMI_CXT* cxt);
int smi_programmed_write_dma(SMI_CXT* cxt);


/* Direct Read */
int smi_direct_read(SMI_CXT* cxt);
int smi_direct_read_arr(SMI_CXT* cxt);
int smi_direct_read_arr_addr(SMI_CXT* cxt);

/* Programmed Read */
int smi_programmed_read(SMI_CXT* cxt);
int smi_programmed_read_arr(SMI_CXT* cxt);
int smi_programmed_read_dma(SMI_CXT* cxt);


/* Setup Interfaces */
int smi_clk_config(SMI_RW* rwconfig);
int smi_gpio_config(SMI_CXT* cxt);



/* --- Workers --- */
inline int smi_start(SMI_CXT* cxt);
int smi_timeout_init();
int smi_await();

#endif