#ifndef DMA_H
#define DMA_H

#include "gpio.h"

#define DMA_BUFFER_SIZE 1024


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

#define DMA_CB_DEST_INC (1<<4)
#define DMA_CB_SRC_INC  (1<<8)

#define DMA_N_REG(base, n) ((uint32_t*)((char*)base + (n * 0x100)))

#define DMA_BASE        0x3F007000

/*
 * DMA Control Block register
 * Returns offsets to DMA register base
*/
#define DMA_CTRL_BLK(c)		((c * 0x100) + 0x04 ) /* Get offset of DMA channel register CTRL_BLK */

/* DMA Control and Status register */
#define CS_SETBIT(cs, n)        (cs |= n)
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

#endif
