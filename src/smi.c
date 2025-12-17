#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#include "smi.h"
#include "clk.h"

void init_smi(volatile SMI_CS* cs, MEM_MAP clk_regs, MEM_MAP smi_regs, volatile SMI_DSR* dsr, volatile SMI_DSW* dsw, int width, int ns, int setup, int strobe, int hold)
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
        perror("SMI SETERR set\n");
        cs->fields.seterr = 1;
    }
    dsr->fields.rsetup = dsw->fields.wsetup = setup; 
    dsr->fields.rstrobe = dsw->fields.wstrobe = strobe;
    dsr->fields.rhold = dsw->fields.whold = hold;
    dsr->fields.rwidth = dsw->fields.wwidth = width;
}

void smi_gpio_init(MEM_MAP gpio_map)
{
    gpio_mode(gpio_map, SMI_SOE_PIN, GPIO_ALT1);
    gpio_mode(gpio_map, SMI_SWE_PIN, GPIO_ALT1);

}

void smi_cs_init(volatile SMI_CS* cs)
{
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;
}

void smi_8b_init(MEM_MAP gpio_map)
{
    for(int i = 8; i < 16; i++)
    {
        gpio_mode(gpio_map, i, GPIO_ALT1);
    }
}

/* 8 bit data bus example */
void smi_8b_write(MEM_MAP gpio_map, MEM_MAP smi_regs)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32(smi_regs, SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32(smi_regs, SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32(smi_regs, SMIO_D);

    cs->value = 0; /* Needed to clear any errors from previous transactions */

    cs->fields.enable = 1;
    cs->fields.write = 1;
    cs->fields.clear = 1;

    l->value = 1;
    a->value = 0x15;
    d->value = 0x10;

    cs->fields.start = 1;

    while(!cs->fields.done);
    cs->fields.done = 1;
}