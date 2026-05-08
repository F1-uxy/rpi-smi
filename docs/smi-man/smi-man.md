% SMI(7)
% Charlie Flux
% April 2026

# NAME
smi - Secondary Memory Interface (SMI) overview and register reference

# DESCRIPTION
The Secondary Memory Interface (SMI) provides a parallel interface for
high-speed data transfers between the system and external peripherals
such as SRAM or display devices.

It supports programmed transfers and DMA-based transfers using FIFO
buffers and configurable timing parameters.

# REGISTERS

## Control and Status Register
Controls transfers and reports device status.

Key fields include:

- ENABLE  
  Enables the SMI interface.

- START  
  Begins a transfer. Auto-clearing.

- DONE  
  Indicates transfer completion.

- ACTIVE  
  Indicates an ongoing transfer.

- WRITE  
  Selects transfer direction (0 = read, 1 = write).

- CLEAR  
  Resets FIFOs.

- TXW / RXR  
  FIFO status indicators.

- AFERR  
  FIFO access error flag.

## Length Register
Specifies the number of words in a transfer.

- Write: sets transfer length  
- Read: returns progress during transfer

## Address Register
Defines the target address and device configuration.

- DEVICE: selects device configuration
- ADDR: bus address

## Data Register
Interface to transmit and receive FIFOs.

- Write: pushes to TX FIFO  
- Read: pops from RX FIFO

## DMA Control Register
Controls DMA request and panic thresholds.

- DMAEN: enable DMA signalling  
- REQR / REQW: DREQ thresholds  
- PANICR / PANICW: panic thresholds  

# DMA USAGE

The SMI supports DMA transfers using external buffers such as
u-dma-buf.

Non-cacheable memory must be used to ensure correct operation.

# U-DMA-BUF

u-dma-buf provides physically contiguous memory suitable for DMA.

Example:

    sudo insmod u-dma-buf.ko udmabuf0=1048576

## Cache Coherency (64-bit systems)

Manual cache management is required.

Do not open buffers with O_SYNC.

Use:

    /sys/class/u-dma-buf/udmabuf0/sync_for_cpu
    /sys/class/u-dma-buf/udmabuf0/sync_for_device

# NOTES
Large transfers may require careful buffer management due to
coherency limitations.

# SEE ALSO
dma(7)