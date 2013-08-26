#include <linux/module.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "dragon.h"

#define DEVCNT 1
#define DEVNAME "Dragon"

#define VendorID 0x0100
#define DeviceID 0x0001

static int iDragon_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static void iDragon_pci_remove(struct pci_dev *pdev);
static int iDragon_open(struct inode *inode, struct file *file);
static ssize_t iDragon_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t iDragon_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);


static LIST_HEAD(dragon_instance_list);

struct idragon_dev_struct {
  struct list_head link; /* Double linked list */
  struct pci_dev *dev;
  struct cdev cdev;
  int mode; // IO=0 or MEM=1
  int minor; /* Minor number */
  unsigned int iobase;
  unsigned int *membase;
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
irqreturn_t iDragon_irq_handler(int irq, void *dev_id)
#else
irqreturn_t iDragon_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
  struct idragon_dev_struct *data = (struct idragon_dev_struct *)dev_id;

  printk(KERN_INFO "iDragon_irq_handler: interrupt from device %d\n", data->minor);

  return IRQ_HANDLED;
}

static int iDragon_open(struct inode *inode, struct file *file)
{
  int minor = MINOR(inode->i_rdev);
  struct list_head *cur;
  struct idragon_dev_struct *data;

  printk(KERN_INFO "iDragon_open!\n");
  list_for_each(cur, &dragon_instance_list) {
    data = list_entry(cur, struct idragon_dev_struct, link);
    if (data->minor == minor) {
      file->private_data = data;
      return 0;
    }
  }

  printk(KERN_WARNING "iDragon_open: minor %d not found\n", minor);

  return -ENODEV;
}

static int iDragon_release(struct inode *inode, struct file *file) {
  printk("iDragon_release!\n");
  file->private_data = NULL;
  return 0;
}

static ssize_t iDragon_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
  int ret;
  struct idragon_dev_struct *data = file->private_data;
  unsigned int port;
  unsigned int value;

  /* Make sure our user wasn't bad... */
  if (!buf || len!=sizeof(unsigned int)) {
    ret = -EINVAL;
    goto out;
  }

  if(data->mode == MODE_IO) {
    port = data->iobase;
    port = port + 0x8;
    value = inl(port);
    if (copy_to_user(buf, &value, sizeof(unsigned int))) {
      ret = -EFAULT;
      goto out;
    }

    /* Good to go, so printk the thingy */
    printk(KERN_INFO "iDragon_read: User got from us 0x%08x\n", value);
  } else {
    if (copy_to_user(buf, data->membase + 2, sizeof(unsigned int))) {
      ret = -EFAULT;
      goto out;
    }

    /* Good to go, so printk the thingy */
    printk(KERN_INFO "iDragon_read: User got from us 0x%08x\n", data->membase[2]);
  }
  ret = sizeof(unsigned int);

 out:
  return ret;
}

static ssize_t iDragon_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
  int ret;
  struct idragon_dev_struct *data = file->private_data;
  unsigned int port;
  unsigned int value;

  /* Make sure our user wasn't bad... */
  if (!buf || len!=sizeof(unsigned int)) {
    ret = -EINVAL;
    goto out;
  }

  if (copy_from_user(&value, buf, len)) {
    ret = -EFAULT;
    goto out;
  }

  if(data->mode == MODE_IO) {
    port = data->iobase;
    port = port + 0x8;
    outl(value, port);

    /* print what userspace gave us */
    printk(KERN_INFO "iDragon_write: Userspace wrote 0x%08x\n", value);
  } else {
    data->membase[2] = value;

    /* print what userspace gave us */
    printk(KERN_INFO "iDragon_write: Userspace wrote 0x%08x\n", data->membase[2]);
  }

  ret = len;

 out:
  return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int iDragon_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long iDragon_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
  struct idragon_dev_struct *data = file->private_data;
  unsigned int port;
  unsigned int value;

    switch (cmd)
    {
        case IOCTL_GET_MODE:
            if (copy_to_user((unsigned int *)arg, &data->mode, sizeof(unsigned int)))
            {
                return -EACCES;
            }
            break;
        case IOCTL_SET_MODE:
            if (copy_from_user(&data->mode, (unsigned int *)arg, sizeof(unsigned int))) {
                return -EACCES;
            }
            printk(KERN_INFO "iDragon_ioctl: Set mode to %d\n", data->mode);
            break;
        case IOCTL_GET_ID:
            if(data->mode == MODE_IO) {
                port = data->iobase;
                value = inl(port);
                if (copy_to_user((unsigned int *)arg, &value, sizeof(unsigned int))) {
                    return -EACCES;
                }
            } else {
                if (copy_to_user((unsigned int *)arg, data->membase, sizeof(unsigned int))) {
                    return -EACCES;
                }
            }
            break;
        case IOCTL_GET_LED:
            if(data->mode == MODE_IO) {
                port = data->iobase;
                value = inl(port + 0x4);
                if (copy_to_user((unsigned int *)arg, &value, sizeof(unsigned int))) {
                    return -EACCES;
                }
            } else {
                if (copy_to_user((unsigned int *)arg, data->membase + 1, sizeof(unsigned int))) {
                    return -EACCES;
                }
            }
            break;
        case IOCTL_SET_LED:
            if(data->mode == MODE_IO) {
                port = data->iobase;
                if (copy_from_user(&value, (unsigned int *)arg, sizeof(unsigned int))) {
                    return -EACCES;
                }
                outl(value, port + 0x4);
            } else {
                if (copy_from_user(&value, (unsigned int *)arg, sizeof(unsigned int))) {
                    return -EACCES;
                }
                data->membase[1] = value;
            }
            break;
        case IOCTL_GET_STATUS:
            if(data->mode == MODE_IO) {
                port = data->iobase;
                value = inl(port + 0x8);
                if (copy_to_user((unsigned int *)arg, &value, sizeof(unsigned int))) {
                    return -EACCES;
                }
            } else {
                if (copy_to_user((unsigned int *)arg, data->membase + 2, sizeof(unsigned int))) {
                    return -EACCES;
                }
            }
            break;
        case IOCTL_SET_STATUS:
            if(data->mode == MODE_IO) {
                port = data->iobase;
                if (copy_from_user(&value, (unsigned int *)arg, sizeof(unsigned int))) {
                    return -EACCES;
                }
                outl(value, port + 0x8);
            } else {
                if (copy_from_user(&value, (unsigned int *)arg, sizeof(unsigned int))) {
                    return -EACCES;
                }
                data->membase[2] = value;
            }
            break;
        case IOCTL_GET_SWITCH:
            if(data->mode == MODE_IO) {
                port = data->iobase;
                value = inl(port + 0xC);
                if (copy_to_user((unsigned int *)arg, &value, sizeof(unsigned int))) {
                    return -EACCES;
                }
            } else {
                if (copy_to_user((unsigned int *)arg, data->membase + 3, sizeof(unsigned int))) {
                    return -EACCES;
                }
            }
            break;
        default:
            return -EINVAL;
    }
 
    return 0;
}

/* File operations for our device */
static struct file_operations idragon_fops = {
  .owner = THIS_MODULE,
  .open = iDragon_open,
  .release = iDragon_release,
  .read = iDragon_read,
  .write = iDragon_write,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
  .ioctl = iDragon_ioctl
#else
  .unlocked_ioctl = iDragon_ioctl
#endif
};

static dev_t idragon_node;

static int __devinit iDragon_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
  int i, ret = 0;
  struct idragon_dev_struct *data;
  static int minor = 0;
  u8 intpin;

  printk(KERN_INFO "iDragon_pci_probe: found 0x%x:0x%x\n", ent->vendor, ent->device);

  data = (struct idragon_dev_struct *)kmalloc(sizeof(struct idragon_dev_struct), GFP_KERNEL);
  if(data == NULL) {
    printk(KERN_INFO "iDragon_pci_probe: unable to allocate private data structure\n");
    ret = -ENOMEM;
    goto cleanup_kmalloc;
  }

  pci_set_drvdata(pdev, data);

  data->dev = pdev;
  data->mode = MODE_MEM;
  data->minor = minor++;

#if 0
  pci_request_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM), "iDragon_pci");
  hw_addr = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
  //printk(KERN_INFO "iDragon_pci_probe: 0x%08x\n", readl(hw_addr));
#else
  ret = pci_enable_device(pdev);
  if(ret < 0) {
    printk(KERN_WARNING "iDragon_pci_probe: unable to initialize PCI device\n");
    goto cleanup_pci_enable;
  }

  ret = pci_request_regions(pdev, "Dragon");
  if(ret < 0) {
    printk(KERN_WARNING "iDragon_pci_probe: unable to reserve PCI resources\n");
    goto cleanup_regions;
  }

  for(i=0; i<DEVICE_COUNT_RESOURCE; i++) {
    if(pci_resource_len(pdev, i) == 0) {
      continue;
    }
    if(pci_resource_start(pdev, i) == 0) {
      continue;
    }
    printk(KERN_INFO "iDragon_pci_probe: BAR%d (0x%08x-0x%08x), len=%d, flags=0x%08x\n", i, (u32)pci_resource_start(pdev, i), (u32)pci_resource_end(pdev, i), (u32)pci_resource_len(pdev, i), (u32)pci_resource_flags(pdev, i));
    if(pci_resource_flags(pdev, i) & IORESOURCE_IO) {
      printk(KERN_INFO "iDragon_pci_probe: IO space\n");
      data->iobase = pci_resource_start(pdev, i);
    } else if(pci_resource_flags(pdev, i) & IORESOURCE_MEM) {
      printk(KERN_INFO "iDragon_pci_probe: MEM space\n");
      if (pci_resource_flags(pdev, i) & IORESOURCE_CACHEABLE) {
	printk (KERN_INFO "iDragon_pci_probe: cacheable ! \n");
	data->membase = ioremap(pci_resource_start(pdev, i), pci_resource_len(pdev, i));
      }
      else {
	printk (KERN_INFO "iDragon_pci_probe: NOT cacheable ! \n");
	data->membase = ioremap_nocache(pci_resource_start(pdev, i), pci_resource_len(pdev, i));
      }
      if(data->membase == NULL) {
	printk (KERN_INFO "iDragon_pci_probe: unable to remap I/O memory\n");
	ret = -ENOMEM;
	goto cleanup_ioremap;
      }
      printk(KERN_INFO "iDragon_pci_probe: I/O memory has been remaped at %#08x\n", (u32)data->membase);
    }
  }

  pci_read_config_byte(pdev, PCI_INTERRUPT_PIN, &intpin);
  if(intpin) {
    ret = request_irq(pdev->irq, iDragon_irq_handler, IRQF_SHARED, "Dragon", data);
    if(ret < 0) {
      printk(KERN_WARNING "iDragon_pci_probe: unable to register irq handler\n");
      goto cleanup_irq;
    }
  } else {
    printk(KERN_INFO "iDragon_pci_probe: no IRQ!\n");
  }
#endif

  if (alloc_chrdev_region(&idragon_node, 0, DEVCNT, DEVNAME)) {
    printk(KERN_WARNING "iDragon_pci_probe: alloc_chrdev_region() failed!\n");
    ret = -1;
    goto cleanup_alloc_chrdev;
  }

  printk(KERN_INFO "iDragon_pci_probe: Allocated %d devices at major: %d\n", DEVCNT, MAJOR(idragon_node));

  /* Initialize the character device and add it to the kernel */
  cdev_init(&data->cdev, &idragon_fops);
  data->cdev.owner = THIS_MODULE;

  if (cdev_add(&data->cdev, idragon_node, DEVCNT)) {
    printk(KERN_ERR "iDragon_pci_probe: cdev_add() failed!\n");
    ret = -1;
    goto cleanup_cdev;
  }

  list_add_tail(&data->link, &dragon_instance_list);

  return 0;

cleanup_cdev:
  /* clean up chrdev allocation */
  unregister_chrdev_region(idragon_node, DEVCNT);
cleanup_alloc_chrdev:
  free_irq(pdev->irq, data);
cleanup_irq:
  iounmap(data->membase);
cleanup_ioremap:
  pci_release_regions(pdev);
cleanup_regions:
  pci_disable_device(pdev);
cleanup_pci_enable:
  kfree(data);
cleanup_kmalloc:
  return ret;
}

static void __devexit iDragon_pci_remove(struct pci_dev *pdev)
{
  struct idragon_dev_struct *data = pci_get_drvdata(pdev);
  u8 intpin;

  /* destroy the cdev */
  cdev_del(&data->cdev);
  unregister_chrdev_region(idragon_node, DEVCNT);

#if 0
  iounmap(hw_addr);
  hw_addr = NULL;
  pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
#else
  iounmap(data->membase);

  pci_release_regions(pdev);
  pci_disable_device(pdev);

  pci_read_config_byte(pdev, PCI_INTERRUPT_PIN, &intpin);
  if(intpin) {
    free_irq(pdev->irq, data);
  }
#endif

  list_del(&data->link);

  kfree(data);

  printk(KERN_INFO "iDragon_pci_remove!\n");
  return;
}

/* device ID table */
static struct pci_device_id idragon_pci_table[] = {
  { PCI_DEVICE(VendorID, DeviceID) },
  { } /* terminating entry - this is required */
};
MODULE_DEVICE_TABLE(pci, idragon_pci_table);

static struct pci_driver idragon_pci_driver = {
  .name = "Dragon",
  .id_table = idragon_pci_table,
  .probe = iDragon_pci_probe,
  .remove = iDragon_pci_remove,
};

static int iDragon_init_module(void) {
  printk(KERN_INFO "iDragon: init\n");
  return pci_register_driver(&idragon_pci_driver);
}

void iDragon_cleanup_module(void) {
  printk(KERN_INFO "iDragon: cleanup\n");
  pci_unregister_driver(&idragon_pci_driver);
}


MODULE_AUTHOR("Derek Qian");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

module_init(iDragon_init_module);
module_exit(iDragon_cleanup_module);
