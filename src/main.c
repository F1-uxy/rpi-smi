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
#include "smi.h"
#include "clk.h"


int main()
{
    MEM_MAP dma_buffer, dma_regs, clk_regs, smi_regs, gpio_regs;

    int fd_sync_cpu = open("/sys/class/u-dma-buf/udmabuf0/sync_for_cpu", O_WRONLY);
    int fd_sync_dev = open("/sys/class/u-dma-buf/udmabuf0/sync_for_device", O_WRONLY);
    

    /* Map u-dma buffer */
    dma_buffer_init(&dma_buffer, 1, 1);
    //clear_buf((unsigned char*)dma_buffer.virt, DMA_BUFFER_SIZE);

    /* Map GPIO regs */
    map_segment(&gpio_regs, GPIO_BASE, PAGE_SIZE);

    /* Map DMA regs */
    map_segment(&dma_regs, DMA_BASE, NADJ_CHANNELS * PAGE_SIZE);
    //memset(dma_regs.virt, 0, 14 * PAGE_SIZE);

    /* Map SMI regs */
    map_segment(&smi_regs, SMI_BASE, PAGE_SIZE);
    smi_regs.bus = smi_regs.phys + 0xC0000000;
    smi_gpio_init(gpio_regs);

    /* Map CLK regs */
    map_segment(&clk_regs, CLK_BASE, PAGE_SIZE);

    
    volatile uintptr_t* dma_cs = DMA_N_REG(dma_regs.virt, 0);

    //REG_SETBIT(*dma_cs, CS_CR);
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

    start_dma(&dma_buffer, dma_regs, fd_sync_dev, 0, cb);

    sleep(0); // Enough delay for DMA transfer to complete
    //write(fd_sync_cpu, "1", 1);
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
    

    /*
    DMA_CB* cb = (DMA_CB*)dma_buffer.virt;
    memset(cb, 0, sizeof(DMA_CB));
    uint8_t* msg = (char*) (cb + 1);
    *msg = 0xF;

    uintptr_t src_offset = (uintptr_t) msg - (uintptr_t)dma_buffer.virt;
    cb->src_addr = (uint32_t)((uintptr_t)dma_buffer.bus + src_offset);
    cb->tfr_len = 1;*/

    volatile SMI_CS* smi_cs  = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);
    volatile SMI_DSR* smi_dsr = (volatile SMI_DSR*) REG32(smi_regs, SMIO_DSR0);
    volatile SMI_DSW* smi_dsw = (volatile SMI_DSW*) REG32(smi_regs, SMIO_DSW0);
    
    init_smi_clk(smi_cs, clk_regs, smi_regs, smi_dsr, smi_dsw, 30, 25, 50, 25);
    uint32_t ctl = *REG32(clk_regs, CLK_SMI_CTL);
    uint32_t div = *REG32(clk_regs, CLK_SMI_DIV);

    printf("SMI_CLK_CTL = 0x%08x\n", ctl);
    printf("SMI_CLK_DIV = 0x%08x\n", div);
    
    smi_8b_init(gpio_regs);
    //smi_dma_setup(smi_regs);
    //smi_dma_write(smi_regs, dma_regs, &dma_buffer, cb, DMA_CHANNEL_0);
    smi_8b_write(smi_regs, 0x0, 0xF);

    sleep(1);
    smi_8b_write(smi_regs, 0xF, 0x0);

    //smi_8byte_write(smi_regs, 0xF);
    sleep(1);
    int val = smi_8b_read(smi_regs, 0xF);
    printf("Address: %p ; Value: %d\n", 0xF, val);

    sleep(1);

    val = smi_8b_read(smi_regs, 0x0);
    printf("Address: %p ; Value: %d\n", 0x0, val);
    
    unmap_segment(dma_buffer.virt, DMA_BUFFER_SIZE);
    unmap_segment(dma_regs.virt, PAGE_SIZE);

    return 0;
}
