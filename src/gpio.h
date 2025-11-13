#ifndef GPIO_H
#define GPIO_H

#define PHYS_REG_BASE   PI_23_REG_BASE
#define PI_23_REG_BASE  0x3F000000      /* Pi 3 */

/* Location of peripheral registers in physical memory*/
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

#endif