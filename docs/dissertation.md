### Topics:

#### Current Documentation:
<ul>
    <li> Current kernel driver is lackluster. Only supports 8 & 16 bit modes. Only uses 1 device bank. Doesn't implement specific packing modes or provide unpacking functionality.
</ul>


#### API Design
<ul>
    <li> Defensive API vs Contract Based API
    <li> Context structs
    <li> Fast word extraction - branchless hot path - LUT vs branches
</ul>

#### SMI Modes:
<ul>
    <li> Tear Effect - R/W is triggered on the tear effect trigger. Enabling this bricks the system
    <li> Important to pass the correct size data type for a given bus width
    <li> RGB565 & XRGB 18 bit swap - reversed bit order, unknown how to unpack
    <li> Why is 248 the limit to programmed read block size
    <li> Automatically use DMA over a certain transfer size like the kernel
    <li> How can I use the interrupts? Kernel module? User space? ; Attempted to catch IRQ 48 using INTD, INTT & INTR but couldn't make it work.
    <li> RXD & TXD are swapped in the documentation.
</ul>

RXF -> TXE
TXE -> RXF
RXD -> TXD = When TXD = 0, TX FIFO is full and transfer direction is set to write
TXD -> RXD
RXR -> TXW
TXW -> RXR

#### Benchmarking
The benchmark enviornment:
<ul>
    <li> GCC -03 optimisation
    <li> -march=native to enable architecture specific functionality
</ul>

#### Good Articles:
[LUT vs Branches](https://specbranch.com/posts/lookup-tables/)
[Raspberry Pi Memory Heirachy](https://sandsoftwaresound.net/raspberry-pi/raspberry-pi-gen-1/memory-hierarchy/)