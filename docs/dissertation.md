### Topics:

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
</ul>


#### Good Articles:

[LUT vs Branches](https://specbranch.com/posts/lookup-tables/)
[Raspberry Pi Memory Heirachy](https://sandsoftwaresound.net/raspberry-pi/raspberry-pi-gen-1/memory-hierarchy/)