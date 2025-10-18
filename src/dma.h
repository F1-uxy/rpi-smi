#ifndef DMA_H
#define DMA_H

#define DMA_BUFFER_SIZE 1024

#define PRINT_BITS_32(x)  \
    printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", \
    (x & 0x80000000) ? '1':'0', (x & 0x40000000) ? '1':'0', (x & 0x20000000) ? '1':'0', (x & 0x10000000) ? '1':'0', \
    (x & 0x08000000) ? '1':'0', (x & 0x04000000) ? '1':'0', (x & 0x02000000) ? '1':'0', (x & 0x01000000) ? '1':'0', \
    (x & 0x00800000) ? '1':'0', (x & 0x00400000) ? '1':'0', (x & 0x00200000) ? '1':'0', (x & 0x00100000) ? '1':'0', \
    (x & 0x00080000) ? '1':'0', (x & 0x00040000) ? '1':'0', (x & 0x00020000) ? '1':'0', (x & 0x00010000) ? '1':'0', \
    (x & 0x00008000) ? '1':'0', (x & 0x00004000) ? '1':'0', (x & 0x00002000) ? '1':'0', (x & 0x00001000) ? '1':'0', \
    (x & 0x00000800) ? '1':'0', (x & 0x00000400) ? '1':'0', (x & 0x00000200) ? '1':'0', (x & 0x00000100) ? '1':'0', \
    (x & 0x00000080) ? '1':'0', (x & 0x00000040) ? '1':'0', (x & 0x00000020) ? '1':'0', (x & 0x00000010) ? '1':'0', \
    (x & 0x00000008) ? '1':'0', (x & 0x00000004) ? '1':'0', (x & 0x00000002) ? '1':'0', (x & 0x00000001) ? '1':'0')


/* Location of peripheral registers in physical memory*/
#define PHYS_REG_BASE   PI_23_REG_BASE
#define PI_23_REG_BASE  0x3F000000      /* Pi 3 */
#define CLOCK_HZ        250000000       /* Pi 3 */

/* Location of peripheral registers in bus memory */
#define BUS_REG_BASE 0x74000000

/* Struct for mapped peripheral or memory */
typedef struct {
    int   fd,
          h;
    size_t  size;
    void* bus;
    void* virt;
    void* phys;
} MEM_MAP;

#define REG32(m, x) ((volatile uint32_t*) ((uint32_t)(m.virt)+(uint32_t)(x)))

/* Get bus address of register */
#define REG_BUS_ADDR(m, x)  ((uint32_t)(m.bus) + (uint32_t)(x))

/* Convert uncached memory virtual address to bus address */
#define MEM_BUS_ADDR(mp, a) ((uint32_t)a-(uint32_t)mp->virt+(uint32_t)mp->bus)

/* Convert bus address to physical address (for mmap) */
#define BUS_PHYS_ADDR(a)    ((void*)((uint32_t)(a)&~0xC0000000))

/* Size of memory page */
#define PAGE_SIZE       0x1000
/* Round up to nearest page */
#define PAGE_ROUNDUP(n) ((n)%PAGE_SIZE==0 ? (n) : ((n)+PAGE_SIZE)&~(PAGE_SIZE-1))


/*
** DMA controller
*/

#define NADJ_CHANNELS 14 /* Number of adjacent DMA channels (15th channel physically removed)*/

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

#define DMA_CB_DEST_INC (1<<4)
#define DMA_CB_SRC_INC  (1<<8)

#define DMA_N_REG(base, n) ((uint32_t*)((char*)base + (n * 0x100)))

#define DMA_BASE        0x3F007000
#define DMA0_BASE       0x7E007000  /* DMA Channel 0 Register Set offset        */
#define DMA_REG_CS      0x00        /* DMA Channel 0 Control and Status offset  */
#define DMA_CONBLK_AD   0x04        /* DMA Channel 0 Control Block Address      */
#define DMA0_DEBUG      0x20        /* DMA Channel 0 Debug                      */

/* DMA Control and Status register bitmap */
#define CS_SETBIT(cs, n)        (cs |= n)
#define CS_CR                   (1 << 31)   /* DMA Channel Reset */
#define CS_ABORT                (1 << 30)
#define CS_DISDEBUG             (1 << 29)
#define CS_WFOW                 (1 << 28)
#define CS_PANIC_PRIORITY       (4 << 20)
#define CS_PRIORITY             (4 << 16)
#define CS_ERROR                (1 << 8)
#define CS_WGFOW                (1 << 6)
#define CS_DREQ_STOP            (1 << 5)
#define CS_PAUSED               (1 << 4)
#define CS_DREQ                 (1 << 3)
#define CS_INT                  (1 << 2)
#define CS_END                  (1 << 1)
#define CS_ACTIVE               (1 << 0)

#endif