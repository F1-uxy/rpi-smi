#include <stdint.h>

#include "smi.h"
#include "dma.h"


/* 
    --- DMA Transfers ---
    SMI can pace DMA transfers therefore, we can use DMA to take care of the data transfer while we done some real compute.
    The DMA transfers require a non-cacheable buffer to stop stale data being read by the DMA engine. The SMI CXT holds a pointer the start of the buffer.
    The provided functions configure the bare requirements for a transfer to take place. YOU are responsible for managing the buffer and any extra features.
*/

/* SMI DMA Read */
int smi_ex_dma_read(SMI_CXT* cxt)
{
    int len = 16; /* The length of the transfer in 32 bit words. e.g. 64 SMI transfers */

    MEM_MAP* dma_buffer = cxt->dma_buffer;
    DMA_CB* cb = (DMA_CB*)dma_buffer->virt;
    memset(cb, 0, sizeof(DMA_CB));

    uint32_t* rxdata = (uint32_t*)(cb+2);
    cb->dest_addr = MEM_BUS_ADDR(dma_buffer, rxdata); /* The control block speaks in BUS addresses */
    cb->ti = 0;
    cxt->rw_config->read_device_num = SMI_DEVICE1;
    cxt->rw_config->rconfig->rwidth = SMI_8_BITS;
    cxt->pxldata = 0;           /* The data can be packed however, the api will not automatically unpack */
    cxt->dma_config.panicr = 4; /* The FIFO level at which the SMI controller *panics* and elevates its priorities */
    cxt->dma_config.reqr = 1; /* The FIFO level at which the SMI controller generates its DREQ */

    smi_sync_context_device(&cxt); /* Sync the R/W config to the CXT's selected device config */

    /* The API takes the CXT struct, CB struct, interface address output, length in words and DMA channel number */
    smi_programmed_read_dma(&cxt, cb, 0, len, 0);
}

/* SMI DMA Write */
int smi_ex_dma_write(SMI_CXT* cxt)
{
    int len = 16; /* The length of the transfer in 32 bit words. e.g. 64 SMI transfers */

    MEM_MAP* dma_buffer = cxt->dma_buffer;
    DMA_CB* cb = (DMA_CB*)dma_buffer->virt;
    memset(cb, 0, sizeof(DMA_CB));

    char* txdata = (char*) (cb + 1);
    strcpy(txdata, "Test");

    cb->src_addr = MEM_BUS_ADDR(dma_buffer, txdata); /* The control block speaks in BUS addresses */
    cb->ti = 0;
    cxt->rw_config->read_device_num = SMI_DEVICE1;
    cxt->rw_config->rconfig->rwidth = SMI_8_BITS;
    cxt->pxldata = 0;
    cxt->dma_config.panicw = 4; /* The FIFO level at which the SMI controller *panics* and elevates its priorities */
    cxt->dma_config.reqw = 1; /* The FIFO level at which the SMI controller generates its DREQ */

    smi_sync_context_device(&cxt); /* Sync the R/W config to the CXT's selected device config */

    /* The API takes the CXT struct, CB struct, interface address output, length in words and DMA channel number */
    smi_programmed_write_dma(&cxt, cb, 0, len, 0);
}


int main()
{

    return 0;
}