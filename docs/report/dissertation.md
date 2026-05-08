### Topics:

#### Midway Feelings:
<ul>
    <li> Implementation section could do with more anecdotal reasoning to why something was chose and why something wasn't chosen
    <li> Redo the bullet point section into a paragraph and talk more about why I chose the tiered system.
    <li> Expand on the lookup table section
    <li> Write more about why instead of just what
    <li> Implement optional heap use instead of forced heap use + resizing.
    <li> Buffer vs stack transfer benchmark
    <li> Map inputs to 18 bit packed
    <li> Actually review the literature 
    <li> Could I put the documentation into a MAN page??
    <li> DMA read not working for read length of 1
    <li> All width without packing benchmark
    <li> Why is STD DEV so high on small DMA packets?
    <li> perf L2 misses, DRAM R/W Bus contention, thermal throttling, calculate l1 vs l2 cache latency and if the fifo would then saturate
    <li> Properly benchmark checking the waveforms
</ul>

Required diagrams:
- Address translation
- Read/Write function flowchart
- API level design
- Await functions
- 

There are also direct pass modes which take the lowest 8/9/16/18-bits of the input word and output this
to the external data bus. To enable these modes the PXLDAT bit of the SMI_CS register must be
cleared. In this case the FORMAT and SWAP bits are ignored.

Why is DMA failing:
- Consistent amount of successful transfers
- Multiple blocks of smaller size don't fix the problem
- Length register shows all read transfers were successful
- FIFO debug shows the FIFO is full? but doesn't match the number of missing samples
- Not noise on data lines as values are larger than maximum possible by 8 bits
- Current suspect is paging on udmabuf where memory is not continuous and is cross the page boundary

Questions to add depth:
- Why does throughput behave differently between the 3B+ and Pi 4? Relate it back to the BCM283x architecture differences you already understand well.
- Why does chunking improve performance at certain transfer sizes but not others? Connect it to FIFO saturation behaviour you described in the implementation.
- Where does DMA start outperforming programmed transfers, and does that threshold match your theoretical expectation? If not, why not?
- Methodology section currently reads slightly more like "what I did" than "why I did it in this order." A sentence or two explicitly framing the iterative, prototype-first approach as a deliberate response to the documentation gap (rather than just a preference) would sharpen it.
- You've cited Bentham and CaribouLite in the background — can you put any of your throughput numbers in context against theirs, or against the theoretical maximum of the SMI bus? Even an order-of-magnitude comparison with a brief explanation of why direct comparison is difficult (different use cases, application-specific implementations) would push the analysis section significantly higher.

#### Current Documentation:
<ul>
    <li> Current kernel driver is lackluster. Only supports 8 & 16 bit modes. Only uses 1 device bank. Doesn't implement specific packing modes or provide unpacking functionality.
</ul>


#### API Design
<ul>
    <li> Defensive API vs Contract Based API
    <li> Context structs
    <li> Fast word extraction - branchless hot path - LUT vs branches
    <li> aligned(32) vs aligned(4)
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