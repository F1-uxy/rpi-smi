#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "gpio.h"
#include "dma.h"

size_t read_sysfile_size(const char* file)
{
    FILE* f = fopen(file, "r");
    if(!f)
    {
        perror("ERROR: File size couldn't be read\n");
        return -1;
    }

    size_t size = 0;
    fscanf(f, "%zu", &size);
    fclose(f);
    
    return size;
}

void* read_sysfile_phys_addr(const char* file)
{
    FILE* f = fopen(file, "r");
    if(!f)
    {
        perror("ERROR: File size couldn't be read\n");
        return NULL;
    }

    void* location;
    fscanf(f, "%p", &location);
    fclose(f);
    
    return location;
}

int write_sync(int fd, uint64_t val)
{
    unsigned char attr[1024];

    if(fd < 0)
    {
        perror("ERROR: File size couldn't be read\n");
        return -1;
    }

    sprintf(attr, "%ld", val);
    write(fd, attr, 1024);

    return 0;
}

int sync_for_cpu(int fd)
{
    return write_sync(fd, 1);
}

int sync_for_device(int fd)
{
    return write_sync(fd, 1);
}

void* map_segment(MEM_MAP* map, uintptr_t addr, int size)
{
    int fd;
    void* mem;

    size = PAGE_ROUNDUP(size);
    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        perror("Error: Could not open /dev/\n");
        exit(1);
    }

    mem = mmap(0, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, (uintptr_t)addr);
    close(fd);

    if(mem == MAP_FAILED)
    {
        perror("Error: Could not map memory\n");
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

void gpio_mode(MEM_MAP gpio_regs, int pin, int mode)
{
    volatile uint32_t* reg = REG32(gpio_regs, GPIO_MODE0) + pin / 10, shift = (pin % 10) * 3;
    *reg = (*reg & ~(7 << shift)) | (mode << shift);
}

void gpio_set(MEM_MAP gpio_regs, int pin, int val)
{
    volatile uint32_t* reg = REG32(gpio_regs, val ? GPIO_SET0 : GPIO_CLR0) + pin/32;
    *reg = 1 << (pin % 32);
}

uint8_t gpio_read(MEM_MAP gpio_regs, int pin)
{
    volatile uint32_t* reg = REG32(gpio_regs, GPIO_LEV0) + pin/32;
    return (((*reg) >> (pin % 32)) & 1);
}


void disp_mode_pins(uint32_t mode)
{
    char *gpio_mode_strs[] = {GPIO_MODE_STRS};

    for(int i = 0; i < 10; i++)
    {
        printf("%u:%-4s ", i, gpio_mode_strs[(mode>>(i*3)) & 7]);
    }

    printf("\n");
}

/* Blinks given pin for 10s */
int gpio_test(MEM_MAP gpio_regs, int pin)
{
    if(pin > 53)
    {
        perror("Pin out of range\n");
        return -1;
    }

    gpio_mode(gpio_regs, pin, GPIO_OUT);

    int i = 0;
    while(i < 5)
    {
        gpio_set(gpio_regs, pin, 1);
        sleep(1);
        gpio_set(gpio_regs, pin, 0);
        sleep(1);
        i += 1;
    }

    return 1;
}