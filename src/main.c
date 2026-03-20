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
#include "errors.h"


int main()
{
    MEM_MAP dma_buffer, dma_regs, clk_regs, smi_regs, gpio_regs;
    SMI_CXT cxt;

    SMI_RW rw;
    SMI_CLK clk;
    SMI_READ rconfig;
    SMI_WRITE wconfig;

    rconfig.rwidth = SMI_8_BITS;
    rconfig.fsetup = 0;
    rconfig.rhold = 2;
    rconfig.rpace = 2;
    rconfig.rsetup = 2;

    wconfig.wwidth = SMI_8_BITS;
    wconfig.wformat = SMI_RGB565;
    wconfig.whold = 2;
    wconfig.wpace = 2;
    wconfig.wsetup = 2;

    smi_init_cxt_map(&cxt, &smi_regs, &clk_regs, &gpio_regs, &dma_regs);
    smi_init_rw_config(&cxt, &rw, &clk, &rconfig, &wconfig, SMI_DEVICE1, SMI_DEVICE1);
    init_smi_clk(clk_regs, smi_regs, 2);
    smi_sync_context_device(&cxt);
    smi_init_udmabuf(&cxt, &dma_buffer);

    /*
    volatile uintptr_t* dma_cs = DMA_N_REG(dma_regs.virt, 0);

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
    volatile SMI_DSR* smi_dsr0 = (volatile SMI_DSR*) REG32(smi_regs, SMIO_DSR0);
    volatile SMI_DSW* smi_dsw0 = (volatile SMI_DSW*) REG32(smi_regs, SMIO_DSW0);
    

    smi_cs->value = 0;

    //init_smi_clk(smi_cs, clk_regs, smi_regs, smi_dsr0, smi_dsw0, 30, 63, 127, 63);
    //sleep(1);
    //init_smi_clk(smi_cs, clk_regs, smi_regs, smi_dsr1, smi_dsw1, 8, 2, 2, 2);
    uint32_t ctl = *REG32(clk_regs, CLK_SMI_CTL);
    uint32_t div = *REG32(clk_regs, CLK_SMI_DIV);

    printf("SMI_CLK_CTL = 0x%08x\n", ctl);
    printf("SMI_CLK_DIV = 0x%08x\n", div);

    
    smi_8b_init(gpio_regs);
    smi_dma_setup(smi_regs);
    //smi_8b_write(smi_regs, 0x0, 1);
    //smi_dma_write(smi_regs, dma_regs, &dma_buffer, cxt.fd_sync_dev, cb, DMA_CHANNEL_0);
    //sleep(1);
    //smi_8byte_write(smi_regs, 8);
    //sram_helloworld(&cxt);
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


    megbyte_load_block_test(&cxt);

    cxt.rw_config->rconfig[1].rhold = 8;
    cxt.rw_config->rconfig[1].rpace = 8;
    cxt.rw_config->rconfig[1].rsetup = 8;
    cxt.rw_config->read_device_num = 1;

    smi_sync_context_device(&cxt);

    megbyte_load_block_test(&cxt);

    cxt.rw_config->read_device_num = 0;

    smi_sync_context_device(&cxt);

    megbyte_load_block_test(&cxt);

    smi_unmap_cxt(&cxt);
    smi_unmap_udmabuf(&cxt);

    return 0;
}