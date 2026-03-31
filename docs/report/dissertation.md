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
    <li> SMI IRQ kernel module, how to await on the done.
    <li> RXD & TXD are swapped in the documentation.
    <li> DMA can't keep up and drops words on raspberry pi 3 & 4
    <li> Multi core bus contentions vs single core  
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
    <li> Each test had 1 warmup & 5 recorded runs
</ul>

#### Good Articles:
[LUT vs Branches](https://specbranch.com/posts/lookup-tables/)
[Raspberry Pi Memory Heirachy](https://sandsoftwaresound.net/raspberry-pi/raspberry-pi-gen-1/memory-hierarchy/)
[SMI Initial Forum](https://forums.raspberrypi.com/viewtopic.php?t=280242)

A priori

#### Final Report Plan:

##### Abstract

##### Introduction
<ul>
    <li> The move away from parallel interfaces and the silent deprecation of support for old systems
    <li> Overview of the project
    <li> Aims and objectives
</ul>

##### Background
<ul>
    <li> Secondary Memory Interface
    <li> Direct Memory Access
    <li> Shared Memory Architectures
    <li> Kernel vs user space
    <li> Motivation of the project
    <li> Current implemenations (literature review) and limitations
</ul>

##### Methodology
<ul>
    <li> API design (style and functionality)
    <li> Corrections of current documentation
    <li> Bus contentions for multi vs single core
</ul>


##### Results and Analysis

##### Conclusion and Future Work