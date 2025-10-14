#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <pigpio.h>
#include "dma.h"

void* map_segment(void* addr, int size)
{
    int fd;
    void* mem;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC | __O_CLOEXEC)) < 0)
    {
        perror("Error: Could not open /dev/mem, sudo required\n");
        return NULL;
    }

    mem = mmap(0, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, (uint32_t) addr);
    close(fd);

    return (mem);
}

int main()
{
    /* physical address = bus_address - BUS_BASE + PHYSICAL_BASE*/
    void* virt_gpio_regs;
    virt_gpio_regs = map_segment((void*)GPIO_BASE, PAGE_SIZE);
    BUS_PHYS(0x7E205000);
    
    return 0;
}