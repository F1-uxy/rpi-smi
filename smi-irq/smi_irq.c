#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/atomic.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>

#define IRQ_NO 90

#define PI_23_REG_BASE 0x3F000000u
#define SMI_BASE_REG_OFFSET 0x600000
#define SMI_SIZE 0x100
#define SMIO_CS 0x00
#define SMIO_CS_DONE (0x1 << 1)

static int dev = 0xaa, counter = 0;

static dev_t dev_num;
static struct cdev smi_cdev;
static struct class* smi_class;

static atomic_t irq_count = ATOMIC_INIT(0);
static wait_queue_head_t irq_waitq;

void __iomem* smi_base;

static irqreturn_t smi_handler(int irq, void* dev)
{
    u32 cs_reg = readl(smi_base + SMIO_CS);

    if(cs_reg & SMIO_CS_DONE)
    {
        writel(cs_reg & SMIO_CS_DONE, smi_base + SMIO_CS);
        atomic_inc(&irq_count);
        wake_up_interruptible(&irq_waitq);
    }

    return IRQ_HANDLED;
}

static __poll_t smi_poll(struct file* f, poll_table* wait)
{
    __poll_t mask = 0;

    poll_wait(f, &irq_waitq, wait);

    if(atomic_read(&irq_count) > 0) mask |= POLLIN | POLLRDNORM;

    return mask;
}

static ssize_t smi_read(struct file* f, char __user* buf, size_t count, loff_t* ppos)
{
    if(wait_event_interruptible(irq_waitq, atomic_read(&irq_count) > 0)) return -ERESTARTSYS;

    atomic_dec(&irq_count);

    return 0;
}

static const struct file_operations my_fops = 
{
    .owner = THIS_MODULE,
    .poll = smi_poll,
    .read = smi_read
};

static inline void destroy_device(void)
{
    device_destroy(smi_class, dev_num);
    class_destroy(smi_class);
}

static inline void del_cdev(void)
{
    cdev_del(&smi_cdev);
}

static inline void unregister_chrdevregion(void)
{
    unregister_chrdev_region(dev_num, 1);
}

static int smi_interrupt_init(void)
{
    int ret = 0;
    pr_info("%s: SMI IRQ init\n", __func__);

    smi_base = ioremap(PI_23_REG_BASE + SMI_BASE_REG_OFFSET, SMI_SIZE);
    if(!smi_base) return -ENOMEM;

    init_waitqueue_head(&irq_waitq);

    ret = alloc_chrdev_region(&dev_num, 0, 1, "smi_irq");
    if (ret) return ret;

    cdev_init(&smi_cdev, &my_fops);
    smi_cdev.owner = THIS_MODULE;

    ret = cdev_add(&smi_cdev, dev_num, 1);
    if (ret)
    {
        unregister_chrdevregion();
        return ret;
    }

    smi_class = class_create("smi_class");
    if(IS_ERR(smi_class))
    {
        ret = PTR_ERR(smi_class);
        del_cdev();
        unregister_chrdevregion();
        return ret;
    }

    struct device *dev_ptr = device_create(smi_class, NULL, dev_num, NULL, "smi_irq");
    if(IS_ERR(dev_ptr))
    {
        ret = PTR_ERR(dev_ptr);
        class_destroy(smi_class);
        del_cdev();
        unregister_chrdevregion();
        return ret;
    }

    ret = request_irq(IRQ_NO, smi_handler, 0, "smi_handler", &dev);

    return 0;
}

static void smi_interrupt_exit(void)
{
    pr_info("%s: SMI IRQ exit\n", __func__);

    synchronize_irq(IRQ_NO);
    free_irq(IRQ_NO, &dev);

    device_destroy(smi_class, dev_num);
    class_destroy(smi_class);

    cdev_del(&smi_cdev);
    unregister_chrdev_region(dev_num, 1);
}

module_init(smi_interrupt_init);
module_exit(smi_interrupt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Charlie Flux");
MODULE_DESCRIPTION("BCM2835 SMI Interrupt Handler");