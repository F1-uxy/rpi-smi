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

    if((fd = open("/dev/udmabuf0", O_RDWR)) < 0)
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

void dma_sync_for_device()
{
    int fd = open(UDMABUF_SYS SYNC_DEVICE, O_WRONLY);
    write(fd, "1", 1);
    close(fd);
}

void dma_sync_for_cpu() 
{
    int fd = open(UDMABUF_SYS SYNC_CPU, O_WRONLY);
    write(fd, "1", 1);
    close(fd);
}

int check_buf(unsigned char* buf, unsigned int size)
{
    int m = 256;
    int n = 10;
    int i, k;
    int error_count = 0;
    while(--n > 0) 
    {
        for(i = 0; i < size; i = i + m) 
        {
            m = (i+256 < size) ? 256 : (size-i);
            for(k = 0; k < m; k++) 
            {
                buf[i+k] = (k & 0xFF);
            }
            for(k = 0; k < m; k++) 
            {
                if (buf[i+k] != (k & 0xFF)) 
                {
                    error_count++;
                }
            }
        }
    }
    return error_count;
}

int clear_buf(unsigned char* buf, unsigned int size)
{
    int n = 100;
    int error_count = 0;
    while(--n > 0) {
        memset((void*)buf, 0, size);
    }
    return error_count;
}

void enable_dma(MEM_MAP dma_regs, int channel)
{
    //*REG32(dma_regs, DMA_ENABLE) |= (1 << channel);
    //*REG32(dma_regs, DMA_REG(channel, DMA_REG_CS)) = CS_CR; /* Reset the channel */
}

void start_dma()
{
    
}

int dma_test_mem_transfer(void* buffer)
{
    DMA_CB* cbp = (DMA_CB*) buffer;
    char* src = (char*)(cbp+1);
    char* dest = src + 0x100;

    strcpy(src, "Memory transfer OK");
    memset(cbp, 0, sizeof(DMA_CB));

    cbp->ti = DMA_CB_SRC_INC | DMA_CB_DEST_INC;
    //cbp->src_addr = BUS_ALIAS_UNCACHED(src);
    //cbp->dest_addr = BUS_ALIAS_UNCACHED(dest);
    cbp->tfr_len = strlen(src) + 1;
    start_dma(cbp);
    sleep(10);      /* Replace with nanosleep for better sleep functionality */
    printf("DMA test: %s\n", dest[0] ? dest : "failed");
    return (dest[0] != 0);

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
    strcpy(msg, "Hello World!");
    

    cb->ti = DMA_CB_SRC_INC | DMA_CB_DEST_INC;
    uintptr_t src_offset  = (uintptr_t)msg  - (uintptr_t)dma_buffer.virt;
    uintptr_t dest_offset = (uintptr_t)dest - (uintptr_t)dma_buffer.virt;
    cb->src_addr  = (uint32_t)((uintptr_t)dma_buffer.bus + src_offset);
    cb->dest_addr = (uint32_t)((uintptr_t)dma_buffer.bus + dest_offset);
    cb->tfr_len   = strlen(msg) + 1;

    volatile uint32_t* dma_conblk_ad = (volatile uint32_t*)((char*)dma_cs + 0x04);
    *dma_conblk_ad = (uint32_t)dma_buffer.bus; 

    uint32_t read_conblk = *dma_conblk_ad;
    printf("DMA CONBLK_AD readback: 0x%08x\n", read_conblk);

    CS_SETBIT(*dma_cs, CS_ACTIVE);
    sleep(1);
    uint32_t cs = *dma_cs;
    if (cs & CS_END)  printf("DMA: END bit set\n");
    if (cs & CS_INT)  printf("DMA: INT bit set\n");
    if (cs & CS_DREQ) printf("DMA: DREQ bit set\n");
    if (!(cs & CS_END)) printf("DMA did not indicate completion (may have aborted). Check CB addresses and TI flags.\n");
    printf("dest virt: %p\n", (void*)dest);
    printf("dest content: '%s'\n", dest);
    unmap_segment(dma_buffer.virt, DMA_BUFFER_SIZE);
    unmap_segment(dma_regs, PAGE_SIZE);

    return 0;
}