#ifndef GPIO_H
#define GPIO_H

#if defined(PI3)
    #define PHYS_REG_BASE 0x3F000000u
#elif defined(PI4)
    #define PHYS_REG_BASE 0xFE000000u
#elif defined(PI5)
    #error "Pi 5 requires different peripheral mapping"
#endif

#define BUS_REG_BASE 0x7E000000

#if defined(__aarch64__)
    #define PI_ARM64
#elif defined(__arm__)
    #define PI_ARM32
#endif

/* Location of peripheral registers in physical memory*/
#define CLOCK_HZ 250000000       /* Pi 3 or Clamp to this on Pi 5 */

/* Location of peripheral registers in bus memory */
//#define BUS_REG_BASE 0x74000000

/* Struct for mapped peripheral or memory */
typedef struct {
    int   fd,
          h;
    size_t  size;
    void* bus;
    void* virt;
    void* phys;
} MEM_MAP;

#define SMI_PACKED __attribute__((packed))
#define SMI_ALIGNED __attribute__ ((aligned(4)))

#define REG32(m, x) ((volatile uint32_t*) ((uintptr_t)(m.virt)+(uintptr_t)(x)))
#define REG32_BUS(m, x) ((volatile uint32_t*) ((uintptr_t)(m.bus))  + (uint32_t)(x))

/* Get bus address of register */
#define REG_BUS_ADDR(m, x)  ((uint32_t)(m.bus) + (uint32_t)(x))

/* Convert uncached memory virtual address to bus address - Also downcast to 32 bit for DMA */
#define MEM_BUS_ADDR(mp, a) ((uint32_t)a-(uint32_t)mp->virt+(uint32_t)mp->bus)

/* Convert bus address to physical address (for mmap) */
#define BUS_PHYS_ADDR(a)    ((void*)((uint32_t)(a)&~0xC0000000))

/* Size of memory page */
#define PAGE_SIZE       0x1000
/* Round up to nearest page */
#define PAGE_ROUNDUP(n) ((n)%PAGE_SIZE==0 ? (n) : ((n)+PAGE_SIZE)&~(PAGE_SIZE-1))

#define REG_SETBIT(reg, n)        (reg |= n)
#define REG_SETFIELD(reg, offset, width, value) \
    do { \
        uint32_t __mask = ((uint32_t)(((1ULL << (width)) - 1) << (offset))); \
        (reg) = ((reg) & ~__mask) | (((value) & ((1U << (width)) - 1)) << (offset)); \
    } while (0)

/* GPIO Register Definitions */
#define GPIO_BASE       (PHYS_REG_BASE + 0x200000)
#define GPIO_MODE0      0x00
#define GPIO_SET0       0x1c
#define GPIO_CLR0       0x28
#define GPIO_LEV0       0x34
#define GPIO_GPPUD      0x94
#define GPIO_GPPUDCLK0  0x98

/* GPIO I/O Definitions */
#define GPIO_IN         0
#define GPIO_OUT        1
#define GPIO_ALT0       4
#define GPIO_ALT1       5
#define GPIO_ALT2       6
#define GPIO_ALT3       7
#define GPIO_ALT4       3
#define GPIO_ALT5       2
#define GPIO_MODE_STRS  "IN","OUT","ALT5","ALT4","ALT0","ALT1","ALT2","ALT3"

#define SMI_SOE_PIN     6   /* Read enable  */
#define SMI_SWE_PIN     7   /* Write enable */

size_t read_sysfile_size(const char* file);
void* read_sysfile_phys_addr(const char* file);
int sync_for_device(int fd);
int sync_for_cpu(int fd);


void* map_segment(MEM_MAP* map, uintptr_t addr, int size);
void unmap_segment(void *mem, int size);

void gpio_mode(MEM_MAP gpio_regs, int pin, int mode);

void gpio_set(MEM_MAP gpio_regs, int pin, int val);

uint8_t gpio_read(MEM_MAP gpio_regs, int pin);

void disp_mode_pins(uint32_t mode);

int gpio_test(MEM_MAP gpio_regs, int pin);

#endif
