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
            a->value = addr + count;
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
    a->value = 0;
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
    a->value = addr;
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
    a->value = addr;

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

inline void smi_read(int mode, uint32_t word, uint32_t* ret_data, int count, int len)
{
    switch (mode)
    {
    case SMI_8_BITS:
        if (count < len) ret_data[count++] = (word >>  0) & 0xFF;
        if (count < len) ret_data[count++] = (word >>  8) & 0xFF;
        if (count < len) ret_data[count++] = (word >> 16) & 0xFF;
        if (count < len) ret_data[count++] = (word >> 24) & 0xFF;
        break;
    case SMI_9_BITS:
        if (count < len) 
        break;
    default:
        break;
    }
    
}

void smi_start(SMI_CXT* cxt)
{
    volatile SMI_CS* cs = (volatile SMI_CS*) REG32((*cxt->smi_regs), SMIO_CS);
    cs->fields.start = 1;
}

int smi_read_await(SMI_CXT* cxt, uint32_t* ret_data, int len)
{
    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_D*  d = (volatile SMI_D*) REG32((*cxt->smi_regs), SMIO_D);
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);

    if(ret_data == NULL) return -1;

    cs->fields.teen = 0;

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    deadline = start_timeout(PROG_READ_TIMEOUT_S);

    while ((!cs->fields.done || cs->fields.rxd > 1) && (count < len)) 
    {

        if(cs->fields.seterr) 
        {
            ERROR("A, L or D register written to during programmed read");
            cs->fields.seterr = 1;
            return -EIO;
        }

        if (cs->fields.rxd) {
            uint32_t word = d->value;
            if (count < len) ret_data[count++] = (word >>  0) & 0xFF;
            if (count < len) ret_data[count++] = (word >>  8) & 0xFF;
            if (count < len) ret_data[count++] = (word >> 16) & 0xFF;
            if (count < len) ret_data[count++] = (word >> 24) & 0xFF;
        }

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
    
    smi_read_await_direct(cxt, ret_data, addr, 1, 0);

    return 1;
}

int smi_direct_read_arr(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len, int increment)
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

    dcs->fields.write = 0;

    da->fields.addr = addr;
    dd->value = 0; /* flush stale data */

    dcs->fields.start = 1;
    
    int count = smi_read_await_direct(cxt, ret_data, addr, len, increment);
    
    return count;
}

int smi_programmed_read(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr)
{
    return smi_programmed_read_arr(cxt, ret_data, addr, 1);
}

int smi_programmed_read_arr(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len)
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
    cs->fields.write = 0;
    cs->fields.clear = 1;

    l->fields.length = len;
    a->fields.addr = addr;

    smi_start(cxt);
    count = smi_read_await(cxt, ret_data, len);

    return count;

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
    a->value = addr;
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