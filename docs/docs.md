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
sudo insmod u_dma_buf.ko udmabuf0=1048576 quirk_mmap_mode=2
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