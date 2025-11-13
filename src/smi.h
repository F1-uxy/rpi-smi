#ifndef SMI_H
#define SMI_H

#include "gpio.h"

#define SMI_BASE    (PHYS_REG_BASE + 0x6000000) /* Base address             */
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
#define SMIO_DCA     0x38                        /* Direct address           */
#define SMIO_DCD     0x3c                        /* Direct data              */
#define SMIO_FD      0x40                        /* FIFO debug               */
#define SMIO_REGLEN  (SMI_FD * 4)

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

/* SMI Direct Mode Data Register */
typedef struct {
    volatile uint32_t   data : 18,
                        _res : 14;
} SMI_DD_BITFIELD;

typedef union {
    SMI_DD_BITFIELD fields;
    volatile uint32_t value;
} SMI_DCD;

void init_smi(SMI_CS* cs, SMI_DSR* dsr, SMI_DSW* dsw, int width, int ns, int setup, int strobe, int hold)
{
    //int divi = ns/2;

    cs->value = 0;
    dsr->value = 0;

    dsr->fields.rsetup = dsw->fields.wsetup = setup;
    dsr->fields.rstrobe = dsw->fields.wstrobe =strobe;
    dsr->fields.rhold = dsw->fields.whold = hold;
    dsr->fields.rwidth = dsw->fields.wwidth = width;
}

void smi_cs_init(volatile SMI_CS* cs)
{
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;
}

#endif