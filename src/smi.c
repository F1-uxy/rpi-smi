#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sched.h>

#include "smi.h"
#include "clk.h"
#include "dma.h"
#include "errors.h"
#include "timeout.h"


void smi_init_cxt_map(SMI_CXT* cxt, MEM_MAP* smi_regs, MEM_MAP* clk_regs, MEM_MAP* gpio_regs, MEM_MAP* dma_regs)
{
    /* Map GPIO regs */
    map_segment(gpio_regs, GPIO_BASE, PAGE_SIZE);

    /* Map DMA regs */
    map_segment(dma_regs, DMA_BASE, NADJ_CHANNELS * PAGE_SIZE);

    /* Map SMI regs */
    map_segment(smi_regs, SMI_BASE, PAGE_SIZE);
    smi_regs->bus = smi_regs->phys - PHYS_REG_BASE + BUS_REG_BASE;
    smi_gpio_init(*gpio_regs);

    /* Map CLK regs */
    map_segment(clk_regs, CLK_BASE, PAGE_SIZE);

    /* Allocate raw buffer */
    cxt->raw_buffer.buf = malloc(1024 * sizeof(uint32_t));
    cxt->raw_buffer.size = (1024);

    if(!cxt->raw_buffer.buf)
    {
        ERROR("Raw buffer allocation failed");
        exit(1);
    }

    cxt->smi_regs  = smi_regs;
    cxt->clk_regs  = clk_regs;
    cxt->gpio_regs = gpio_regs;
    cxt->dma_regs  = dma_regs;
}

void smi_unmap_cxt(SMI_CXT* cxt)
{
    unmap_segment(cxt->gpio_regs, PAGE_SIZE);
    unmap_segment(cxt->dma_regs, NADJ_CHANNELS * PAGE_SIZE);
    unmap_segment(cxt->smi_regs, PAGE_SIZE);
    unmap_segment(cxt->clk_regs, PAGE_SIZE);

    if(cxt->raw_buffer.buf != NULL)
    {
        free(cxt->raw_buffer.buf);
    }

    cxt->raw_buffer.size = 0;
}

void smi_init_rw_config(SMI_CXT* cxt, SMI_RW* rw, SMI_CLK* clk, SMI_READ* rconfig, SMI_WRITE* wconfig, int read_device, int write_device)
{
    rw->read_device_num = read_device;
    rw->write_device_num = write_device;
    rw->rconfig[write_device] = *rconfig;
    rw->wconfig[write_device] = *wconfig;
    rw->clk = clk;
    cxt->rw_config = rw;
}

int smi_init_udmabuf(SMI_CXT* cxt, MEM_MAP* dma_buffer)
{
    int fd_sync_cpu = open("/sys/class/u-dma-buf/udmabuf0/sync_for_cpu", O_WRONLY);
    int fd_sync_dev = open("/sys/class/u-dma-buf/udmabuf0/sync_for_device", O_WRONLY);

    if(fd_sync_cpu < 0 || fd_sync_dev < 0)
    {
        if (fd_sync_cpu >= 0) close(fd_sync_cpu);
        if (fd_sync_dev >= 0) close(fd_sync_dev);
        ERROR("Could not open u-dma-buf sync");
        
        return -1;
    }

    dma_buffer_init(dma_buffer, 1, 1);

    cxt->fd_sync_dev = fd_sync_dev;
    cxt->fd_sync_cpu = fd_sync_cpu;
    cxt->dma_buffer = dma_buffer;

    return 0;
}

void smi_unmap_udmabuf(SMI_CXT* cxt)
{
    unmap_segment(cxt->dma_buffer, DMA_BUFFER_SIZE);
    if (cxt->fd_sync_cpu >= 0) close(cxt->fd_sync_cpu);
    if (cxt->fd_sync_dev >= 0) close(cxt->fd_sync_dev);
}

/* ns: Clock period; even number 2 -> 30*/
void init_smi_clk(MEM_MAP clk_regs, MEM_MAP smi_regs, int ns)
{    
    int divi = ns/2;

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
}

void smi_sync_context_device(SMI_CXT* cxt)
{
    smi_configure_write_device(cxt, cxt->rw_config->write_device_num);
    smi_configure_read_device(cxt, cxt->rw_config->read_device_num);
}

void smi_configure_read_device(SMI_CXT* cxt, uint8_t n)
{
    if(n > 3)
    {
        ERROR("Device number out of range");
        return;
    }

    volatile SMI_DSR* dsr = (volatile SMI_DSR*) REG32((*cxt->smi_regs), SMIO_DSR(n));
    
    dsr->fields.rstrobe = cxt->rw_config->rconfig[n].rstrobe;
    dsr->fields.rdreq = cxt->rw_config->rconfig[n].rdreq;
    dsr->fields.rpace = cxt->rw_config->rconfig[n].rpace;
    dsr->fields.rpaceall = cxt->rw_config->rconfig[n].rpaceall;
    dsr->fields.rhold = cxt->rw_config->rconfig[n].rhold;
    dsr->fields.fsetup = cxt->rw_config->rconfig[n].fsetup;
    dsr->fields.mode68 = cxt->rw_config->rconfig[n].mode68;
    dsr->fields.rsetup = cxt->rw_config->rconfig[n].rsetup;
    dsr->fields.rwidth = cxt->rw_config->rconfig[n].rwidth;
}

void smi_configure_write_device(SMI_CXT* cxt, uint8_t n)
{
    if(n > 3)
    {
        ERROR("Device number out of range");
        return;
    }

    volatile SMI_DSW* dsw = (volatile SMI_DSW*) REG32((*cxt->smi_regs), SMIO_DSW(n));

    dsw->fields.wsetup = cxt->rw_config->wconfig[n].wformat;
    dsw->fields.wwidth = cxt->rw_config->wconfig[n].wwidth;
    dsw->fields.whold = cxt->rw_config->wconfig[n].whold;
    dsw->fields.wpace = cxt->rw_config->wconfig[n].wpace;
    dsw->fields.wpaceall = cxt->rw_config->wconfig[n].wpaceall;
    dsw->fields.wstrobe = cxt->rw_config->wconfig[n].wstrobe;
    dsw->fields.wdreq = cxt->rw_config->wconfig[n].wdreq;
    dsw->fields.wswap = cxt->rw_config->wconfig[n].wswap;
    dsw->fields.wformat = cxt->rw_config->wconfig[n].wformat;
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

void smi_8b_init(MEM_MAP gpio_map)
{
    for(int i = 0; i < 26; i++)
    {
        gpio_mode(gpio_map, i, GPIO_ALT1);
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

    if(ret_data == NULL) return -EINVAL;

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    
    deadline = start_timeout(PROG_READ_TIMEOUT_S);

    /* We can assume that if data is leftover in the FIFO is hardware fault? */
    while(count < len)
    {
        //printf("Len %d vs Count %d ; Done %d\n", len, count, (cs->fields.done > 0));
        //printf("RXF: %d ; RXD: %d ; RXR: %d ; TXE: %d ; TXD: %d ; TXW: %d\n", (cs->fields.rxf > 0), (cs->fields.rxd > 0), (cs->fields.rxr > 0), (cs->fields.txe > 0), (cs->fields.txd > 0), (cs->fields.txw > 0));
        
        if (cs->fields.rxd) 
        {
            ret_data[count++] = d->value;
            spin = 0;
            continue;
        }

        if(timeout_apply(deadline, timeout_spin_tier(spin, SPIN_HARD_LIMIT, SPIN_YIELD_LIMIT, SPIN_SOFT_LIMIT)))
        {
            ERROR("Read await timeout reached");
            cs->fields.clear = 1;
            return -ETIMEDOUT;
        }
        spin++;
    }

    // while(cs->fields.rxd && count < len)
    // {
    //     //printf("Len %d vs Count %d ; Done %d\n", len, count, (cs->fields.done > 0));
    //     //printf("RXF: %d ; RXD: %d ; RXR: %d ; TXE: %d ; TXD: %d ; TXW: %d\n", (cs->fields.rxf > 0), (cs->fields.rxd > 0), (cs->fields.rxr > 0), (cs->fields.txe > 0), (cs->fields.txd > 0), (cs->fields.txw > 0));
    //     ret_data[count++] = d->value;
    // }

    if(count < len)
    {
        ERROR("Transfer complete but data lost. Slow the clock");
    }

    if(cs->fields.rxd)
    {
        ERROR("Leftover data in the RX FIFO");
    }
    
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
            if(timeout_apply(deadline, timeout_spin_tier(spin, SPIN_HARD_LIMIT, SPIN_YIELD_LIMIT, SPIN_SOFT_LIMIT)))
            {
                ERROR("Direct read await timeout reached");
                cs->fields.clear = 1;
                dcs->fields.done = 1;
                return -ETIMEDOUT;
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

    dsr->fields.rwidth = 0;
    dcs->fields.done = 1;

    dcs->fields.write = 0;

    da->fields.addr = addr;
    dd->value = 0;

    dcs->fields.start = 1;
    
    int count = smi_read_await_direct(cxt, ret_data, addr, 1, 0);

    return count;
}

int smi_direct_read_arr(SMI_CXT* cxt, uint32_t* ret_data, uint8_t addr, int len, int increment)
{
    volatile SMI_DA*  da  = (volatile SMI_DA*)  REG32((*cxt->smi_regs), SMIO_DA);
    volatile SMI_DCS* dcs = (volatile SMI_DCS*) REG32((*cxt->smi_regs), SMIO_DCS);
    volatile SMI_DD*  dd  = (volatile SMI_DD*)  REG32((*cxt->smi_regs), SMIO_DD);
    volatile SMI_DSR* dsr = (volatile SMI_DSR*) REG32((*cxt->smi_regs), SMIO_DSR0);

    dsr->fields.rwidth = cxt->rw_config->rconfig->rwidth;
    dcs->value = 0;
    dcs->fields.done = 1;
    dcs->fields.write = 0;

    da->fields.addr = addr;
    dd->value = 0;

    dcs->fields.start = 1;
    int count = smi_read_await_direct(cxt, ret_data, addr, len, increment);
    
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

    int count = 0;
    smi_pack_ratio_t ratio = smi_packed_ratio(cxt);
    int word_reads = SMI_DIV_CEIL((len * ratio.read), ratio.out_pixels);

    if(word_reads > cxt->raw_buffer.size)
    {
        LOG("Previous buffer too small, resizing to fit");
        
        uint32_t* new_buf = realloc(cxt->raw_buffer.buf, word_reads * sizeof(uint32_t));
        if(!new_buf)
        {
            ERROR("Reallocation of raw buffer failed");
            return -1;
        }
        cxt->raw_buffer.buf = new_buf;
        cxt->raw_buffer.size = word_reads;
    }

    uint32_t* raw_data = cxt->raw_buffer.buf;

    a->fields.device = cxt->rw_config->read_device_num;
    /*
    This isn't really the read functions concern? It should just be selecting the device and then the config is seperate
    */
    volatile SMI_DSR* dsr = (volatile SMI_DSR*) REG32((*cxt->smi_regs), SMIO_DSR(cxt->rw_config->read_device_num));
    volatile SMI_DSW* dsw = (volatile SMI_DSW*) REG32((*cxt->smi_regs), SMIO_DSW(cxt->rw_config->write_device_num));
    dsr->fields.rwidth = cxt->rw_config->rconfig->rwidth;
    dsw->fields.wformat = cxt->rw_config->wconfig->wformat;
    dsw->fields.wswap = cxt->rw_config->wconfig->wswap;

    cs->fields.aferr = 0;
    cs->fields.seterr = 1;

    cs->fields.pxldat = cxt->pxldata;
    cs->fields.enable = 1;
    cs->fields.write = 0;
    cs->fields.clear = 1;
    cs->fields.pad = cxt->pad;
    cs->fields.prdy = cxt->prdy;
    cs->fields.intr = cxt->intr;
    cs->fields.intd = cxt->intd;
    cs->fields.pvmode = cxt->pixel_value_mode;

    l->fields.length = len;
    a->fields.addr = addr;
    smi_start(cxt);
    count = smi_read_await(cxt, raw_data, word_reads);
    /*  Unpack all the raw data even if we didn't fill the buffer during reads.
        We can't count the individual reads when packed, only the overall words read.
    */
    cxt->pxldata ? smi_unpack(cxt, raw_data, ret_data, word_reads, ratio) : smi_truncate(cxt, raw_data, ret_data, count);
    return count;

}

int smi_write_await(SMI_CXT* cxt, uint32_t* data, uint8_t addr, int len)
{
    if(data == NULL || cxt == NULL) return -1;

    volatile SMI_CS*  cs  = (volatile SMI_CS*)  REG32((*cxt->smi_regs), SMIO_CS);
    volatile SMI_D*  d  = (volatile SMI_D*)  REG32((*cxt->smi_regs), SMIO_D);

    int count = 0;
    int spin = 0;

    smi_timeout_ns deadline;
    deadline = start_timeout(PROG_WRITE_TIMEOUT_S);
    
    while(!cs->fields.done)
    {
        if(count < len + 1)
        {
            d->value = data[count++];
        }

        while(!cs->fields.txd)
        {
            if(timeout_apply(deadline, timeout_spin_tier(spin, SPIN_HARD_LIMIT, SPIN_YIELD_LIMIT, SPIN_SOFT_LIMIT)))
            {
                ERROR("Write await timeout reached. Interface not receiving data");
                cs->fields.clear = 1;
                cs->fields.done = 1;
                return -ETIMEDOUT;
            }

            spin++;
        }
        
        if(timeout_apply(deadline, timeout_spin_tier(spin, SPIN_HARD_LIMIT, SPIN_YIELD_LIMIT, SPIN_SOFT_LIMIT)))
        {
            ERROR("Write await timeout reached. Data available but not readable");
            cs->fields.clear = 1;
            cs->fields.done = 1;
            return -ETIMEDOUT;
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
            if(timeout_apply(deadline, timeout_spin_tier(spin, SPIN_HARD_LIMIT, SPIN_YIELD_LIMIT, SPIN_SOFT_LIMIT)))
            {
                ERROR("Direct write await timeout reached");
                cs->fields.clear = 1;
                dcs->fields.done = 1;
                return -ETIMEDOUT;
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

    int count = 0;
    
    cs->fields.aferr = 0;
    cs->fields.seterr = 1;
    cs->fields.done = 1;
    cs->fields.enable = 1;
    cs->fields.write = 1;
    cs->fields.clear = 1;
    cs->fields.pxldat = 0;
    cs->fields.pad = cxt->pad;
    cs->fields.prdy = cxt->prdy;
    cs->fields.intr = cxt->intr;
    cs->fields.intd = cxt->intd;
    cs->fields.intt = cxt->intt;


    l->fields.length = len;
    a->fields.addr = addr;

    smi_start(cxt);
    count = smi_write_await(cxt, data, addr, len);

    return count;

}

int smi_dma_await(SMI_CXT* cxt)
{
    volatile DMA_CS* dma_cs = (volatile DMA_CS*)  REG32((*cxt->dma_regs), DMAO_CS);

    long delay = 1000;    
    smi_timeout_ns deadline;
    
    int spin = 0;
    deadline = start_timeout(PROG_READ_TIMEOUT_S);

    while(!dma_cs->fields.end)
    {
        if(timeout_apply(deadline, timeout_spin_tier(spin, SPIN_HARD_LIMIT, SPIN_YIELD_LIMIT, SPIN_SOFT_LIMIT)))
        {
            ERROR("DMA await timeout reached");
            dma_cs->fields.abort = 1;
            return -ETIMEDOUT;
        }

        spin++;
    }
    
    return 1;
}

int smi_programmed_write_dma(SMI_CXT* cxt, DMA_CB* cb, uint8_t addr, int len, int channel)
{
    if(cxt == NULL || cb == NULL) return -1;

    if(cxt->smi_regs == NULL) return -1;

    volatile SMI_CS* cs = (volatile SMI_CS*) REG32((*cxt->smi_regs), SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32((*cxt->smi_regs), SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32((*cxt->smi_regs), SMIO_A);
    volatile SMI_DC* dc = (volatile SMI_DC*) REG32((*cxt->smi_regs), SMIO_DC);

    MEM_MAP* smi_regs = cxt->smi_regs;
    MEM_MAP* dma_regs = cxt->dma_regs;
    MEM_MAP* dma_buffer = cxt->dma_buffer;

    cb->ti = DMA_DEST_DREQ | (DMA_SMI_DREQ << 16) | DMA_CB_SRCE_INC;
    cb->dest_addr = REG_BUS_ADDR((*smi_regs), SMIO_D);
    cb->tfr_len = SMI_DMA_L(len);

    cb->next_cb = 0;
    cb->stride = 0;

    cs->fields.clear = 1;
    cs->fields.enable = 1;
    cs->fields.pxldat = cxt->pxldata;
    cs->fields.write = 1;

    a->fields.addr = addr;
    l->fields.length = len;

    dc->fields.dmaen = 1;
    dc->fields.panicw = cxt->dma_config.panicw;
    dc->fields.reqw   = cxt->dma_config.reqw;

    smi_start(cxt);
    uintptr_t cb_addr = MEM_BUS_ADDR(dma_buffer, cb);
    start_dma(dma_regs->virt, cb_addr, channel, cxt->fd_sync_dev, cxt->fd_sync_cpu);
    int err = smi_dma_await(cxt);
        
    return err;
}

int smi_programmed_read_dma(SMI_CXT* cxt, DMA_CB* cb, uint8_t addr, int len, int channel)
{
    if(cxt == NULL || cb == NULL) return -1;

    if(cxt->smi_regs == NULL) return -1;

    volatile SMI_CS* cs = (volatile SMI_CS*) REG32((*cxt->smi_regs), SMIO_CS);    
    volatile SMI_L*  l = (volatile SMI_L*) REG32((*cxt->smi_regs), SMIO_L);
    volatile SMI_A*  a = (volatile SMI_A*) REG32((*cxt->smi_regs), SMIO_A);
    volatile SMI_DC* dc = (volatile SMI_DC*) REG32((*cxt->smi_regs), SMIO_DC);

    MEM_MAP* smi_regs = cxt->smi_regs;
    MEM_MAP* dma_regs = cxt->dma_regs;
    MEM_MAP* dma_buffer = cxt->dma_buffer;

    smi_pack_ratio_t ratio = smi_packed_ratio(cxt);
    int word_reads = SMI_DIV_CEIL((len * ratio.read), ratio.out_pixels);
    
    //if(word_reads > 4096)
    //{
    //    ERROR("DMA transfer length is greater than 16384. Consider splitting into multiple chained CBs");
    //    return -1;
    //}

    /* Required DMA CB flags */
    cb->ti = DMA_SRCE_DREQ | (DMA_SMI_DREQ << 16) | DMA_CB_DEST_INC | DMA_WAIT_RSP;
    cb->src_addr = REG_BUS_ADDR((*smi_regs), SMIO_D);
    cb->tfr_len = SMI_DMA_L(word_reads);

    cb->next_cb = 0;
    cb->stride = 0;

    cs->fields.clear = 1;
    cs->fields.enable = 1;

    cs->fields.pxldat = cxt->pxldata;
    cs->fields.pvmode = 0;
    cs->fields.pad = 0;
    cs->fields.write = 0;

    a->fields.addr = addr;
    l->fields.length = len;

    dc->fields.dmaen = 1;
    dc->fields.panicr = cxt->dma_config.panicr;
    dc->fields.reqr   = cxt->dma_config.reqr;

    uintptr_t cb_addr = MEM_BUS_ADDR(dma_buffer, cb);
    start_dma(dma_regs->virt, cb_addr, channel, cxt->fd_sync_dev, cxt->fd_sync_cpu);
    smi_start(cxt);
    int err = smi_dma_await(cxt);
    
    return err;
}

void smi_unpack_rgb565_8(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint8_t* dst = out;

    for(size_t i = 0; i < count; i++)
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
}

void smi_unpack_xrgb_8(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint8_t* dst = out;

    for(size_t i = 0; i < count; i++)
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
}

/* 
    Data1 = { FIFO[12:10], FIFO[7:2] } 
    Data2 = { FIFO[23:18], FIFO[15:13] }
*/
void smi_unpack_xrgb_9(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];

        uint16_t d0 = ((word >> 13) & 0x7) | ((word >> 15) & 0x1F8);
        uint16_t d1 = ((word >> 2) & 0x3F) | ((word >> 4) & 0x1C0);
        dst[0] = d0;
        dst[1] = d1;
        dst += 2;
    }
}

/* 
    Data1 = { FIFO[23:18], FIFO[15:13] }
    Data2 = { FIFO[12:10], FIFO[7:2] } 
*/
void smi_unpack_xrgb_9_swap(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];

        uint16_t d0 = ((word >> 2) & 0x3F) | ((word >> 4) & 0x1C0);
        uint16_t d1 = ((word >> 13) & 0x7) | ((word >> 15) & 0x1F8);
        dst[0] = d0;
        dst[1] = d1;
        dst += 2;
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
    uint16_t word_buffer[4];

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];
        smi_unpack_rgb565_9_word(word, word_buffer);

        dst[0] = word_buffer[0];
        dst[1] = word_buffer[1];
        dst[2] = word_buffer[2];
        dst[3] = word_buffer[3];

        dst += 4;
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
    uint16_t word_buffer[4];

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word = raw[i];

        smi_unpack_rgb565_9_word(word, word_buffer);

        dst[0] = word_buffer[1];
        dst[1] = word_buffer[0];
        dst[2] = word_buffer[3];
        dst[3] = word_buffer[2];

        dst += 4;
    }
}

void smi_unpack_xrgb_16(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;
    size_t pairs = count / 2;

    for (size_t i = 0; i < pairs; i++)
    {
        uint32_t word  = raw[2*i];
        uint32_t word2 = raw[2*i + 1];

        uint16_t d0 = (word >> 8) & 0xFFFF;
        uint16_t d1 = ((word << 8) & 0xFF00) | ((word2 >> 16) & 0x00FF);
        uint16_t d2 = word2 & 0xFFFF;

        dst[0] = d0;
        dst[1] = d1;
        dst[2] = d2;

        dst += 3;
    }
}

void smi_unpack_rgb565_16(const uint32_t* raw, void* out, size_t count, smi_pack_ratio_t ratio)
{
    uint16_t* dst = out;

    for(size_t i = 0; i < count; i++)
    {
        uint32_t word  = raw[i];

        uint16_t d0 = ((word >> 0) & 0xFFFF);
        uint16_t d1 = ((word >> 16) & 0xFFFF);

        dst[0] = d0;
        dst[1] = d1;

        dst += 2;
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
    return;
}

smi_pack_ratio_t smi_packed_ratio(SMI_CXT* cxt)
{
    int width = cxt->rw_config->rconfig->rwidth;
    int format = cxt->rw_config->wconfig->wformat;

    if(cxt->pxldata == 0){ return (smi_pack_ratio_t){1,1}; }

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

void smi_truncate_8(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    uint8_t* out = ret_data;
    for(size_t i = 0; i < count; i++)
    {
        out[i] = data[i] & 0xFF;
    }
}

void smi_truncate_9(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    uint16_t* out = ret_data;
    for(size_t i = 0; i < count; i++)
    {
        out[i] = data[i] & 0x1FF;
    }
}

void smi_truncate_16(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    uint16_t* out = ret_data;
    for(size_t i = 0; i < count; i++)
    {
        out[i] = data[i] & 0xFFFF;
    } 
}

void smi_truncate_18(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    uint32_t* out = ret_data;
    for(size_t i = 0; i < count; i++)
    {
        out[i] = data[i] & 0x3FFFF;
    }
}

void smi_truncate(SMI_CXT* cxt, uint32_t* data, void* ret_data, size_t count)
{
    int width = cxt->rw_config->rconfig->rwidth;
        
    switch (width)
    {
    case SMI_8_BITS:
        smi_truncate_8(cxt, data, ret_data, count);
        break;

    case SMI_9_BITS:
        smi_truncate_9(cxt, data, ret_data, count);
        break;

    case SMI_16_BITS:
        smi_truncate_16(cxt, data, ret_data, count);
        break;

    case SMI_18_BITS:
        smi_truncate_18(cxt, data, ret_data, count);
        break;

    default:
        ERROR("Unknown interface configuration - returning untruncated data");
    }

    return;
}