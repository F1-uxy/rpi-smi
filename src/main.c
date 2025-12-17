#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

//#include <pigpio.h>
#include "dma.h"
#include "dma_helpers.h"
#include "smi.h"
#include "clk.h"

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


int start_dma(MEM_MAP* dma_buffer, void* dma_regs, uint8_t channel, DMA_CB* cb)
{
    if(cb == NULL || dma_regs == NULL)
    {
        perror("ERROR: Null pointer passed to dma\n");
        return -1;
    }

    if (channel > 14)
    {
        perror("ERROR: Channel out of range\n"); 
        return - 1;
    }

    volatile uint32_t* dma_cs = DMA_N_REG(dma_regs, channel);
    volatile uint32_t* dma_conblk_ad = (volatile uint32_t*)((char*)dma_cs + 0x04);
    *dma_conblk_ad = (uint32_t)dma_buffer->bus; 

    REG_SETBIT(*dma_cs, CS_ACTIVE);

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

int main()
{
    MEM_MAP dma_buffer, dma_regs, clk_regs, smi_regs, gpio_regs;
    
    /* Map u-dma buffer */
    dma_buffer_init(&dma_buffer, 1, 1);
    clear_buf((unsigned char*)dma_buffer.virt, DMA_BUFFER_SIZE);

    /* Map GPIO regs */
    map_segment(&gpio_regs, GPIO_BASE, PAGE_SIZE);

    /* Map DMA regs */
    map_segment(&dma_regs, DMA_BASE, NADJ_CHANNELS * PAGE_SIZE);
    memset(dma_regs.virt, 0, sizeof(14 * PAGE_SIZE));

    /* Map SMI regs */
    map_segment(&smi_regs, SMI_BASE, PAGE_SIZE);
    smi_gpio_init(gpio_regs);

    /* Map CLK regs */
    map_segment(&clk_regs, CLK_BASE, PAGE_SIZE);

    volatile uintptr_t* dma_cs = DMA_N_REG(dma_regs.virt, 0);

    REG_SETBIT(*dma_cs, CS_CR);
    DMA_CB* cb    = (DMA_CB*) dma_buffer.virt;

    memset(cb, 0, sizeof(DMA_CB));
    char* msg  = (char*) (cb + 1);
    char* dest = msg + 0x100;
    strcpy(msg, "This is a DMA transfer message");


    cb->ti = DMA_CB_SRC_INC | DMA_CB_DEST_INC;
    uintptr_t src_offset  = (uintptr_t)msg  - (uintptr_t)dma_buffer.virt;
    uintptr_t dest_offset = (uintptr_t)dest - (uintptr_t)dma_buffer.virt;
    cb->src_addr  = (uint32_t)((uintptr_t)dma_buffer.bus + src_offset);
    cb->dest_addr = (uint32_t)((uintptr_t)dma_buffer.bus + dest_offset);
    cb->tfr_len   = strlen(msg) + 1;

    start_dma(&dma_buffer, dma_regs.virt, 0, cb);

    sleep(0); // Enough delay for DMA transfer to complete
    uintptr_t cs = *dma_cs;
    uintptr_t debug = *dma_cs + 0x20;
    if(cs & CS_END) 
    {
        printf("dest virt: %p\n", (void*)dest);
        printf("dest content: '%s'\n", dest);
    } else
    {
        perror("DMA transfer incomplete\n");
    }
    
    printf("SMI regs virt: %p\n", smi_regs.virt);
    printf("SMI regs phys: %p\n", smi_regs.phys);

    volatile SMI_CS* smi_cs  = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);
    volatile SMI_DSR* smi_dsr = (volatile SMI_DSR*) REG32(smi_regs, SMIO_DSR0);
    volatile SMI_DSW* smi_dsw = (volatile SMI_DSW*) REG32(smi_regs, SMIO_DSW0);
    
    init_smi(smi_cs, clk_regs, smi_regs, smi_dsr, smi_dsw, 100, 1, 25, 50, 25);
    uint32_t ctl = *REG32(clk_regs, CLK_SMI_CTL);
    uint32_t div = *REG32(clk_regs, CLK_SMI_DIV);

    printf("SMI_CLK_CTL = 0x%08x\n", ctl);
    printf("SMI_CLK_DIV = 0x%08x\n", div);
    //smi_8b_init(gpio_regs);
    smi_8b_write(gpio_regs, smi_regs);
    //REG_SETBIT(*smi_cs, SMI_CS_CLEAR);
    //REG_SETBIT(*smi_cs, SMI_CS_AFERR);
    //REG_SETBIT(*smi_cs, SMI_CS_CLEAR);
    //REG_SETBIT(*smi_cs, SMI_CS_ENABLE);

    //REG_SETBIT(*smi_dcs, SMI_DCS_DONE);
    //REG_SETBIT(*smi_dcs, SMI_DCS_WRITE);
    //REG_SETBIT(*smi_dd, SMI_DD_DATA);
    //REG_SETBIT(*smi_dcs, SMI_DCS_START);
    //printf("SMI_CS_DONE: %d\n", (*smi_dcs & SMI_DCS_DONE));
    //while(!(*smi_dcs & SMI_DCS_DONE));
    //printf("CS: %d\nDD: %d\nDCS: %d\n", *smi_cs, *smi_dd, *smi_dcs);
    //*smi_dcs = 0;
    //printf("DCS: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\n", BYTE_TO_BINARY(*smi_dcs>>24), BYTE_TO_BINARY(*smi_dcs>>16), BYTE_TO_BINARY(*smi_dcs>>8), BYTE_TO_BINARY(*smi_dcs));
    //printf("SMI transfer complete\n");

    unmap_segment(dma_buffer.virt, DMA_BUFFER_SIZE);
    unmap_segment(dma_regs.virt, PAGE_SIZE);

    return 0;
}
