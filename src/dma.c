#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "dma.h"

#define UDMABUF_SYS "/sys/class/u-dma-buf/udmabuf0/"
#define SIZE_FILE "size"
#define PHYS_ADDR "phys_addr"
#define SYNC_CPU "sync_for_cpu"
#define SYNC_DEVICE "sync_for_device"



void* map_dma_buffer(size_t buf_size)
{
    int fd = 0;

    if((fd = open("/dev/udmabuf0", O_RDWR | O_SYNC)) < 0)
    {
        perror("ERROR: failed opening /dev/udmabuf0\n");
        return NULL;    
    }

    void* buf = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(buf == MAP_FAILED)
    {
        perror("ERROR: buffer mapping failed\n");
        close(fd);
        return NULL;
    }

    close(fd);
    return buf;
}


int start_dma(MEM_MAP* dma_buffer, MEM_MAP dma_regs, uint8_t channel, DMA_CB* cb)
{
    if(cb == NULL)
    {
        perror("ERROR: CB Null Pointer\n");
        return -1;
    }

    if (channel > 14)
    {
        perror("ERROR: Channel out of range\n"); 
        return - 1;
    }

    uintptr_t offset = DMA_CS_OFFSET(channel);
    volatile DMA_CS* dma_cs = (volatile DMA_CS*) REG32(dma_regs, offset);

    if (dma_cs->fields.active)
    {
        perror("ERROR: DMA already active\n");
        return -1;
    }

    offset = DMA_CTRL_BLK(channel);
    volatile DMA_CONBLK_AD* dma_conblk_ad = (volatile DMA_CONBLK_AD*) REG32(dma_regs, offset);

    dma_conblk_ad->value = (uint32_t)dma_buffer->bus; 
    dma_cs->fields.active = 1;

    return 0;
}

size_t dma_buffer_init(MEM_MAP* buff, int check, int clear)
{
    buff->virt = map_dma_buffer(DMA_BUFFER_SIZE);

    if(buff->virt == NULL)
    {
        perror("Error mapping segment\n");
        return -1;
    }

    buff->size = read_sysfile_size(UDMABUF_SYS SIZE_FILE);
    buff->phys = read_sysfile_phys_addr(UDMABUF_SYS PHYS_ADDR);
    buff->bus = buff->phys + 0xC0000000;

    if(check)
    {
        int error_count = check_buf((unsigned char*)buff->virt, DMA_BUFFER_SIZE);
        if(error_count > 0)
        {
            fprintf(stderr, "ERROR: Buffer initialisation check failed; Count = %d\n", error_count);
            return -1;
        }
    }

    if(clear)
    {
        int error_count = clear_buf((unsigned char*)buff->virt, DMA_BUFFER_SIZE);
        if(error_count > 0)
        {
            fprintf(stderr, "ERROR: Buffer initialisation clear failed; Count = %d\n", error_count);
            return -1;
        }
    }
    
    return buff->size;

    /*
    printf("Buffer size: %zu\n", dma_buffer.size);
    printf("Virtual Address: %p\n", dma_buffer.virt);
    printf("Physical Address: %p\n", dma_buffer.phys);
    printf("Bus Address: %p\n", dma_buffer.bus);
    */
}

int check_buf(unsigned char* buf, unsigned int size)
{
    int m = 256;
    int n = 10;
    int i, k;
    int error_count = 0;
    while(--n > 0) 
    {
        for(i = 0; i < size; i = i + m) 
        {
            m = (i+256 < size) ? 256 : (size-i);
            for(k = 0; k < m; k++) 
            {
                buf[i+k] = (k & 0xFF);
            }
            for(k = 0; k < m; k++) 
            {
                if (buf[i+k] != (k & 0xFF)) 
                {
                    error_count++;
                }
            }
        }
    }
    return error_count;
}

int clear_buf(unsigned char* buf, unsigned int size)
{
    int n = 100;
    int error_count = 0;
    while(--n > 0) {
        memset((void*)buf, 0, size);
    }
    return error_count;
}