#ifndef DMA_H
#define DMA_H

#define PHYS_REG_BASE   0x3F000000
#define GPIO_BASE       (PHYS_REG_BASE + 0x200000)
#define PAGE_SIZE       0x1000

#define BUS_REG_BASE 0x74000000

#define BUS2PHYS(bus) (bus - BUS_REG_BASE) + PHYS_REG_BASE 

#endif