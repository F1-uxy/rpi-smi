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

#define UDMABUF_SYS "/sys/class/u-dma-buf/udmabuf0/"
#define SIZE_FILE "size"
#define PHYS_ADDR "phys_addr"
#define SYNC_CPU "sync_for_cpu"
#define SYNC_DEVICE "sync_for_device"

size_t read_sysfile_size(const char* file)
{
    FILE* f = fopen(file, "r");
    if(!f)
    {
        perror("ERROR: File size couldn't be read\n");
        exit(1);
    }

    size_t size = 0;
    fscanf(f, "%zu", &size);
    fclose(f);
    
    return size;
}

void* read_sysfile_phys_addr(const char* file)
{
    FILE* f = fopen(file, "r");
    if(!f)
    {
        perror("ERROR: File size couldn't be read\n");
        exit(1);
    }

    void* location;
    fscanf(f, "%p", &location);
    fclose(f);
    
    return location;
}


void* map_segment(uint32_t addr, int size)
{
    int fd;
    void* mem;

    size = PAGE_ROUNDUP(size);
    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        perror("Error: Could not open /dev/\n");
        exit(1);
    }

    mem = mmap(0, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, (uint32_t)addr);
    close(fd);

    if(mem == MAP_FAILED)
    {
        perror("Error: Could not map memory\n");
        exit(1);
    }

    return mem;
}

void unmap_segment(void *mem, int size)
{
    if (mem)
        munmap(mem, PAGE_ROUNDUP(size));
}

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

    CS_SETBIT(*dma_cs, CS_ACTIVE);

    return 0;
}

int main()
{
    MEM_MAP dma_buffer;
    
    /* Map u-dma buffer */
    dma_buffer.virt = map_dma_buffer(DMA_BUFFER_SIZE);
    if(dma_buffer.virt == NULL)
    {
        perror("Error mapping segment\n");
        return 1;
    }

    dma_buffer.size = read_sysfile_size(UDMABUF_SYS SIZE_FILE);
    printf("Buffer size: %zu\n", dma_buffer.size);

    dma_buffer.phys = read_sysfile_phys_addr(UDMABUF_SYS PHYS_ADDR);
    dma_buffer.bus = dma_buffer.phys + 0xC0000000;
    printf("Virtual Address: %p\n", dma_buffer.virt);
    printf("Physical Address: %p\n", dma_buffer.phys);
    printf("Bus Address: %p\n", dma_buffer.bus);
    
    int error_count = check_buf((unsigned char*)dma_buffer.virt, DMA_BUFFER_SIZE);
    fprintf(stdout, "Error count: %d\n", error_count);
    clear_buf((unsigned char*)dma_buffer.virt, DMA_BUFFER_SIZE);

    /* Map DMA regs */
    void* dma_regs = map_segment(DMA_BASE, NADJ_CHANNELS * PAGE_SIZE);
    memset(dma_regs, 0, sizeof(14 * PAGE_SIZE));

    volatile uint32_t* dma_cs = DMA_N_REG(dma_regs, 0);

    CS_SETBIT(*dma_cs, CS_CR);
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

    start_dma(&dma_buffer, dma_regs, 0, cb);

    sleep(0);
    uint32_t cs = *dma_cs;
    uint32_t debug = *dma_cs + 0x20;
    printf("Version: %i", (debug & DB_VERSION));
    if(cs & CS_END) 
    {
        printf("dest virt: %p\n", (void*)dest);
        printf("dest content: '%s'\n", dest);
    } else
    {
        perror("DMA transfer incomplete\n");
    }
    
    void* smi_regs = map_segment(SMI_BASE, PAGE_SIZE);
    volatile SMI_CS* smi_cs;
    volatile SMI_DSR* smi_dsr;
    volatile SMI_DSW* smi_dsw;
    volatile SMI_DCS* smi_dcs;
    volatile SMI_DCD*  smi_dd;


    smi_cs = smi_regs + SMIO_CS;
    smi_dsr = smi_regs + SMIO_DSR0;
    smi_dsw = smi_regs + SMIO_DSW0;
    smi_dcs = smi_regs + SMIO_DCS;
    smi_dd = smi_regs + SMIO_DCD;
    smi_cs_init(smi_cs);
    init_smi(smi_cs, smi_dsr, smi_dsw, 100, 1, 25, 50, 25);

    smi_dcs->fields.done = 1;
    smi_dcs->fields.write = 1;
    smi_dd->value = 0xFF;
    smi_dcs->fields.start = 1;



    unmap_segment(dma_buffer.virt, DMA_BUFFER_SIZE);
    unmap_segment(dma_regs, PAGE_SIZE);

    return 0;
}
