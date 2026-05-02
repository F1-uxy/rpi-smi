#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "dma.h"
#include "gpio.h"
#include "smi.h"
#include "errors.h"

void* map_dma_buffer(size_t buf_size)
{
    int fd = 0;
    int openFlags = O_RDWR;

    #if defined(PI_ARM32)
        openFlags |= O_SYNC;
    #endif
    

    if((fd = open("/dev/udmabuf0", openFlags)) < 0)
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

int start_dma(volatile void* dma_regs, uintptr_t cb, uint8_t channel, int fd_sync_dev, int fd_sync_cpu)
{
    if(cb == 0)
    {
        perror("ERROR: CB Null Pointer\n");
        return -1;
    }

    if (channel > 14)
    {
        perror("ERROR: Channel out of range\n"); 
        return - 1;
    }

    #if defined(PI_ARM64)
        sync_for_cpu(fd_sync_cpu);
    #endif

    volatile DMA_CS* dma_cs = (volatile DMA_CS*) ((uintptr_t)dma_regs + DMA_CS_OFFSET(channel));
    volatile DMA_CONBLK_AD* dma_conblk_ad = (volatile DMA_CONBLK_AD*) ((uintptr_t)dma_regs + DMA_CTRL_BLK(channel));

    if (dma_cs->fields.active)
    {
        ERROR("DMA already active");
        return -1;
    }

    dma_conblk_ad->fields.scb_addr = (uint32_t)cb;

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
    

    printf("Buffer size: %zu\n", buff->size);
    printf("Virtual Address: %p\n", buff->virt);
    printf("Physical Address: %p\n", buff->phys);
    printf("Bus Address: %p\n", buff->bus);

    return buff->size;
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
