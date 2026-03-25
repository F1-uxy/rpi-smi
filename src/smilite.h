/*
**  SMI Lite is a single header implementation of the SMI API
**  This header contains core functionality such as read awaits and control structs
**  Quality of life functionality is not included and is expected to be implemented by the user
*/

#ifndef SMI_LITE
#define SMI_LITE

/*
    - Macros:
        - Memory translation and accessors X
        - Base addresses X
        - Offsets X
        - Errors X
    - Structs:
        - All smi structs X
    - Functions:
        - SMI inits (cxt, rw, clk, gpio?) No
        - SMI await (read, write, dma, direct) X
        - SMI unpackers & truncaters X
        - Map & unmappers X
        - Timeouts X
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

/* Physical Register Base */
/* This must be changed to match you system. This is for RPI3 */
#define PHYS_REG_BASE 0x3F000000u

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

/* DMA Register Offsets */
#define DMAO_CS         0x0
#define DMAO_CONBLK_AD  0x4
#define DMAO_TI         0x8
#define DMAO_SOURCE_AD  0xC
#define DMA0_DEST_AD    0x10
#define DMAO_TXFR_LEN   0x14
#define DMAO_STRIDE     0x18
#define DMAO_NEXTCONBK  0x1C
#define DMAO_DEBUG      0x20

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

/* Errors */
#define ERROR(e) (fprintf(stderr, "ERROR: %s\n", e))

/* Accessors & Translators */
#define REG32(m, x) ((volatile uint32_t*) ((uintptr_t)(m.virt)+(uintptr_t)(x)))
#define REG32_BUS(m, x) ((volatile uint32_t*) ((uintptr_t)(m.bus))  + (uint32_t)(x))
#define REG_BUS_ADDR(m, x)  ((uint32_t)(m.bus) + (uint32_t)(x))
#define MEM_BUS_ADDR(mp, a) ((uint32_t)a-(uint32_t)mp->virt+(uint32_t)mp->bus)
#define PAGE_SIZE       0x1000
#define PAGE_ROUNDUP(n) ((n)%PAGE_SIZE==0 ? (n) : ((n)+PAGE_SIZE)&~(PAGE_SIZE-1))

/* Struct for mapped peripheral or memory */
typedef struct {
    int   fd,
          h;
    size_t  size;
    void* bus;
    void* virt;
    void* phys;
} MEM_MAP;

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

typedef struct {
    uint8_t read;
    uint8_t out_pixels;
} smi_pack_ratio_t;

typedef struct {
    volatile uint32_t   active          : 1,
                        end             : 1,
                        intr            : 1,
                        dreq            : 1,
                        pause           : 1,
                        dreqstop        : 1,
                        waiting_write   : 1,
                        _x1             : 1,
                        error           : 1,
                        _x2             : 7,
                        priority        : 4,
                        panic_priority  : 4,
                        _x3             : 4,
                        wait_write      : 1,
                        disdebug        : 1,
                        abort           : 1,
                        reset           : 1;
} DMA_CS_BITFIELD;

typedef union {
    DMA_CS_BITFIELD fields;
    volatile uint32_t value;
} DMA_CS __attribute__((aligned(32)));

typedef struct {
    uint32_t ti,        // Transfer info
             src_addr,  // Source address
             dest_addr, // Desitination address
             tfr_len,   // Transfer length
             stride,    
             next_cb,   // Next control block
             debug,     // Debug register
             unused;
} DMA_CB __attribute__ ((aligned(32)));

/* --- Timeouts --- */
#if !defined(_POSIX_VERSION) || _POSIX_VERSION < 199309L
    #error "POSIX verison is insufficient for the required clock_gettime()"
#endif
typedef struct timespec smi_timeout;
typedef int64_t smi_timeout_ns;

static inline smi_timeout_ns start_timeout(double seconds)
{
    smi_timeout now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t)now.tv_sec * 1000000000LL +
           now.tv_nsec +
           (int64_t)(seconds * 1000000000.0);
}

static inline int timeout_complete(smi_timeout_ns deadline)
{
    smi_timeout now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int64_t now_ns = (int64_t)now.tv_sec * 1000000000LL + now.tv_nsec;

    return now_ns >= deadline;
}

/* --- SMI Setup Helpers --- */
void smi_init_cxt_map(SMI_CXT* cxt, MEM_MAP* smi_regs, MEM_MAP* clk_regs, MEM_MAP* gpio_regs, MEM_MAP* dma_regs);
void smi_init_rw_config(SMI_CXT* cxt, SMI_RW* rw, SMI_CLK* clk, SMI_READ* rconfig, SMI_WRITE* wconfig, int read_device, int write_device);
void* map_segment(MEM_MAP* map, uintptr_t addr, int size);
void unmap_segment(void *mem, int size);

/* --- SMI Destructors --- */
void smi_unmap_cxt(SMI_CXT* cxt);

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

/* --- Implementation --- */
void* map_segment(MEM_MAP* map, uintptr_t addr, int size)
{
    int fd;
    void* mem;

    size = PAGE_ROUNDUP(size);
    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        ERROR("Could not open /dev/");
        exit(1);
    }

    mem = mmap(0, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, (uintptr_t)addr);
    close(fd);

    if(mem == MAP_FAILED)
    {
        ERROR("Could not map memory");
        exit(1);
    }

    map->virt = mem;
    map->bus = NULL;
    map->phys = (void*)addr;
    map->size = (size_t) size;

    return mem;
}

void unmap_segment(void *mem, int size)
{
    if (mem)
        munmap(mem, PAGE_ROUNDUP(size));
}

int smi_read_await(SMI_CXT* cxt, uint32_t* ret_data, int len)
{
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_D*  d = (volatile SMI_D*) REG32((*cxt->smi_regs), SMIO_D);

    if(ret_data == NULL) return -EINVAL;

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    
    deadline = start_timeout(PROG_READ_TIMEOUT_S);

    /* We can assume that if data is leftover in the FIFO is hardware fault? */
    while(count < len)
    {

        if (cs->fields.rxd) 
        {
            volatile uint32_t word = d->value;
            ret_data[count++] = word;
        }

        if(spin > 1024)
        {
            if(timeout_complete(deadline))
            {
                ERROR("Programmed read data fetch timeout");
                cs->fields.clear = 1;
                return -ETIMEDOUT;
            }
        }

        spin++;
    }

    if(cs->fields.rxd)
    {
        ERROR("Leftover data in the RX FIFO");
    }
    

    return count;
}

int smi_read_await_direct(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len, int increment)
{
    if(ret_data == NULL || cxt == NULL) return -1;

    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32((*cxt->smi_regs), SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32((*cxt->smi_regs), SMIO_DD);

    int count = 0;
    int spin = 0;

    dcs->fields.write = 0;
    da->fields.addr = addr;

    smi_timeout_ns deadline;
    deadline = start_timeout(DIRECT_READ_TIMEOUT_S);

    while(count < len)
    {
        dcs->fields.start = 1;
        
        while (!dcs->fields.done)
        {
            if(spin > 1024)
            {
                if(timeout_complete(deadline))
                {
                    ERROR("Direct read timeout");
                    cs->fields.clear = 1;
                    dcs->fields.done = 1;
                    return -ETIMEDOUT;
                }
                spin = 0;
            }
            spin++;
        }

        uint32_t val = dd->value & 0xFF;
        ret_data[count++] = val;
        
        dcs->fields.done = 1;

        if(increment > 0) {addr++; da->fields.addr = addr;}
        else if (increment < 0) {addr--; da->fields.addr = addr;};
    }

    return count;
}

int smi_write_await(SMI_CXT* cxt, uint32_t* data, uint8_t addr, int len)
{
    if(data == NULL || cxt == NULL) return -1;

    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_D*  d  = (volatile SMI_D*)  REG32((*cxt->smi_regs), SMIO_D);

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    deadline = start_timeout(PROG_WRITE_TIMEOUT_S);
    
    while(!cs->fields.done)
    {
        if(count < len + 1)
        {
            d->value = data[count++];
        }

        while(!cs->fields.txd)
        {
            if(spin > 1024)
            {
                if(timeout_complete(deadline))
                {
                    ERROR("Programmed write await timeout - TX is full");
                    cs->fields.clear = 1;
                    cs->fields.done = 1;
                    return -ETIMEDOUT;
                }
                spin = 0;
            }

            spin++;
        }
        
        if(spin > 1024)
        {
            if(timeout_complete(deadline))
            {
                ERROR("Programmed write await timeout");
                cs->fields.clear = 1;
                cs->fields.done = 1;
                return -ETIMEDOUT;
            }
            spin = 0;
        }

        if (cs->fields.aferr)
        {
            ERROR("Protected registers written to during write");
            cs->fields.aferr = 1;
            cs->fields.clear = 1;
            return count;
        }

        spin++;
    }

    return count;
}

int smi_write_await_direct(SMI_CXT* cxt, uint32_t* data, uint8_t addr, int len, int increment)
{
    if(data == NULL || cxt == NULL) return -1;

    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32((*cxt->smi_regs), SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32((*cxt->smi_regs), SMIO_DD);

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    deadline = start_timeout(PROG_WRITE_TIMEOUT_S);

    dcs->fields.write = 1;

    while(count < len)
    {
        dd->value = data[count++];
        dcs->fields.start = 1;
        
        while(!dcs->fields.done)
        {
            if(spin > 1024)
            {
                if(timeout_complete(deadline))
                {
                    ERROR("Direct read array timeout");
                    cs->fields.clear = 1;
                    dcs->fields.done = 1;
                    return -ETIMEDOUT;
                }
            }
        }

        if(increment > 0) {addr++; da->fields.addr = addr;}
        else if (increment < 0) {addr--; da->fields.addr = addr;};

        spin++;
    }

    return count;
}

int smi_dma_await(SMI_CXT* cxt)
{
    volatile DMA_CS* dma_cs = (volatile DMA_CS*) REG32((*cxt->dma_regs), DMAO_CS);

    long delay = 1000;    
    smi_timeout_ns deadline;
    
    deadline = start_timeout(PROG_READ_TIMEOUT_S);

    while(!dma_cs->fields.end)
    {
        if(timeout_complete(deadline))
        {
            ERROR("DMA transfer timeout");
            dma_cs->fields.abort = 1;
            return -ETIMEDOUT;
        }

        struct timespec ts = {0, delay};
        nanosleep(&ts, NULL);

        if(delay < 100000) delay *= 2;
    }
    
    return 1;
}

void smi_unpack_rgb565_8(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint8_t* dst = (uint8_t*) out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];
        uint8_t b1 = (word >>  0) & 0xFF;
        uint8_t b0 = (word >>  8) & 0xFF;
        uint8_t b3 = (word >> 16) & 0xFF;
        uint8_t b2 = (word >> 24) & 0xFF;

        dst[0] = b0;
        dst[1] = b1;
        dst[2] = b2;
        dst[3] = b3;

        dst += 4;
    }
}

void smi_unpack_xrgb_8(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint8_t* dst = (uint8_t*) out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];

        uint8_t b2 = (word >>  0) & 0xFF;
        uint8_t b1 = (word >>  8) & 0xFF;
        uint8_t b0 = (word >> 16) & 0xFF;

        dst[0] = b0;
        dst[1] = b1;
        dst[2] = b2;

        dst += 3;
    }
}

/* 
    Data1 = { FIFO[12:10], FIFO[7:2] } 
    Data2 = { FIFO[23:18], FIFO[15:13] }
*/
void smi_unpack_xrgb_9(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = (uint16_t*) out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];

        uint16_t d0 = ((word >> 13) & 0x7) | ((word >> 15) & 0x1F8);
        uint16_t d1 = ((word >> 2) & 0x3F) | ((word >> 4) & 0x1C0);
        dst[0] = d0;
        dst[1] = d1;
        dst += 2;
    }
}

/* 
    Data1 = { FIFO[23:18], FIFO[15:13] }
    Data2 = { FIFO[12:10], FIFO[7:2] } 
*/
void smi_unpack_xrgb_9_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = (uint16_t*) out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];

        uint16_t d0 = ((word >> 2) & 0x3F) | ((word >> 4) & 0x1C0);
        uint16_t d1 = ((word >> 13) & 0x7) | ((word >> 15) & 0x1F8);
        dst[0] = d0;
        dst[1] = d1;
        dst += 2;
    }
}

/* 
    RGB565 includes repeated bits
    Data0 = { FIFO[15:11], FIFO[15], FIFO[10:8] }
    Data1 = { FIFO[7:0], FIFO[4] }
    Data2 = { FIFO[31:27], FIFO[31], FIFO[26:24] }
    Data3 = { FIFO[23:16], FIFO[20] }
*/
static inline void smi_unpack_rgb565_9_word(uint32_t word, uint16_t out[4])
{
    uint8_t b0 = (word >>  0) & 0xFF;
    uint8_t b1 = (word >>  8) & 0xFF;
    uint8_t b2 = (word >> 16) & 0xFF;
    uint8_t b3 = (word >> 24) & 0xFF;
    
    uint16_t d1 = (b0 << 1) | ((b0 >> 7) & 0x1);
    uint16_t d3 = (b2 << 1) | ((b2 >> 7) & 0x1);

    uint16_t d0 = ((b1 << 1) & 0x1F0) | ((b1 >> 2) & 0x8) | ((b1) & 0x7);
    uint16_t d2 = ((b3 << 1) & 0x1F0) | ((b3 >> 2) & 0x8) | ((b3) & 0x7);

    out[0] = d0;
    out[1] = d1;
    out[2] = d2;
    out[3] = d3;
}

void smi_unpack_rgb565_9(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = (uint16_t*) out;
    uint16_t word_buffer[4];

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];
        smi_unpack_rgb565_9_word(word, word_buffer);

        //printf("%u ; %u ; %u ; %u\n", word_buffer[0], word_buffer[1], word_buffer[2], word_buffer[3]);

        dst[0] = word_buffer[0];
        dst[1] = word_buffer[1];
        dst[2] = word_buffer[2];
        dst[3] = word_buffer[3];

        dst += 4;
    }
}

/* 
    RGB565 includes repeated bits
    Data1 = { FIFO[15:11], FIFO[15], FIFO[10:8] }
    Data2 = { FIFO[7:0], FIFO[4] }
    Data3 = { FIFO[15:11], FIFO[15], FIFO[10:8] } 
    Data4 = { FIFO[23:16], FIFO[20] }
*/
static inline void apply_swap_16(uint16_t w[4])
{
    uint16_t t;

    t = w[0]; w[0] = w[1]; w[1] = t;
    t = w[2]; w[2] = w[3]; w[3] = t;
}

void smi_unpack_rgb565_9_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = (uint16_t*) out;
    uint16_t word_buffer[4];

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];

        smi_unpack_rgb565_9_word(word, word_buffer);
        //printf("%u ; %u ; %u ; %u\n", word_buffer[0], word_buffer[1], word_buffer[2], word_buffer[3]);

        dst[0] = word_buffer[1];
        dst[1] = word_buffer[0];
        dst[2] = word_buffer[3];
        dst[3] = word_buffer[2];

        dst += 4;
    }
}

void smi_unpack_xrgb_16(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = (uint16_t*) out;
    size_t pairs = count / 2;

    for (size_t i = 0; i < pairs; i++)
    {
        uint32_t word  = raw[2*i];
        uint32_t word2 = raw[2*i + 1];

        uint16_t d0 = (word >> 8) & 0xFFFF;
        uint16_t d1 = ((word << 8) & 0xFF00) | ((word2 >> 16) & 0x00FF);
        uint16_t d2 = word2 & 0xFFFF;

        dst[0] = d0;
        dst[1] = d1;
        dst[2] = d2;

        dst += 3;
    }
}

void smi_unpack_rgb565_16(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = (uint16_t*) out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word  = raw[i];

        uint16_t d0 = ((word >> 0) & 0xFFFF);
        uint16_t d1 = ((word >> 16) & 0xFFFF);

        dst[0] = d0;
        dst[1] = d1;

        dst += 2;
    }
}

void smi_unpack_xrgb_18(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint32_t* dst = (uint32_t*) out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word  = raw[i];

        uint32_t d0 = ((word >> 2) & 0x3F) | ((word >> 4) & 0xFC0) | ((word >> 6) & 0x3F000);

        dst[0] = d0;

        dst += 1;
    }
}

void smi_unpack_rgb565_18(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint32_t* dst = (uint32_t*) out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word  = raw[i];
        uint32_t w0 = (word >> 0) & 0xFFFF;
        uint32_t w1 = (word >> 16) & 0xFFFF;

        uint32_t d0 = ((w0 << 2)  & 0x3E000) | ((w0 >> 3)  & 0x1000) | ((w0 << 1)  & 0xFFE);
        uint32_t d1 = ((w1 << 2)  & 0x3E000) | ((w1 >> 3)  & 0x1000) | ((w1 << 1)  & 0xFFE);
        
        dst[0] = d0;
        dst[1] = d1;

        dst += 2;
    }
}

/* RGB565 18 bit swap mode is not correctly implemented - Bit order is reversed however unpacking is still a mystery */
static inline uint32_t reverse18(uint32_t v)
{
    v = ((v & 0x15555) << 1) | ((v & 0x2AAAA) >> 1);
    v = ((v & 0x03333) << 2) | ((v & 0x0CCCC) >> 2);
    v = ((v & 0x00F0F) << 4) | ((v & 0x0F0F0) >> 4);
    v = ((v & 0x000FF) << 8) | ((v & 0x3FC00) >> 8);
    return v;
}

void smi_unpack_rgb565_18_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint32_t* dst = (uint32_t*) out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word  = raw[i];
        uint32_t w0 = reverse18((word >> 0) & 0xFFFF);
        uint32_t w1 = reverse18((word >> 16) & 0xFFFF);

        uint32_t d0 = ((w0 << 2)  & 0x3E000) | ((w0 >> 3)  & 0x1000) | ((w0 << 1)  & 0xFFE);
        uint32_t d1 = ((w1 << 2)  & 0x3E000) | ((w1 >> 3)  & 0x1000) | ((w1 << 1)  & 0xFFE);

        dst[0] = d0;
        dst[1] = d1;

        dst += 2;
    }
}

void smi_unpack(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count, smi_pack_ratio_t ratio)
{
    int width = cxt->rw_config->rconfig->rwidth;
    int format = cxt->rw_config->wconfig->wformat;
    int swap = cxt->rw_config->wconfig->wswap;

    switch (width)
    {
    case SMI_8_BITS:
        if (format == SMI_RGB565) smi_unpack_rgb565_8(data, ret_data, count, ratio);
        if (format == SMI_XRGB)   smi_unpack_xrgb_8(data, ret_data, count, ratio);
        return;

    case SMI_9_BITS:
        if (format == SMI_RGB565) (swap == 0) ? 
                smi_unpack_rgb565_9(data, ret_data, count, ratio) :
                smi_unpack_rgb565_9_swap(data, ret_data, count, ratio);
        if (format == SMI_XRGB)   (swap == 0) ? 
                smi_unpack_xrgb_9(data, ret_data, count, ratio) :
                smi_unpack_xrgb_9_swap(data, ret_data, count, ratio); 
        return;

    case SMI_16_BITS:
        if (format == SMI_RGB565) smi_unpack_rgb565_16(data, ret_data, count, ratio);
        if (format == SMI_XRGB)   smi_unpack_xrgb_16(data, ret_data, count, ratio);
        return;

    case SMI_18_BITS:
        if (format == SMI_RGB565) (swap == 0) ? 
                smi_unpack_rgb565_18(data, ret_data, count, ratio) :
                smi_unpack_rgb565_18_swap(data, ret_data, count, ratio);
        if (format == SMI_XRGB)   smi_unpack_xrgb_18(data, ret_data, count, ratio);
        return;
    }

    ERROR("Unknown interface configuration - could not unpack data");
    return;
}

smi_pack_ratio_t smi_packed_ratio(SMI_CXT* cxt)
{
    int width = cxt->rw_config->rconfig->rwidth;
    int format = cxt->rw_config->wconfig->wformat;

    if(cxt->pxldata == 0){ return (smi_pack_ratio_t){1,1}; }

    switch (width)
    {
    case SMI_8_BITS:
        if (format == SMI_RGB565) return (smi_pack_ratio_t){1, 4};
        if (format == SMI_XRGB)   return (smi_pack_ratio_t){1, 3};
        break;

    case SMI_9_BITS:
        if (format == SMI_RGB565) return (smi_pack_ratio_t){1, 4};
        if (format == SMI_XRGB)   return (smi_pack_ratio_t){1, 2};
        break;

    case SMI_16_BITS:
        if (format == SMI_RGB565) return (smi_pack_ratio_t){1, 2};
        if (format == SMI_XRGB)   return (smi_pack_ratio_t){2, 3};
        break;

    case SMI_18_BITS:
        if (format == SMI_RGB565) return (smi_pack_ratio_t){1, 2};
        if (format == SMI_XRGB)   return (smi_pack_ratio_t){1, 1};
        break;
    }


    ERROR("Unknown interface configuration - using 1:1 ratio");
    return (smi_pack_ratio_t){1,1};
}

void smi_truncate_8(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    uint8_t* out = (uint8_t*) ret_data;
    for(size_t i = 0; i < count; i++)
    {
        out[i] = data[i] & 0xFF;
    }
}

void smi_truncate_9(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    uint16_t* out = (uint16_t*) ret_data;
    for(size_t i = 0; i < count; i++)
    {
        out[i] = data[i] & 0x1FF;
    }
}

void smi_truncate_16(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    uint16_t* out = (uint16_t*) ret_data;
    for(size_t i = 0; i < count; i++)
    {
        out[i] = data[i] & 0xFFFF;
    } 
}

void smi_truncate_18(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    uint32_t* out = (uint32_t*) ret_data;
    for(size_t i = 0; i < count; i++)
    {
        out[i] = data[i] & 0x3FFFF;
    }
}

void smi_truncate(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    int width = cxt->rw_config->rconfig->rwidth;
        
    switch (width)
    {
    case SMI_8_BITS:
        smi_truncate_8(cxt, data, ret_data, count);
        break;

    case SMI_9_BITS:
        smi_truncate_9(cxt, data, ret_data, count);
        break;

    case SMI_16_BITS:
        smi_truncate_16(cxt, data, ret_data, count);
        break;

    case SMI_18_BITS:
        smi_truncate_18(cxt, data, ret_data, count);
        break;

    default:
        ERROR("Unknown interface configuration - returning untruncated data");
    }

    return;
}

#endif