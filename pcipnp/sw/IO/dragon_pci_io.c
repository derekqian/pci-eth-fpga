/*
 * Linux driver for KNJN Dragon PCI (PCI_PnP.100K.bit, IORESOURCE_IO)
 *
 * Pierre Ficheux (pierre.ficheux@openwide.fr)
 *
 * 3/2011
 */

#include <linux/kernel.h>	/* printk() */
#include <linux/module.h>	/* modules */
#include <linux/init.h>		/* module_{init,exit}() */
#include <linux/slab.h>		/* kmalloc()/kfree() */
#include <linux/pci.h>		/* pci_*() */
#include <linux/pci_ids.h>	/* pci idents */
#include <linux/list.h>		/* list_*() */
#include <asm/uaccess.h>	/* copy_{from,to}_user() */
#include <linux/fs.h>		/* file_operations */
#include <linux/interrupt.h>	/* request_irq etc */
#include <linux/version.h>

MODULE_DESCRIPTION("dragon_pci_io");
MODULE_AUTHOR("Pierre Ficheux");
MODULE_LICENSE("GPL");

#define PCI_PNP_RAM_SIZE 64             /* RAM size (bytes) */

/*
 * Arguments
 */
static int major = 0; /* Major number */
module_param(major, int, 0660);
MODULE_PARM_DESC(major, "Static major number (none = dynamic)");

static int debug = 0; 
module_param(debug, int, 0660);
MODULE_PARM_DESC(major, "Debug flag (1 = YES, 0 = NO)");

/*
 * Supported devices
 */
static struct pci_device_id dragon_pci_io_id_table[] __devinitdata = {
  {0x0100, 0x0000, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
  {0,}	/* 0 terminated list */
};
MODULE_DEVICE_TABLE(pci, dragon_pci_io_id_table);

/*
 * Global variables
 */
static LIST_HEAD(dragon_pci_io_list);

struct dragon_pci_io_struct {
  struct list_head	link; /* Double linked list */
  struct pci_dev		*dev; /* PCI device */
  int			minor; /* Minor number */
  unsigned int		iobase[DEVICE_COUNT_RESOURCE];
  u32			iolen[DEVICE_COUNT_RESOURCE];
};

/*
 * Event handlers
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
  irqreturn_t dragon_pci_io_irq_handler(int irq, void *dev_id)
#else
  irqreturn_t dragon_pci_io_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
  struct dragon_pci_io_struct *data = (struct dragon_pci_io_struct *)dev_id;

  printk(KERN_INFO "dragon_pci_io: interrupt from device %d\n", data->minor);

  return IRQ_HANDLED;
}

/*
 * File operations
 */
static ssize_t dragon_pci_io_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
  int i, j, real, bank = DEVICE_COUNT_RESOURCE;
  struct dragon_pci_io_struct *data = file->private_data;
  unsigned char kbuf[PCI_PNP_RAM_SIZE];
  unsigned int port;

  /* Find the first remapped I/O memory bank to read */
  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
    if (data->iobase[i] != 0) {
      bank = i;
      break;
    }
  }

  /* No bank found */
  if (bank == DEVICE_COUNT_RESOURCE) {
    printk(KERN_INFO "dragon_pci_io: no I/O memory bank to read\n");
    return -ENXIO;
  }

  /* Check for overflow */
  real = min(data->iolen[bank] - (int)*ppos, count);

  /* Copy data from board */
  if (real) {
    port = data->iobase[i] + *ppos;
    for (j = 0; j < real; j += sizeof(long))
      *((unsigned long *)(kbuf + j)) = inl(port + j);

    if (copy_to_user(buf, kbuf, real))
      return -EFAULT;
  }

  if (debug)
    printk(KERN_INFO "dragon_pci_io: read %d/%d chars at offset %d from I/O memory bank %d\n", real, count, (int)*ppos, bank);

  *ppos += real;

  return real;
}

static ssize_t dragon_pci_io_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
  int i, j, real, bank = DEVICE_COUNT_RESOURCE;
  struct dragon_pci_io_struct *data = file->private_data;
  unsigned char kbuf[PCI_PNP_RAM_SIZE];
  unsigned int port;

  /* Find the first remapped I/O memory bank to read */
  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
    if (data->iobase[i] != 0) {
      bank = i;
      break;
    }
  }

  /* No bank found */
  if (bank == DEVICE_COUNT_RESOURCE) {
    printk(KERN_INFO "dragon_pci_io: no I/O memory bank to read\n");
    return -ENXIO;
  }

  /* Check for overflow */
  real = min(data->iolen[bank] - (int)*ppos, count);

  if (real) {
    if (copy_from_user(kbuf, buf, real))
      return -EFAULT;

    port = data->iobase[i] + *ppos;

    for (j = 0 ; j < real ; j += sizeof(long))
      outl(*((unsigned long *)(kbuf + j)), port + j);

  }

  if (debug)
    printk(KERN_INFO "dragon_pci_io: wrote %d/%d chars at offset %d to I/O memory bank %d\n", real, count, (int)*ppos, bank);

  *ppos += real;

  return real;
}

static int dragon_pci_io_open(struct inode *inode, struct file *file)
{
  int minor = MINOR(inode->i_rdev);
  struct list_head *cur;
  struct dragon_pci_io_struct *data;

  if (debug)
    printk("dragon_pci_io_open()\n");

  list_for_each(cur, &dragon_pci_io_list) {
    data = list_entry(cur, struct dragon_pci_io_struct, link);

    if (data->minor == minor) {
      file->private_data = data;

      return 0;
    }
  }

  printk(KERN_WARNING "dragon_pci_io: minor %d not found\n", minor);

  return -ENODEV;
}

static int dragon_pci_io_release(struct inode *inode, struct file *file)
{
  if (debug)
    printk("dragon_pci_io_release()\n");

  file->private_data = NULL;

  return 0;
}

static struct file_operations dragon_pci_io_fops = {
  .owner =	THIS_MODULE,
  .read =	dragon_pci_io_read,
  .write =	dragon_pci_io_write,
  .open =	dragon_pci_io_open,
  .release =	dragon_pci_io_release,
};

/*
 * PCI handling
 */
static int __devinit dragon_pci_io_probe(struct pci_dev *dev, const struct pci_device_id *ent)
{
  int i, ret = 0;
  struct dragon_pci_io_struct *data;
  static int minor = 0;
  u8 mypin;

  printk(KERN_INFO "dragon_pci_io: found %x:%x\n", ent->vendor, ent->device);
  printk(KERN_INFO "dragon_pci_io: using major %d and minor %d for this device\n", major, minor);

  /* Allocate a private structure and reference it as driver's data */
  data = (struct dragon_pci_io_struct *)kmalloc(sizeof(struct dragon_pci_io_struct), GFP_KERNEL);
  if (data == NULL) {
    printk(KERN_INFO "dragon_pci_io: unable to allocate private structure\n");

    ret = -ENOMEM;
    goto cleanup_kmalloc;
  }

  pci_set_drvdata(dev, data);

  /* Init private field */
  data->dev = dev;
  data->minor = minor++;

  /* Initialize device before it's used by the driver */
  ret = pci_enable_device(dev);
  if (ret < 0) {
    printk(KERN_WARNING "dragon_pci_io: unable to initialize PCI device\n");

    goto cleanup_pci_enable;
  }

  /* Reserve PCI I/O and memory resources */
  ret = pci_request_regions(dev, "dragon_pci_io");
  if (ret < 0) {
    printk(KERN_WARNING "dragon_pci_io: unable to reserve PCI resources\n");

    goto cleanup_regions;
  }

  /* Inspect PCI BARs and search IORESOURCE_IO */
  for (i=0; i < DEVICE_COUNT_RESOURCE; i++) {
    if (pci_resource_len(dev, i) == 0)
      continue;

    if (pci_resource_start(dev, i) == 0)
      continue;

    printk(KERN_INFO "dragon_pci_io: BAR %d (%#08x-%#08x), len=%d, flags=%#08x\n", i, (u32) pci_resource_start(dev, i), (u32) pci_resource_end(dev, i), (u32) pci_resource_len(dev, i), (u32) pci_resource_flags(dev, i));

    if (pci_resource_flags(dev, i) & IORESOURCE_IO) {
      data->iobase[i] = pci_resource_start(dev, i);
      data->iolen[i] = pci_resource_len(dev, i);
      printk(KERN_INFO "dragon_pci_io: BAR %d is IO_RESOURCE_IO @ %x!\n", i, data->iobase[i]);
    } else {
      data->iobase[i] = 0;
    }
  }

  /* Install the irq handler */
#ifdef USE_IRQ
  pci_read_config_byte (dev, PCI_INTERRUPT_PIN, &mypin);
  if (mypin) {
    ret = request_irq(dev->irq, dragon_pci_io_irq_handler, IRQF_SHARED, "dragon_pci_io", data);
    if (ret < 0) {
      printk(KERN_WARNING "dragon_pci_io: unable to register irq handler\n");

      goto cleanup_irq;
    }
  }
  else
#endif
    printk(KERN_INFO "dragon_pci_io: no IRQ!\n");

  /* Link the new data structure with others */
  list_add_tail(&data->link, &dragon_pci_io_list);

  return 0;

 cleanup_irq:
  pci_release_regions(dev);
 cleanup_regions:
  pci_disable_device(dev);
 cleanup_pci_enable:
  kfree(data);
 cleanup_kmalloc:
  return ret;
}

static void __devexit dragon_pci_io_remove(struct pci_dev *dev)
{
  struct dragon_pci_io_struct *data = pci_get_drvdata(dev);
  u8 mypin;

  pci_release_regions(dev);
  pci_disable_device(dev);

#ifdef USE_IRQ
  pci_read_config_byte (dev, PCI_INTERRUPT_PIN, &mypin);
  if (mypin)
    free_irq(dev->irq, data);
#endif

  list_del(&data->link);

  kfree(data);

  printk(KERN_INFO "dragon_pci_io: device removed\n");
}

static struct pci_driver dragon_pci_io_driver = {
  .name =	"dragon_pci_io",
  .id_table =	dragon_pci_io_id_table,
  .probe =	dragon_pci_io_probe,		/* Init one device */
  .remove =	dragon_pci_io_remove,		/* Remove one device */
};

/*
 * Init and Exit
 */
static int __init dragon_pci_io_init(void)
{
  int ret;

  /* Register the device driver */
  ret = register_chrdev(major, "dragon_pci_io", &dragon_pci_io_fops);
  if (ret < 0) {
    printk(KERN_WARNING "dragon_pci_io: unable to get a major\n");

    return ret;
  }

  if (major == 0)
    major = ret; /* dynamic value */

  /* Register PCI driver */
  ret = pci_register_driver(&dragon_pci_io_driver);
  if (ret < 0) {
    printk(KERN_WARNING "dragon_pci_io: unable to register PCI driver\n");
    unregister_chrdev(major, "dragon_pci_io");

    return ret;
  }

  return 0;
}

static void __exit dragon_pci_io_exit(void)
{
  pci_unregister_driver(&dragon_pci_io_driver);

  unregister_chrdev(major, "dragon_pci_io");
}

/*
 * Module entry points
 */
module_init(dragon_pci_io_init);
module_exit(dragon_pci_io_exit);
