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
#include "sram.h"


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
    smi_regs.bus = smi_regs.phys - 0x3F000000 + 0x7E000000;
    smi_gpio_init(gpio_regs);

    /* Map CLK regs */
    map_segment(&clk_regs, CLK_BASE, PAGE_SIZE);

    
    volatile uintptr_t* dma_cs = DMA_N_REG(dma_regs.virt, 0);

    /*
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
    
    */

    SMI_LAYOUT smi_layout[4] = {
        [WIDTH_8]  = { 8,  0xFF,      { 0,  8  } },
        [WIDTH_16] = { 16, 0xFFFF,    { 0, 16 } },
        [WIDTH_18] = { 18, 0x3FFFF,   { 0, 18 } },
        [WIDTH_9]  = { 9,  0x1FF,     { 0, 9  } },
    };
    
    DMA_CB* cb = (DMA_CB*)dma_buffer.virt;
    memset(cb, 0, sizeof(DMA_CB));
    
    char* msg = (char*) (cb + 1);
    strcpy(msg, "Test");
    
    /*
    uint8_t* msg = (uint8_t*) (cb + 1);
    *msg = 0xA;
    */
    uintptr_t src_offset = (uintptr_t) msg - (uintptr_t)dma_buffer.virt;
    cb->src_addr = (uint32_t)((uintptr_t)dma_buffer.bus + src_offset);
    cb->tfr_len = strlen(msg);

    volatile SMI_CS* smi_cs  = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);
    volatile SMI_DSR* smi_dsr = (volatile SMI_DSR*) REG32(smi_regs, SMIO_DSR0);
    volatile SMI_DSW* smi_dsw = (volatile SMI_DSW*) REG32(smi_regs, SMIO_DSW0);
    

    smi_cs->value = 0;

    init_smi_clk(smi_cs, clk_regs, smi_regs, smi_dsr, smi_dsw, 30, 63, 127, 63);
    //init_smi_clk(smi_cs, clk_regs, smi_regs, smi_dsr, smi_dsw, 4, 3, 6, 3);
    //init_smi_clk(smi_cs, clk_regs, smi_regs, smi_dsr, smi_dsw, 10, 2, 6, 2);
    uint32_t ctl = *REG32(clk_regs, CLK_SMI_CTL);
    uint32_t div = *REG32(clk_regs, CLK_SMI_DIV);

    printf("SMI_CLK_CTL = 0x%08x\n", ctl);
    printf("SMI_CLK_DIV = 0x%08x\n", div);

    
    SMI_CXT cxt;
    cxt.smi_regs = &smi_regs;
    cxt.dma_regs = &dma_regs;
    cxt.dma_buffer = &dma_buffer;
    cxt.fd_sync_dev = fd_sync_dev;

    smi_8b_init(gpio_regs);
    //smi_dma_setup(smi_regs);
    //smi_8b_write(smi_regs, 0x0, 1);
    //smi_dma_write(smi_regs, dma_regs, &dma_buffer, fd_sync_dev, cb, DMA_CHANNEL_0);
    //sleep(1);
    //smi_8byte_write(smi_regs, 8);
    sram_helloworld(&cxt);
    //sram_block_byte_write(smi_regs);
    int data_len = 12;
    uint32_t data32[data_len];
    uint8_t data[data_len];
    int len_read = 0;

    //smi_programmed_write_dma(&cxt, cb, 0);

    //len_read = smi_programmed_read_old(smi_regs, 1, data, data_len);
    //len_read = smi_programmed_read(&cxt, &data32, 0);
    uint32_t val;
    //smi_direct_read_arr(&cxt, data32, 0, 12, SMI_ADDR_INC);
    //uint8_t val2 = smi_8b_read(smi_regs, 0);
    //printf("New read: %d \n", len_read);
    //len_read = smi_programmed_read_old(smi_regs, 1, data, data_len);
    //len_read = smi_programmed_read_arr(&cxt, data32, 0, data_len);
    //printf("Old read: %d \n", len_read);
    
    //printf("Data Direct     %d = %c ; %d\n", 0, val, val);

    //printf("Data Direct Old %d = %c ; %d\n", 0, val2, val2);
    //printf("Data Programmed %d = %c ; %d\n", 0, data32, data32);

    //int read = testbench_write(&cxt, 1000000);
    //int read = testbench_read(&cxt, 1000000);
    //printf("Data read: %d\n", read);

    unmap_segment(dma_buffer.virt, DMA_BUFFER_SIZE);
    unmap_segment(dma_regs.virt, PAGE_SIZE);

    return 0;
}
