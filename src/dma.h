#ifndef DMA_H
#define DMA_H

#include "gpio.h"

#define DMA_BUFFER_SIZE 1024

#define UDMABUF_SYS "/sys/class/u-dma-buf/udmabuf0/"
#define SIZE_FILE "size"
#define PHYS_ADDR "phys_addr"
#define SYNC_CPU "sync_for_cpu"
#define SYNC_DEVICE "sync_for_device"

/*
** DMA controller
*/

#define NADJ_CHANNELS 14 /* Number of adjacent DMA channels (15th channel physically removed)*/

typedef struct {
    uint32_t ti,        // Transfer info
             src_addr,  // Source address
             dest_addr, // Desitination address
             tfr_len,   // Transfer length
             stride,    
             next_cb,   // Next control block
             debug,     // Debug register
             unused;
} DMA_CB __attribute__ ((aligned(32)));

#define DMA_CB_DEST_INC     (1<<4)
#define DMA_CB_DEST_DREQ    (1<<6)
#define DMA_CB_SRC_INC      (1<<8)
#define DMA_SRCE_DREQ       (1<<10)

#define DMA_SMI_DREQ 4 

#define DMA_N_REG(base, n) ((uintptr_t*)((char*)base + (n * 0x100)))

#define DMA_BASE        0x3F007000

/*
 * DMA Control Block register
 * Returns offsets to DMA register base
*/
#define DMA_CTRL_BLK(c)		((c * 0x100) + 0x04 ) /* Get offset of DMA channel register CTRL_BLK */

/* DMA Channel Offset */
#define DMA_CHANNEL_0  0x000
#define DMA_CHANNEL_1  0x100
#define DMA_CHANNEL_2  0x200
#define DMA_CHANNEL_3  0x300
#define DMA_CHANNEL_4  0x400
#define DMA_CHANNEL_5  0x500
#define DMA_CHANNEL_6  0x600
#define DMA_CHANNEL_7  0x700
#define DMA_CHANNEL_8  0x800
#define DMA_CHANNEL_9  0x900
#define DMA_CHANNEL_10 0xa00
#define DMA_CHANNEL_11 0xb00
#define DMA_CHANNEL_12 0xc00
#define DMA_CHANNEL_13 0xd00
#define DMA_CHANNEL_14 0xe00


/* DMA Control and Status register */

#define DMA_CS_OFFSET(c)    (uintptr_t)(c * 0x100) /* Get offset of DMA CS register from DMA regs base*/

typedef struct
{
    volatile uint32_t   active          : 1,
                        end             : 1,
                        intr            : 1,
                        dreq            : 1,
                        pause           : 1,
                        dreqstop        : 1,
                        waiting_write   : 1,
                        _x1             : 1,
                        error           : 1,
                        _x2             : 7,
                        priority        : 4,
                        panic_priority  : 4,
                        _x3             : 4,
                        wait_write      : 1,
                        disdebug        : 1,
                        abort           : 1,
                        reset           : 1;
} DMA_CS_BITFIELD;

typedef union
{
    DMA_CS_BITFIELD fields;
    volatile uint32_t value;
} DMA_CS __attribute__((aligned(32)));

#define CS_CR                   (1 << 31)   /* DMA Channel Reset */
#define CS_ABORT                (1 << 30)
#define CS_DISDEBUG             (1 << 29)
#define CS_WFOW                 (1 << 28)
#define CS_PANIC_PRIORITY       (15 << 20)
#define CS_PRIORITY             (15 << 16)
#define CS_ERROR                (1 << 8)
#define CS_WGFOW                (1 << 6)
#define CS_DREQ_STOP            (1 << 5)
#define CS_PAUSED               (1 << 4)
#define CS_DREQ                 (1 << 3)
#define CS_INT                  (1 << 2)
#define CS_END                  (1 << 1)
#define CS_ACTIVE               (1 << 0)

/* DMA Control Block Address Register */
typedef struct
{
    volatile uint32_t scb_addr;
} DMA_CONBLK_AD_BITFIELD;

typedef union
{
    DMA_CONBLK_AD_BITFIELD fields;
    volatile uint32_t value;
} DMA_CONBLK_AD __attribute__((aligned(32)));

/* DMA Debug register */
#define DMA_DEBUG(cs)		((c * 0x100) + 0x20)
#define DB_LITE			    (1 << 28)
#define DB_VERSION		    (7 << 25)
#define DB_STATE		    (255 << 16)
#define DB_ID			    (255 << 8)
#define DB_OUTSTANDING_WR	(15 << 4)
#define DB_RD_ERR		    (1 << 2)
#define DB_FIFO_ERR		    (1 << 1)
#define DB_RLNSE		    (1 << 0)



void* map_dma_buffer(size_t buf_size);
int start_dma(MEM_MAP* dma_buffer, MEM_MAP dma_regs, int fd_sync_dev , uint8_t channel, DMA_CB* cb);
size_t dma_buffer_init(MEM_MAP* buff, int check, int clear);

int check_buf(unsigned char* buf, unsigned int size);
int clear_buf(unsigned char* buf, unsigned int size);


#endif
