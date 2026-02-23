#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


#include "smi.h"
#include "clk.h"
#include "dma.h"
#include "errors.h"
#include "timeout.h"

SMI_LAYOUT smi_layout[4] = {
    [WIDTH_8]  = { 8,  0xFF,      { 0,  8 } },
    [WIDTH_16] = { 16, 0xFFFF,    { 0, 16 } },
    [WIDTH_18] = { 18, 0x3FFFF,   { 0, 18 } },
    [WIDTH_9]  = { 9,  0x1FF,     { 0, 9  } },7
};

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

int smi_programmed_write_old(volatile SMI_CS* cs, volatile SMI_L* l, volatile SMI_A* a, volatile SMI_D* d, int8_t* data, int length, int8_t addr)
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

    int count = 0;
    while(count < length)
    {
        if(!cs->fields.txe)
        {
            a->fields.addr = addr + count;
            d->value = data[count];
            count++;
            sleep(0);
        }
    }

    while(!cs->fields.done)
    {
    }

    cs->fields.done = 1;

    return count;
}

void smi_dma_write(MEM_MAP smi_regs, MEM_MAP dma_regs, MEM_MAP* dma_buffer, int fd_sync_dev, DMA_CB* cb, uint8_t channel)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);
    volatile SMI_L*  l  = (volatile SMI_L*) REG32(smi_regs, SMIO_L);  
    volatile SMI_A*  a  = (volatile SMI_A*) REG32(smi_regs, SMIO_A);  
    volatile SMI_D*  d  = (volatile SMI_D*) REG32(smi_regs, SMIO_D);  
    volatile SMI_DC* dc = (volatile SMI_DC*) REG32(smi_regs, SMIO_DC);

    volatile DMA_CS* dma_cs = (volatile DMA_CS*) (REG32(dma_regs, DMAO_CS) + DMA_CHANNEL_0);
    volatile DMA_DEBUG* dma_debug = (volatile DMA_DEBUG*) (REG32(dma_regs, DMAO_DEBUG) + DMA_CHANNEL_0);
    volatile uint32_t* dma_dest_addr = (REG32(dma_regs, DMA0_DEST_AD) + DMA_CHANNEL_0);
    
    cs->value = 0;
    cs->fields.clear = 1;
    while (cs->fields.clear);
    a->fields.addr = 0;
    l->value = cb->tfr_len;
    
    dc->fields.dmaen = 1;
    //dc->fields.dmap = 1; /* Top 2 bits are used for external data requests */

    cs->fields.pxldat = 1;
    cs->fields.enable = 1;
    cs->fields.write = 1;


    cb->dest_addr = REG32_BUS(smi_regs, SMIO_D);
    cb->ti = DMA_DEST_DREQ | (DMA_SMI_DREQ << 16) | DMA_CB_SRCE_INC;
    
    cs->fields.start = 1;
    start_dma(dma_buffer, dma_regs, fd_sync_dev, 0, cb);

    while(!cs->fields.done);
}

void smi_8byte_write(MEM_MAP smi_regs, uint8_t addr, uint8_t* data, int len)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32(smi_regs, SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32(smi_regs, SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32(smi_regs, SMIO_D);

    smi_programmed_write_old(cs, l, a, d, data, len, addr);
}

int smi_8b_direct_write(MEM_MAP smi_regs, uint8_t data, uint8_t addr)
{
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

    while (!dcs->fields.done)
    {
    }

    dcs->fields.done = 1;
}

/* 8 bit data bus example */
void smi_8b_write(MEM_MAP smi_regs, uint8_t data, uint8_t addr)
{
    
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
    a->fields.addr = addr;
    d->value = data;

    cs->fields.start = 1;

    while(!cs->fields.done);
    cs->fields.done = 1;
    
    
}

int smi_programmed_read_old(MEM_MAP smi_regs, uint8_t addr, uint8_t* ret_data, uint8_t len)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32(smi_regs, SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32(smi_regs, SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32(smi_regs, SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32(smi_regs, SMIO_D);
    volatile SMI_FD*  fd = (volatile SMI_FD*) REG32(smi_regs, SMIO_FD);

    int count = 0;
    
    cs->value = 0;
    
    cs->fields.aferr = 1;
    cs->fields.seterr = 1;

    cs->fields.pxldat = 1;

    cs->fields.enable = 1;
    cs->fields.write = 0;
    cs->fields.clear = 1;

    l->value = len;
    a->fields.addr = addr;

    cs->fields.start = 1;
    
    while ((!cs->fields.done || cs->fields.rxd > 1) && (count < len)) {

        if(cs->fields.seterr) 
        {
            ERROR("A, L or D register written to during programmed read");
            cs->fields.seterr = 1;
            return -EIO;
        }

        if (cs->fields.rxd) {
            uint32_t word = d->value;
            ret_data[count++] = (word >>  0) & 0xFF;
            ret_data[count++] = (word >>  8) & 0xFF;
            ret_data[count++] = (word >> 16) & 0xFF;
            ret_data[count++] = (word >> 24) & 0xFF;
        }
    }

    cs->fields.clear = 1;

    return count;
}

int smi_8b_read(MEM_MAP smi_regs, uint8_t addr)
{
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32(smi_regs, SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32(smi_regs, SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32(smi_regs, SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32(smi_regs, SMIO_DD);
    volatile SMI_DSR* dsr = (volatile SMI_DSR*) REG32(smi_regs, SMIO_DSR0);

    cs->value = 0;
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;
    cs->fields.pxldat = 0;
    cs->fields.pad = 0;

    dsr->fields.rwidth = 0; /* 8bit read width */

    dcs->fields.done = 1;

    dcs->fields.write = 0;

    da->fields.addr = addr;
    dd->value = 0; /* flush stale data */

    dcs->fields.start = 1;

    while (!dcs->fields.done);

    uint8_t val = (dd->fields.data & 0xFF);

    dcs->fields.done = 1;

    return val;
}

void smi_start(SMI_CXT* cxt)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32((*cxt->smi_regs), SMIO_CS);
    cs->fields.start = 1;
}

int smi_read_await(SMI_CXT* cxt, uint32_t* ret_data, int len)
{
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_A*  a  = (volatile SMI_A*)  REG32((*cxt->smi_regs), SMIO_A);

    volatile SMI_D*  d = (volatile SMI_D*) REG32((*cxt->smi_regs), SMIO_D);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);

    if(ret_data == NULL) return -EINVAL;

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    deadline = start_timeout(PROG_READ_TIMEOUT_S);

    while ((!cs->fields.done || cs->fields.rxd > 1)) 
    {
        if(spin > 1024)
        {
            if(timeout_complete(deadline))
            {
                ERROR("Programmed read timeout");
                cs->fields.clear = 1;
                return -ETIMEDOUT;
            }
        }

        spin++;
    }

    deadline = start_timeout(PROG_READ_TIMEOUT_S);
    while((count < len))
    {
        if (cs->fields.rxd) {
            volatile uint32_t word = d->value;
            printf("Word: %u\n", word);
            ret_data[count++] = word;
        }

        if(spin > 1024)
        {
            if(timeout_complete(deadline))
            {
                ERROR("Programmed read data fetch timeout");
                cs->fields.clear = 1;
                return -ETIMEDOUT;
            }
        }

        spin++;
    }

    cs->fields.done = 1;

    return count;
}

int smi_read_await_direct(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len, int increment)
{
    if(ret_data == NULL || cxt == NULL) return -1;

    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32((*cxt->smi_regs), SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32((*cxt->smi_regs), SMIO_DD);

    int count = 0;
    int spin = 0;

    dcs->fields.write = 0;
    da->fields.addr = addr;

    smi_timeout_ns deadline;
    deadline = start_timeout(DIRECT_READ_TIMEOUT_S);

    while(count < len)
    {
        dcs->fields.start = 1;
        
        while (!dcs->fields.done)
        {
            if(spin > 1024)
            {
                if(timeout_complete(deadline))
                {
                    ERROR("Direct read timeout");
                    cs->fields.clear = 1;
                    dcs->fields.done = 1;
                    return -ETIMEDOUT;
                }
                spin = 0;
            }
            spin++;
        }

        uint32_t val = dd->value & 0xFF;
        ret_data[count++] = val;
        
        dcs->fields.done = 1;

        if(increment > 0) {addr++; da->fields.addr = addr;}
        else if (increment < 0) {addr--; da->fields.addr = addr;};
    }

    return count;
}

int smi_direct_read(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr)
{
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32((*cxt->smi_regs), SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32((*cxt->smi_regs), SMIO_DD);
    volatile SMI_DSR* dsr = (volatile SMI_DSR*) REG32((*cxt->smi_regs), SMIO_DSR0);

    uint32_t raw_data[1];

    cs->value = 0;
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;
    cs->fields.pxldat = 0;
    cs->fields.pad = 0;

    dsr->fields.rwidth = 0; /* 8bit read width */

    dcs->fields.done = 1;

    dcs->fields.write = 0;

    da->fields.addr = addr;
    dd->value = 0; /* flush stale data */

    dcs->fields.start = 1;
    
    int count = smi_read_await_direct(cxt, ret_data, addr, 1, 0);

    //smi_unpack(cxt, raw_data, ret_data, count);

    return 1;
}

int smi_direct_read_arr(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len, int increment)
{
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32((*cxt->smi_regs), SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32((*cxt->smi_regs), SMIO_DD);
    volatile SMI_DSR* dsr = (volatile SMI_DSR*) REG32((*cxt->smi_regs), SMIO_DSR0);

    //uint32_t raw_data[len];

    cs->value = 0;
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;
    cs->fields.pxldat = 0;
    cs->fields.pad = 0;

    dsr->fields.rwidth = 0; /* 8bit read width */

    dcs->fields.done = 1;

    dcs->fields.write = 0;

    da->fields.addr = addr;
    dd->value = 0; /* flush stale data */

    dcs->fields.start = 1;
    int count = smi_read_await_direct(cxt, ret_data, addr, len, increment);
    
    //smi_pack_ratio_t t = smi_packed_ratio(cxt);
    //smi_unpack(cxt, raw_data, ret_data, count, t);
    
    return count;
}

int smi_programmed_read(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr)
{
    return smi_programmed_read_arr(cxt, ret_data, addr, 1);
}

int smi_programmed_read_arr(SMI_CXT* cxt, void* ret_data, uint8_t addr, int len)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32((*cxt->smi_regs), SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32((*cxt->smi_regs), SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32((*cxt->smi_regs), SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32((*cxt->smi_regs), SMIO_D);
    volatile SMI_DSR* dsr = (volatile SMI_DSR*) REG32((*cxt->smi_regs), SMIO_DSR0);
    volatile SMI_DSW* dsw = (volatile SMI_DSW*) REG32((*cxt->smi_regs), SMIO_DSW0);


    int count = 0;
    smi_pack_ratio_t ratio = smi_packed_ratio(cxt);
    int word_reads = SMI_DIV((len * ratio.read), ratio.out_pixels);
    printf("Word Reads: %d\n", word_reads);
    int raw_data[word_reads];

    dsr->fields.rwidth = cxt->rw_config->rconfig->rwidth;
    dsw->fields.wformat = cxt->rw_config->wconfig->wformat;
    dsw->fields.wswap = cxt->rw_config->wconfig->wswap;


    cs->value = 0;
    
    cs->fields.aferr = 1;
    cs->fields.seterr = 1;

    cs->fields.pxldat = 1;

    cs->fields.enable = 1;
    cs->fields.write = 0;
    cs->fields.clear = 1;

    l->fields.length = len;
    a->fields.addr = addr;
    printf("Address: %d\n", a->fields.addr);

    smi_start(cxt);
    count = smi_read_await(cxt, raw_data, word_reads);
    smi_unpack(cxt, raw_data, ret_data, len, ratio);

    return len;

}

int smi_write_await(SMI_CXT* cxt, uint32_t* data, uint8_t addr, int len)
{
    if(data == NULL || cxt == NULL) return -1;

    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_A*  a  = (volatile SMI_A*)  REG32((*cxt->smi_regs), SMIO_A);
    volatile SMI_D*  d  = (volatile SMI_D*)  REG32((*cxt->smi_regs), SMIO_D);

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    deadline = start_timeout(PROG_WRITE_TIMEOUT_S);

    a->fields.addr = addr;
    cs->fields.write = 1;

    while(count < len)
    {
        cs->fields.start = 1;
        d->value = data[count++];

        while(!cs->fields.done)
        {
            if(spin > 1024)
            {
                if(timeout_complete(deadline))
                {
                    ERROR("Programmed write await timeout");
                    cs->fields.clear = 1;
                    cs->fields.done = 1;
                    return -ETIMEDOUT;
                }
            }
        }

        if (cs->fields.aferr)
        {
            ERROR("Protected registers written to during write");
            cs->fields.aferr = 1;
            cs->fields.clear = 1;
            return count;
        }

        spin++;
    }

    return count;
}

int smi_write_await_direct(SMI_CXT* cxt, uint32_t* data, uint8_t addr, int len, int increment)
{
    if(data == NULL || cxt == NULL) return -1;

    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32((*cxt->smi_regs), SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32((*cxt->smi_regs), SMIO_DD);

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    deadline = start_timeout(PROG_WRITE_TIMEOUT_S);

    dcs->fields.write = 1;

    while(count < len)
    {
        dd->value = data[count++];
        dcs->fields.start = 1;
        
        while(!dcs->fields.done)
        {
            if(spin > 1024)
            {
                if(timeout_complete(deadline))
                {
                    ERROR("Direct read array timeout");
                    cs->fields.clear = 1;
                    dcs->fields.done = 1;
                    return -ETIMEDOUT;
                }
            }
        }

        if(increment > 0) {addr++; da->fields.addr = addr;}
        else if (increment < 0) {addr--; da->fields.addr = addr;};

        spin++;
    }

    return count;
}

int smi_direct_write(SMI_CXT* cxt, uint32_t data, uint8_t addr)
{
    return smi_direct_write_arr(cxt, &data, addr, 1, 0);
}

int smi_direct_write_arr(SMI_CXT* cxt, uint32_t* data, uint8_t addr, uint8_t len, int increment)
{
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32((*cxt->smi_regs), SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32((*cxt->smi_regs), SMIO_DD);
    volatile SMI_DSR* dsr = (volatile SMI_DSR*) REG32((*cxt->smi_regs), SMIO_DSR0);

    cs->value = 0;
    cs->fields.clear = 1;
    cs->fields.aferr = 1;
    cs->fields.enable = 1;
    cs->fields.pxldat = 0;
    cs->fields.pad = 0;

    dsr->fields.rwidth = 0; /* 8bit read width */

    dcs->fields.done = 1;

    dcs->fields.write = 1;

    da->fields.addr = addr;
    dd->value = 0; /* flush stale data */

    dcs->fields.start = 1;
    
    smi_write_await_direct(cxt, data, addr, len, increment);

    return 1;
}

int smi_programmed_write(SMI_CXT* cxt, uint32_t data, uint8_t addr)
{
    return smi_programmed_write_arr(cxt, &data, addr, 1);
}

int smi_programmed_write_arr(SMI_CXT* cxt, uint32_t* data, uint8_t addr, int len)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32((*cxt->smi_regs), SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32((*cxt->smi_regs), SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32((*cxt->smi_regs), SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32((*cxt->smi_regs), SMIO_D);

    int count = 0;
    
    cs->value = 0;
    
    cs->fields.aferr = 1;
    cs->fields.seterr = 1;

    cs->fields.pxldat = 1;

    cs->fields.enable = 1;
    cs->fields.write = 1;
    cs->fields.clear = 1;

    l->fields.length = len;
    a->fields.addr = addr;

    smi_start(cxt);

    count = smi_write_await(cxt, data, addr, len);

    return count;

}

int smi_dma_write_await(SMI_CXT* cxt, int channel)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32((*cxt->smi_regs), SMIO_CS);    
    volatile SMI_DC* dc = (volatile SMI_DC*) REG32((*cxt->smi_regs), SMIO_DC);
    
    uintptr_t offset = DMA_CS_OFFSET(channel);
    volatile DMA_CS* dcs = (volatile DMA_CS*) REG32((*cxt->dma_regs), offset);

    int spin = 0;

    smi_timeout_ns deadline;
    deadline = start_timeout(DMA_WRITE_TIMEOUT_S);

    while(!cs->fields.done)
    {
        if(spin > 1024)
        {
            if(timeout_complete(deadline))
            {
                ERROR("DMA transfer timeout");
                cs->fields.clear = 1;
                cs->fields.done = 1;
                return -ETIMEDOUT;
            }

            spin = 0;
        }

        spin++;
    }

    return 1; /* Should find a way count transfers - FIFO Debug register */
}

int smi_programmed_write_dma(SMI_CXT* cxt, DMA_CB* cb, uint8_t addr)
{
    if(cxt == NULL || cb == NULL) return -1;

    if(cxt->smi_regs == NULL) return -1;

    volatile SMI_CS* cs = (volatile SMI_CS*) REG32((*cxt->smi_regs), SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32((*cxt->smi_regs), SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32((*cxt->smi_regs), SMIO_A);
    volatile SMI_D*  d = (volatile SMI_D*) REG32((*cxt->smi_regs), SMIO_D);
    volatile SMI_DC* dc = (volatile SMI_DC*) REG32((*cxt->smi_regs), SMIO_DC);

    MEM_MAP smi_regs = *(cxt->smi_regs);
    MEM_MAP dma_regs = *(cxt->dma_regs);
    MEM_MAP* dma_buffer = cxt->dma_buffer;
    int channel = 0;

    cb->dest_addr = REG32_BUS(smi_regs, SMIO_D);
    cb->ti = DMA_DEST_DREQ | (DMA_SMI_DREQ << 16) | DMA_CB_SRCE_INC;

    cs->fields.clear = 1;
    a->fields.addr = addr;
    l->value = cb->tfr_len;

    dc->fields.dmaen = 1;
    cs->fields.pxldat = 1;
    cs->fields.enable = 1;
    cs->fields.write = 1;

    dc->fields.panicw = cxt->dma_config.panicw;
    dc->fields.reqw   = cxt->dma_config.reqw;

    dc->fields.panicw = 1;


    dc->fields.reqw = 1;

    cs->fields.start = 1;

    start_dma(dma_buffer, dma_regs, cxt->fd_sync_dev, channel, cb);
    return smi_dma_write_await(cxt, 0);
}

void smi_unpack_rgb565_8(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint8_t* dst = out;
    size_t full_words = count / 4;
    size_t tail_bytes = count % 4;

    for(size_t i = 0; i < full_words; i++)
    {
        uint32_t word = raw[i];

        uint8_t b1 = (word >>  0) & 0xFF;
        uint8_t b0 = (word >>  8) & 0xFF;
        uint8_t b3 = (word >> 16) & 0xFF;
        uint8_t b2 = (word >> 24) & 0xFF;

        dst[0] = b0;
        dst[1] = b1;
        dst[2] = b2;
        dst[3] = b3;

        dst += 4;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];

        uint8_t bytes[4] = {
            (word >>  8) & 0xFF,
            (word >>  0) & 0xFF,
            (word >> 24) & 0xFF,
            (word >> 16) & 0xFF
        };

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = bytes[i];
        }
    }
}

void smi_unpack_xrgb_8(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint8_t* dst = out;

    size_t full_words = count / 3;
    size_t tail_bytes = count % 3;

    for(size_t i = 0; i < full_words; i++)
    {
        uint32_t word = raw[i];

        uint8_t b2 = (word >>  0) & 0xFF;
        uint8_t b1 = (word >>  8) & 0xFF;
        uint8_t b0 = (word >> 16) & 0xFF;

        dst[0] = b0;
        dst[1] = b1;
        dst[2] = b2;

        dst += 3;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];

        uint8_t bytes[3] = {
            (word >> 16) & 0xFF,
            (word >>  8) & 0xFF,
            (word >>  0) & 0xFF
        };

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = bytes[i];
        }
    }
}

/* 
    Data1 = { FIFO[12:10], FIFO[7:2] } 
    Data2 = { FIFO[23:18], FIFO[15:13] }
*/
void smi_unpack_xrgb_9(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;
    size_t full_words = count / ratio.out_pixels;
    size_t tail_bytes = count % ratio.out_pixels;

    for(size_t i = 0; i < full_words; i++)
    {
        uint32_t word = raw[i];
        printf("Word: %d\n", raw[0]);

        uint16_t d0 = ((word >> 13) & 0x7) | ((word >> 15) & 0x1F8);
        uint16_t d1 = ((word >> 2) & 0x3F) | ((word >> 4) & 0x1C0);
        dst[0] = d0;
        dst[1] = d1;
        dst += 2;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];

        uint16_t bytes[2] = {
            ((word >> 13) & 0x7) | ((word >> 15) & 0x1F8),
            ((word >> 2) & 0x3F) | ((word >> 4) & 0x1C0)
        };

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = bytes[i];
        }
    }
}

/* 
    Data1 = { FIFO[23:18], FIFO[15:13] }
    Data2 = { FIFO[12:10], FIFO[7:2] } 
*/
void smi_unpack_xrgb_9_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;
    size_t full_words = count / 2;
    size_t tail_bytes = count % 2;

    printf("Swap Full words: %d ; Tail bytes: %d\n", full_words, tail_bytes);

    for(size_t i = 0; i < full_words; i++)
    {
        uint32_t word = raw[i];

        uint16_t d0 = ((word >> 2) & 0x3F) | ((word >> 4) & 0x1C0);
        uint16_t d1 = ((word >> 13) & 0x7) | ((word >> 15) & 0x1F8);
        dst[0] = d0;
        dst[1] = d1;
        dst += 2;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];

        uint16_t bytes[2] = {
            ((word >> 2) & 0x3F) | ((word >> 4) & 0x1C0),
            ((word >> 13) & 0x7) | ((word >> 15) & 0x1F8)
        };

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = bytes[i];
        }
    }
}

/* 
    RGB565 includes repeated bits
    Data0 = { FIFO[15:11], FIFO[15], FIFO[10:8] }
    Data1 = { FIFO[7:0], FIFO[4] }
    Data2 = { FIFO[31:27], FIFO[31], FIFO[26:24] }
    Data3 = { FIFO[23:16], FIFO[20] }
*/
static inline void smi_unpack_rgb565_9_word(uint32_t word, uint16_t out[4])
{
    uint8_t b0 = (word >>  0) & 0xFF;
    uint8_t b1 = (word >>  8) & 0xFF;
    uint8_t b2 = (word >> 16) & 0xFF;
    uint8_t b3 = (word >> 24) & 0xFF;
    
    uint16_t d1 = (b0 << 1) | ((b0 >> 7) & 0x1);
    uint16_t d3 = (b2 << 1) | ((b2 >> 7) & 0x1);

    uint16_t d0 = ((b1 << 1) & 0x1F0) | ((b1 >> 2) & 0x8) | ((b1) & 0x7);
    uint16_t d2 = ((b3 << 1) & 0x1F0) | ((b3 >> 2) & 0x8) | ((b3) & 0x7);

    out[0] = d0;
    out[1] = d1;
    out[2] = d2;
    out[3] = d3;
}

void smi_unpack_rgb565_9(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;
    size_t full_words = count / 4;
    size_t tail_bytes = count % 4;
    uint16_t word_buffer[4];

    for(size_t i = 0; i < full_words; i++)
    {
        uint32_t word = raw[i];
        smi_unpack_rgb565_9_word(word, word_buffer);

        printf("%u ; %u ; %u ; %u\n", word_buffer[0], word_buffer[1], word_buffer[2], word_buffer[3]);

        dst[0] = word_buffer[0];
        dst[1] = word_buffer[1];
        dst[2] = word_buffer[2];
        dst[3] = word_buffer[3];

        dst += 4;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];
        smi_unpack_rgb565_9_word(word, word_buffer);

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = word_buffer[i];
        }
    }
}

/* 
    RGB565 includes repeated bits
    Data1 = { FIFO[15:11], FIFO[15], FIFO[10:8] }
    Data2 = { FIFO[7:0], FIFO[4] }
    Data3 = { FIFO[15:11], FIFO[15], FIFO[10:8] } 
    Data4 = { FIFO[23:16], FIFO[20] }
*/
static inline void apply_swap_16(uint16_t w[4])
{
    uint16_t t;

    t = w[0]; w[0] = w[1]; w[1] = t;
    t = w[2]; w[2] = w[3]; w[3] = t;
}

void smi_unpack_rgb565_9_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;
    size_t full_words = count / 4;
    size_t tail_bytes = count % 4;
    uint16_t word_buffer[4];

    for(size_t i = 0; i < full_words; i++)
    {
        uint32_t word = raw[i];

        smi_unpack_rgb565_9_word(word, word_buffer);
        printf("%u ; %u ; %u ; %u\n", word_buffer[0], word_buffer[1], word_buffer[2], word_buffer[3]);

        dst[0] = word_buffer[1];
        dst[1] = word_buffer[0];
        dst[2] = word_buffer[3];
        dst[3] = word_buffer[2];

        dst += 4;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];
        smi_unpack_rgb565_9_word(word, word_buffer);
        apply_swap_16(word_buffer);

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = word_buffer[i];
        }
    }
}

void smi_unpack_xrgb_16(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;
    size_t full_words = count / 2;
    size_t tail_bytes = count % 2;

    for (size_t i = 0; i < full_words; i++)
    {
        uint32_t word  = raw[2*i];
        uint32_t word2 = raw[2*i + 1];

        uint16_t d0 = (word >> 8) & 0xFFFF;
        uint16_t d1 = ((word << 8) & 0xFF00) |
                      ((word2 >> 16) & 0x00FF);
        uint16_t d2 = word2 & 0xFFFF;

        dst[0] = d0;
        dst[1] = d1;
        dst[2] = d2;

        dst += 3;
    }

    if (tail_bytes)
    {
        uint32_t word = raw[2 * full_words];

        uint16_t d0 = (word >> 8) & 0xFFFF;
        dst[0] = d0;
    }
}

void smi_unpack_rgb565_16(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;
    size_t full_words = count / 2;
    size_t tail_bytes = count % 2;

    for(size_t i = 0; i < full_words; i++)
    {
        uint32_t word  = raw[i];

        uint16_t d0 = ((word >> 0) & 0xFFFF);
        uint16_t d1 = ((word >> 16));

        dst[0] = d0;
        dst[1] = d1;

        dst += 2;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];

        uint16_t bytes[4] = {
            ((word >> 0) & 0xFFFF),
            ((word >> 16))
        };

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = bytes[i];
        }
    }
}

void smi_unpack_xrgb_18(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint32_t* dst = out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word  = raw[i];

        uint32_t d0 = ((word >> 2) & 0x3F) | ((word >> 4) & 0xFC0) | ((word >> 6) & 0x3F000);

        dst[0] = d0;

        dst += 1;
    }
}

void smi_unpack_rgb565_18(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint32_t* dst = out;
    size_t full_words = count / 2;
    size_t tail_bytes = count % 2;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word  = raw[i];
        uint32_t w0 = (word >> 0) & 0xFFFF;
        uint32_t w1 = (word >> 16) & 0xFFFF;

        uint32_t d0 = ((w0 << 2)  & 0x3E000) | ((w0 >> 3)  & 0x1000) | ((w0 << 1)  & 0xFFE);
        uint32_t d1 = ((w1 << 2)  & 0x3E000) | ((w1 >> 3)  & 0x1000) | ((w1 << 1)  & 0xFFE);
        
        dst[0] = d0;
        dst[1] = d1;

        dst += 2;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];
        uint32_t w0 = (word >> 0) & 0xFFFF;
        uint32_t w1 = (word >> 16) & 0xFFFF;
        uint32_t bytes[4] = {
            ((w0 << 2)  & 0x3E000) | ((w0 >> 3)  & 0x1000) | ((w0 << 1)  & 0xFFE),
            ((w1 << 2)  & 0x3E000) | ((w1 >> 3)  & 0x1000) | ((w1 << 1)  & 0xFFE)
        };

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = bytes[i];
        }
    }
}

/* RGB565 18 bit swap mode is not correctly implemented - Bit order is reversed however unpacking is still a mystery */
static inline uint32_t reverse18(uint32_t v)
{
    v = ((v & 0x15555) << 1) | ((v & 0x2AAAA) >> 1);
    v = ((v & 0x03333) << 2) | ((v & 0x0CCCC) >> 2);
    v = ((v & 0x00F0F) << 4) | ((v & 0x0F0F0) >> 4);
    v = ((v & 0x000FF) << 8) | ((v & 0x3FC00) >> 8);
    return v;
}

void smi_unpack_rgb565_18_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint32_t* dst = out;
    size_t full_words = count / 2;
    size_t tail_bytes = count % 2;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word  = raw[i];
        uint32_t w0 = reverse18((word >> 0) & 0xFFFF);
        uint32_t w1 = reverse18((word >> 16) & 0xFFFF);

        uint32_t d0 = ((w0 << 2)  & 0x3E000) | ((w0 >> 3)  & 0x1000) | ((w0 << 1)  & 0xFFE);
        uint32_t d1 = ((w1 << 2)  & 0x3E000) | ((w1 >> 3)  & 0x1000) | ((w1 << 1)  & 0xFFE);

        dst[0] = d0;
        dst[1] = d1;

        dst += 2;
    }

    if(tail_bytes)
    {
        uint32_t word = raw[full_words];
        uint32_t w0 = reverse18((word >> 0) & 0xFFFF);
        uint32_t w1 = reverse18((word >> 16) & 0xFFFF);

        uint32_t bytes[4] = {
            ((w0 << 2)  & 0x3E000) | ((w0 >> 3)  & 0x1000) | ((w0 << 1)  & 0xFFE),
            ((w1 << 2)  & 0x3E000) | ((w1 >> 3)  & 0x1000) | ((w1 << 1)  & 0xFFE)
        };

        for(size_t i = 0; i < tail_bytes; i++)
        {
            dst[i] = bytes[i];
        }
    }
}

void smi_unpack(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count, smi_pack_ratio_t ratio)
{
    int width = cxt->rw_config->rconfig->rwidth;
    int format = cxt->rw_config->wconfig->wformat;
    int swap = cxt->rw_config->wconfig->wswap;

    switch (width)
    {
    case SMI_8_BITS:
        if (format == SMI_RGB565) smi_unpack_rgb565_8(data, ret_data, count, ratio);
        if (format == SMI_XRGB)   smi_unpack_xrgb_8(data, ret_data, count, ratio);
        return;

    case SMI_9_BITS:
        if (format == SMI_RGB565) (swap == 0) ? 
                smi_unpack_rgb565_9(data, ret_data, count, ratio) :
                smi_unpack_rgb565_9_swap(data, ret_data, count, ratio);
        if (format == SMI_XRGB)   (swap == 0) ? 
                smi_unpack_xrgb_9(data, ret_data, count, ratio) :
                smi_unpack_xrgb_9_swap(data, ret_data, count, ratio); 
        return;

    case SMI_16_BITS:
        if (format == SMI_RGB565) smi_unpack_rgb565_16(data, ret_data, count, ratio);
        if (format == SMI_XRGB)   smi_unpack_xrgb_16(data, ret_data, count, ratio);
        return;

    case SMI_18_BITS:
        if (format == SMI_RGB565) (swap == 0) ? 
                smi_unpack_rgb565_18(data, ret_data, count, ratio) :
                smi_unpack_rgb565_18_swap(data, ret_data, count, ratio);
        if (format == SMI_XRGB)   smi_unpack_xrgb_18(data, ret_data, count, ratio);
        return;
    }

    ERROR("Unknown interface configuration - could not unpack data");
}

smi_pack_ratio_t smi_packed_ratio(SMI_CXT* cxt)
{
    int width = cxt->rw_config->rconfig->rwidth;
    int format = cxt->rw_config->wconfig->wformat;

    switch (width)
    {
    case SMI_8_BITS:
        if (format == SMI_RGB565) return (smi_pack_ratio_t){1, 4};
        if (format == SMI_XRGB)   return (smi_pack_ratio_t){1, 3};
        break;

    case SMI_9_BITS:
        if (format == SMI_RGB565) return (smi_pack_ratio_t){1, 4};
        if (format == SMI_XRGB)   return (smi_pack_ratio_t){1, 2};
        break;

    case SMI_16_BITS:
        if (format == SMI_RGB565) return (smi_pack_ratio_t){1, 2};
        if (format == SMI_XRGB)   return (smi_pack_ratio_t){2, 3};
        break;

    case SMI_18_BITS:
        if (format == SMI_RGB565) return (smi_pack_ratio_t){1, 2};
        if (format == SMI_XRGB)   return (smi_pack_ratio_t){1, 1};
        break;
    }


    ERROR("Unknown interface configuration - using 1:1 ratio");
    return (smi_pack_ratio_t){1,1};
}