### Secondary Memory Interface

#### Control & Status Register
The status and control register controls non-direct transfers and monitors peripheral status.

| Field  | Description                                                                                                                                                                                                                                                                                                                                    | Access |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------ |
| ENABLE | Enable the SMI. Value is OR'd with the DCS register. When disabled, the SMI enters low power mode, however R/W access is still available                                                                                                                                                                                                       | R/W    |
| DONE   | Current transfer status. Set when the final word has been recieved or written for a given transfer                                                                                                                                                                                                                                             | R/W    |
| ACTIVE | Indicates the current transfer status                                                                                                                                                                                                                                                                                                          | R      |
| START  | Start Transfer. Auto clearing bit and will always be read as 0, writing 1 while active has no effect                                                                                                                                                                                                                                           | W      |
| CLEAR  | Clear the FIFOs. FIFOs will be reset to the empty state. Auto clearing bit and will always be read as 0                                                                                                                                                                                                                                        | W      |
| WRITE  | Set the transfer direction. 0 = Read ; 1 = Write                                                                                                                                                                                                                                                                                               | R/W    |
| PAD    | Padding Words. The number of FIFO words to be discarded at the start of a transfer. For writes, value indicates the number of words that will be taken from the TX FIFO but not transferred. For reads, value indicate number of words that will be received from the peripheral, after packing, but will not be writted into the RX FIFO. 0-4 | R/W    |
| TEEN   | Tear Effect Mode. Programmed transfers will wait for a Tear Effect trigger before starting                                                                                                                                                                                                                                                     | R/W    |
| INTD   | Interrupt while DONE = 1                                                                                                                                                                                                                                                                                                                       | R/W    |
| INTT   | Interrupt while TXW = 1                                                                                                                                                                                                                                                                                                                        | R/W    |
| INTR   | Interrupt while RXR = 1                                                                                                                                                                                                                                                                                                                        | R/W    |
| PVMODE | Pixel Value Mode. Transmit data is taken from the pixel value interface rather than the AXI input.                                                                                                                                                                                                                                             |        |
| SETERR | Setup error has occured. Setup registers were written to when enabled. Can be cleared by writing 1                                                                                                                                                                                                                                             | R/W    |
| PXLDAT | Enable pixel formatting modes. Data in the FIFO's will be packed to match the pixel format selected                                                                                                                                                                                                                                            | R/W    |
| EDREQ  | External DREQ received. Indicates the status of the external devices DREQ when in DMAP mode                                                                                                                                                                                                                                                    | R      |
| PRDY   | Appear not ready on the AXI bus if the appropriate FIFO is not ready. SMI will stall the AXI bus when reading or writing data to SMI_D unless there is room in the FIFO for writes or data available for a read. AXI may become locked or seriously impact system performance                                                                  | R/W    |
| AFERR  | AXI FIFO error has occured. Either a read of the RFIFO when empty or a write of the WFIFO when full. Can be cleared by writing 1                                                                                                                                                                                                               | R/W    |
| TXW    | TX FIFO need writing. 0 = TX FIFO is at least 1/4 full or the transfer direction is set to READ. 1 = TX FIFO is less than 1/4 full and the transfer direction is set to WRITE                                                                                                                                                                  | R      |
| RXR    | RX FIFO needs reading. 0 = RX FIFO is less than 3/4 full or when DONE and FIFO no empty. 1 = RX FIFO is at least 3/4 full or the transfer has finished and the FIFO still need reading. Transfer direction must be set to READ                                                                                                                 | R      |
| RXD    | TX FIFO can accept data. 0 = TX FIFO is cannot accept new data or the transfer direction is set to READ. 1 = TX FIFO can accept at least 1 word of data and the transfer direction is set to WRITE                                                                                                                                             | R      |
| RXD    | RX FIFO contains data. 0 = RX FIFO contains no data or the transfer direction is set to WRITE. 1 = RX FIFO contains at least 1 word of data that can be read and the transfer direction is set to READ                                                                                                                                         | R      |
| TXE    | RX FIFO is empty.                                                                                                                                                                                                                                                                                                                              | R      |
| RXF    | RX FIFO is full.                                                                                                                                                                                                                                                                                                                               | R      |


#### SMI Transfer length Register
The SMI length register is used to specify the number of transfers on the SMI bus in words. The word width is configured by the DSR & DSW registers. During a transfer the register reads as the number of words transferred so far.

| Field  | Width | Description                                                                                                                                | Access |
| ------ | ----- | ------------------------------------------------------------------------------------------------------------------------------------------ | ------ |
| Length | 31:0  | Write: Sets the number of words to transfer over the external bus. Read: During a transfer contains the number of words transferred so far | R/W    |


#### SMI Address Register Defintion
The SMI address register is used to specify the address for a given transfer. Device settings can be selected using this register.

| Field  | Width | Description                                                         | Access |
| ------ | ----- | ------------------------------------------------------------------- | ------ |
| ADDR   | 5:0   | Address to be used for transfers and presented on the SMI bus       | R/W    |
| DEVICE | 1:0   | Which set of device settings should be used for this transfer (0-4) | R/W    |


#### SMI Data Register
The SMI data register is used to interface with the transmit FIFOs. Data written to the SMI_D register is placed in the transmit FIFO. Data read from the SMI_D register is taken from the recieve FIFO.

| Field | Width | Description                                                                  | Access |
| ----- | ----- | ---------------------------------------------------------------------------- | ------ |
| DATA  | 31:0  | Reading returns the top value of RX FIFO. Writing writes to back of TX FIFO. | R/W    |


### u-dma-buf:
u-dma-buf is used to allocate non-cacheable memory which is required for a DMA transfer to complete.

To allocate a 1KB region use in your local u-dma-buf clone:
`sudo insmod u-dma-buf.ko udmabuf0=1048576`

#### 64-bit u-dma-buf:
To use u-dma-buf with a 64-bit operating system we must maintain our own cache coherancy.

u-dma-buf provides some tooling to help with this:

When opening the u-dma-buf buffer file do not use the O_SYNC flag

u-dma-buf also provides a quirk-mmap which is an altered version of mmap that lets us manually maintain cache coherancy. This is enabled when you create the buffer, for example:
```
sudo insmod u-dma-buf.ko udmabuf0=1048576 quirk_mmap_mode=2
```

We must also make use of two files that we write to signal an incoming read or write:
```
int fd_sync_cpu = open("/sys/class/u-dma-buf/udmabuf0/sync_for_cpu", O_WRONLY);
int fd_sync_dev = open("/sys/class/u-dma-buf/udmabuf0/sync_for_device", O_WRONLY);
```

When we write to the DMA buffer:
```
write(fd_sync_dev, "1", 1);
```

On the reverse, when we read:
```
write(fd_sync_cpu, "1", 1);
```

### Useful links:
<ul>
    <li> https://github.com/ikwzm/udmabuf?tab=readme-ov-file
    <li> https://iosoft.blog/2020/07/16/raspberry-pi-smi/
</ul>