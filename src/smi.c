#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#include "smi.h"
#include "clk.h"
#include "dma.h"

/* ns: Clock period; even number 2 -> 30*/
void init_smi_clk(volatile SMI_CS* cs, MEM_MAP clk_regs, MEM_MAP smi_regs, volatile SMI_DSR* dsr, volatile SMI_DSW* dsw, int ns, int setup, int strobe, int hold)
{    
    int divi = ns/2; /* Only valid on RPI 3 */

    if(*REG32(clk_regs, CLK_SMI_DIV) != divi << 12)
    {
        *REG32(clk_regs, CLK_SMI_CTL) = CLK_PASSWD | (1 << 5);
        sleep(0);
        while (*REG32(clk_regs, CLK_SMI_CTL) & (1 << 7)) ;
        sleep(0);
        *REG32(clk_regs, CLK_SMI_DIV) = CLK_PASSWD | (divi << 12);
        sleep(0);
        *REG32(clk_regs, CLK_SMI_CTL) = CLK_PASSWD | 6 | (1 << 4);
        sleep(0);
        while ((*REG32(clk_regs, CLK_SMI_CTL) & (1 << 7)) == 0) ;
        sleep(0);
    }

    if (cs->fields.seterr)
    {
        perror("SMI SETERR true\n");
        cs->fields.seterr = 1;
    }
    
    dsr->fields.rsetup = dsw->fields.wsetup = setup; 
    dsr->fields.rstrobe = dsw->fields.wstrobe = strobe;
    dsr->fields.rhold = dsw->fields.whold = hold;
}

void smi_gpio_init(MEM_MAP gpio_map)
{
    gpio_mode(gpio_map, SMI_SOE_PIN, GPIO_ALT1);
    gpio_mode(gpio_map, SMI_SWE_PIN, GPIO_ALT1);
}

void smi_dma_setup(MEM_MAP smi_regs)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);
    volatile SMI_DC* dc = (volatile SMI_DC*) REG32(smi_regs, SMIO_DCS);

    cs->fields.clear = 1;
    cs->fields.pxldat = 1;
    cs->fields.enable = 1;

    dc->fields.dmaen = 1;

    if (cs->fields.seterr)
    {
        perror("SMI SETERR true\n");
        cs->fields.seterr = 1;
    }
}

void smi_direct_init(volatile SMI_CS* cs)
{
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;
    cs->fields.pxldat = 0;
}

void smi_8b_init(MEM_MAP gpio_map)
{
    for(int i = 0; i < 26; i++)
    {
        gpio_mode(gpio_map, i, GPIO_ALT1);
    }
}

int smi_write_byte(volatile SMI_CS* cs, volatile SMI_L* l, volatile SMI_A* a, volatile SMI_D* d, int8_t* data, int length, int8_t addr)
{
    if(cs == NULL || l == NULL || a == NULL || d == NULL || data == NULL) 
    {
        perror("ERROR: SMI write byte NULL reference passed\n");
        return -1;
    }

    cs->value = 0; /* Needed to clear any errors from previous transactions */
    cs->fields.clear = 1;
    cs->fields.aferr = 1;

    cs->fields.enable = 1;
    cs->fields.write = 1;
    cs->fields.clear = 1;

    l->value = length;
    cs->fields.start = 1;

    for(int i = 0; i < length; i++)
    {
        a->value = addr + i;
        d->value = data[i];
        cs->fields.write = 1;
    }

    while(!cs->fields.done);
    cs->fields.done = 1;

    return 1;
}

void smi_dma_write(MEM_MAP smi_regs, MEM_MAP dma_regs, MEM_MAP* dma_buffer, DMA_CB* cb, uint8_t channel)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);    
    cb->dest_addr = (uint32_t) REG32_BUS(smi_regs, SMIO_D);
    printf("Dest address: %p\n", cb->dest_addr);
    printf("SMI virt address: %p\n", smi_regs.virt);
    printf("SMI phys address: %p\n", smi_regs.phys);
    printf("SMI bus address: %p\n", smi_regs.bus);

    cb->ti = DMA_CB_DEST_DREQ | (DMA_SMI_DREQ << 16) | DMA_CB_SRC_INC;

    cs->fields.clear = 1;
    cs->fields.enable = 1;
    cs->fields.write = 1;
    
    //start_dma(dma_buffer, dma_regs, channel, cb);

    cs->fields.start = 1;
}

void smi_8byte_write(MEM_MAP smi_regs, uint8_t addr)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32(smi_regs, SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32(smi_regs, SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32(smi_regs, SMIO_D);

    int8_t data[] = {0x0, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF};

    smi_write_byte(cs, l, a, d, data, sizeof(data), addr);
}

/* 8 bit data bus example */
void smi_8b_write(MEM_MAP smi_regs, uint8_t data, uint8_t addr)
{
    /*
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32(smi_regs, SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32(smi_regs, SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32(smi_regs, SMIO_D);
    
    cs->value = 0;
    
    cs->fields.clear = 1;
    cs->fields.aferr = 1;

    cs->fields.enable = 1;
    cs->fields.write = 1;
    cs->fields.clear = 1;

    l->value = 1;
    a->value = addr;
    d->value = data;

    cs->fields.start = 1;

    while(!cs->fields.done);
    cs->fields.done = 1;
    */
    
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32(smi_regs, SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32(smi_regs, SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32(smi_regs, SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32(smi_regs, SMIO_DD);

    cs->value = 0;
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;

    dcs->fields.done = 1;

    dcs->fields.write = 1;

    da->fields.addr = addr;
    dd->fields.data = data;

    dcs->fields.start = 1;

    while (!dcs->fields.done);

    dcs->fields.done = 1;
}

int smi_programmed_read(MEM_MAP smi_regs, uint8_t addr, uint8_t len)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32(smi_regs, SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32(smi_regs, SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32(smi_regs, SMIO_D);
    
    int8_t* data[] = {0};

    cs->value = 0;
    
    cs->fields.clear = 1;
    cs->fields.aferr = 1;

    cs->fields.enable = 1;
    cs->fields.write = 0;
    cs->fields.clear = 1;

    l->value = len;


    for(int i = 0; i < len; i++)
    {
        
    }

}

int smi_8b_read(MEM_MAP smi_regs, uint8_t addr)
{
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32(smi_regs, SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32(smi_regs, SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32(smi_regs, SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32(smi_regs, SMIO_DD);

    cs->value = 0;
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;
    cs->fields.pxldat = 0;
    cs->fields.pad = 0;

    dcs->fields.done = 1;

    dcs->fields.write = 0;

    da->fields.addr = addr;
    dd->value = 0; // flush stale data

    dcs->fields.start = 1;

    while (!dcs->fields.done);

    int val = dd->fields.data;

    dcs->fields.done = 1;

    return val;
}
