### u-dma-buf:
u-dma-buf is used to allocate non-cacheable memory which is required for a DMA transfer to complete.

To allocate a 1KB region use in your local u-dma-buf clone:
`sudo insmod u-dma-buf.ko udmabuf0=1048576`


### Useful links:
<ul>
    <li> https://github.com/ikwzm/udmabuf?tab=readme-ov-file
    <li> https://iosoft.blog/2020/07/16/raspberry-pi-smi/
</ul>