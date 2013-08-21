/*
 * Linux driver for KNJN Dragon PCI (PCI_PnP.100K.bit, IORESOURCE_MEM)
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

MODULE_DESCRIPTION("dragon_pci_mem");
MODULE_AUTHOR("Pierre Ficheux");
MODULE_LICENSE("GPL");

/*
 * Arguments
 */
static int major = 0; /* Major number */
module_param(major, int, 0660);
MODULE_PARM_DESC(major, "Static major number (none = dynamic)");

static int debug = 0; 
module_param(debug, int, 0660);
MODULE_PARM_DESC(debug, "Debug flag (1 = YES, 0 = NO)");

/*
 * Supported devices
 */
static struct pci_device_id dragon_pci_mem_id_table[] __devinitdata = {
  {0x0100, 0x0000, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
  {0,}	/* 0 terminated list */
};
MODULE_DEVICE_TABLE(pci, dragon_pci_mem_id_table);

/*
 * Global variables
 */
static LIST_HEAD(dragon_pci_mem_list);

struct dragon_pci_mem_struct {
  struct list_head	link; /* Double linked list */
  struct pci_dev	*dev; /* PCI device */
  int			minor; /* Minor number */
  unsigned int		*mmio[DEVICE_COUNT_RESOURCE];
  u32			mmio_len[DEVICE_COUNT_RESOURCE];
};

/*
 * Event handlers
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
  irqreturn_t dragon_pci_mem_irq_handler(int irq, void *dev_id)
#else
  irqreturn_t dragon_pci_mem_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
  struct dragon_pci_mem_struct *data = (struct dragon_pci_mem_struct *)dev_id;

//  printk(KERN_INFO "dragon_pci_mem: interrupt from device %d\n", data->minor);

  return IRQ_HANDLED;
}

/*
 * File operations
 */
static ssize_t dragon_pci_mem_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
  int i, real, bank = DEVICE_COUNT_RESOURCE;
  struct dragon_pci_mem_struct *data = file->private_data;

  /* Find the first remapped I/O memory bank to read */
  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
    if (data->mmio[i] != 0) {
      bank = i;
      break;
    }
  }

  /* No bank found */
  if (bank == DEVICE_COUNT_RESOURCE) {
    printk(KERN_INFO "dragon_pci_mem: no remapped I/O memory bank to read\n");
    return -ENXIO;
  }

  /* Check for overflow */
  real = min(data->mmio_len[bank] - (int)*ppos, count);

  /* Copy data from board */
  if (real)
    if (copy_to_user((void __user *)buf, (void *)data->mmio[bank] + (int)*ppos, real))
      return -EFAULT;

  if (debug)
    printk(KERN_INFO "dragon_pci_mem: read %d/%d chars at offset %d from remapped I/O memory bank %d\n", real, count, (int)*ppos, bank);

  *ppos += real;

  return real;
}

static ssize_t dragon_pci_mem_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
  int i, real, bank = DEVICE_COUNT_RESOURCE;
  struct dragon_pci_mem_struct *data = file->private_data;

  /* Find the first remapped I/O memory bank to read */
  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
    if (data->mmio[i] != 0) {
      bank = i;
      break;
    }
  }

  /* No bank found */
  if (bank == DEVICE_COUNT_RESOURCE) {
    printk(KERN_INFO "dragon_pci_mem: no remapped I/O memory bank to read\n");
    return -ENXIO;
  }

  /* Check for overflow */
  real = min(data->mmio_len[bank] - (int)*ppos, count);

  // for (i = 0 ; i < real ; i++)
  //    printk (KERN_INFO "buf[%d] = %x\n", i, *(buf+i));

  if (real)
    if (copy_from_user((void*)data->mmio[bank] + (int)*ppos, (void __user *)buf , real))
      return -EFAULT;

  if (debug)
    printk(KERN_INFO "dragon_pci_mem: wrote %d/%d chars at offset %d to remapped I/O memory bank %d\n", real, count, (int)*ppos, bank);

  *ppos += real;

  return real;
}

static int dragon_pci_mem_open(struct inode *inode, struct file *file)
{
  int minor = MINOR(inode->i_rdev);
  struct list_head *cur;
  struct dragon_pci_mem_struct *data;

  if (debug)
    printk("dragon_pci_mem_open()\n");

  list_for_each(cur, &dragon_pci_mem_list) {
    data = list_entry(cur, struct dragon_pci_mem_struct, link);

    if (data->minor == minor) {
      file->private_data = data;

      return 0;
    }
  }

  printk(KERN_WARNING "dragon_pci_mem: minor %d not found\n", minor);

  return -ENODEV;
}

static int dragon_pci_mem_release(struct inode *inode, struct file *file)
{
  if (debug)
    printk("dragon_pci_mem_release()\n");

  file->private_data = NULL;

  return 0;
}

static struct file_operations dragon_pci_mem_fops = {
  .owner =	THIS_MODULE,
  .read =	dragon_pci_mem_read,
  .write =	dragon_pci_mem_write,
  .open =	dragon_pci_mem_open,
  .release =	dragon_pci_mem_release,
};

/*
 * PCI handling
 */
static int __devinit dragon_pci_mem_probe(struct pci_dev *dev, const struct pci_device_id *ent)
{
  int i, ret = 0;
  struct dragon_pci_mem_struct *data;
  static int minor = 0;
  u8 mypin;

  printk(KERN_INFO "dragon_pci_mem: found %x:%x\n", ent->vendor, ent->device);
  printk(KERN_INFO "dragon_pci_mem: using major %d and minor %d for this device\n", major, minor);

  /* Allocate a private structure and reference it as driver's data */
  data = (struct dragon_pci_mem_struct *)kmalloc(sizeof(struct dragon_pci_mem_struct), GFP_KERNEL);
  if (data == NULL) {
    printk(KERN_WARNING "dragon_pci_mem: unable to allocate private structure\n");

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
    printk(KERN_WARNING "dragon_pci_mem: unable to initialize PCI device\n");

    goto cleanup_pci_enable;
  }

  /* Reserve PCI I/O and memory resources */
  ret = pci_request_regions(dev, "dragon_pci_mem");
  if (ret < 0) {
    printk(KERN_WARNING "dragon_pci_mem: unable to reserve PCI resources\n");

    goto cleanup_regions;
  }

  /* Inspect PCI BARs and search IORESOURCE_IO */
  for (i=0; i < DEVICE_COUNT_RESOURCE; i++) {
    if (pci_resource_len(dev, i) == 0)
      continue;

    if (pci_resource_start(dev, i) == 0)
      continue;

    printk(KERN_INFO "dragon_pci_mem: BAR %d (%#08x-%#08x), len=%d, flags=%#08x\n", i, (u32) pci_resource_start(dev, i), (u32) pci_resource_end(dev, i), (u32) pci_resource_len(dev, i), (u32) pci_resource_flags(dev, i));

    // Remap if IORESOURCE_MEM. Check if cacheable
    //
    // Could be done with :
    //  data->mmio[i] = pci_iomap (dev, i, pci_resource_len(dev, i));
    //
    if (pci_resource_flags(dev, i) & IORESOURCE_MEM) {
      if (pci_resource_flags(dev, i) & IORESOURCE_CACHEABLE) {
	printk (KERN_INFO "cacheable ! \n");
	data->mmio[i] = ioremap(pci_resource_start(dev, i), pci_resource_len(dev, i));
      }
      else {
	printk (KERN_INFO "NOT cacheable ! \n");
	data->mmio[i] = ioremap_nocache(pci_resource_start(dev, i), pci_resource_len(dev, i));
      }
    
      if (data->mmio[i] == NULL) {
	printk(KERN_WARNING "dragon_pci_mem: unable to remap I/O memory\n");
	
	ret = -ENOMEM;
	goto cleanup_ioremap;
      }

      data->mmio_len[i] = pci_resource_len(dev, i);

      printk(KERN_INFO "dragon_pci_mem: I/O memory has been remaped at %#08x\n", (u32)data->mmio[i]);
    } else {
      data->mmio[i] = 0;
    }
  }

  /* Install the irq handler */
  pci_read_config_byte (dev, PCI_INTERRUPT_PIN, &mypin);
  if (mypin) {
    ret = request_irq(dev->irq, dragon_pci_mem_irq_handler, IRQF_SHARED, "dragon_pci_mem", data);
    if (ret < 0) {
      printk(KERN_WARNING "dragon_pci_mem: unable to register irq handler\n");

      goto cleanup_irq;
    }
  }
  else
    printk(KERN_INFO "dragon_pci_mem: no IRQ!\n");

  /* Link the new data structure with others */
  list_add_tail(&data->link, &dragon_pci_mem_list);

  return 0;

cleanup_irq:
  for (i--; i >= 0; i--) {
    if (data->mmio[i] != NULL)
      iounmap(data->mmio[i]);
  }
cleanup_ioremap:
  pci_release_regions(dev);
cleanup_regions:
  pci_disable_device(dev);
cleanup_pci_enable:
  kfree(data);
cleanup_kmalloc:
  return ret;
}

static void __devexit dragon_pci_mem_remove(struct pci_dev *dev)
{
  int i;
  struct dragon_pci_mem_struct *data = pci_get_drvdata(dev);
  u8 mypin;

  for (i = 0 ; i < DEVICE_COUNT_RESOURCE ; i++) {
    if (data->mmio[i] != NULL)
      iounmap(data->mmio[i]);
  }

  pci_release_regions(dev);
  pci_disable_device(dev);

  pci_read_config_byte (dev, PCI_INTERRUPT_PIN, &mypin);
  if (mypin)
    free_irq(dev->irq, data);

  list_del(&data->link);

  kfree(data);

  printk(KERN_INFO "dragon_pci_mem: device removed\n");
}

static struct pci_driver dragon_pci_mem_driver = {
  .name =	"dragon_pci_mem",
  .id_table =	dragon_pci_mem_id_table,
  .probe =	dragon_pci_mem_probe,		/* Init one device */
  .remove =	dragon_pci_mem_remove,		/* Remove one device */
};

/*
 * Init and Exit
 */
static int __init dragon_pci_mem_init(void)
{
  int ret;

  /* Register the device driver */
  ret = register_chrdev(major, "dragon_pci_mem", &dragon_pci_mem_fops);
  if (ret < 0) {
    printk(KERN_WARNING "dragon_pci_mem: unable to get a major\n");

    return ret;
  }

  if (major == 0)
    major = ret; /* dynamic value */

  /* Register PCI driver */
  ret = pci_register_driver(&dragon_pci_mem_driver);
  if (ret < 0) {
    printk(KERN_WARNING "dragon_pci_mem: unable to register PCI driver\n");

    unregister_chrdev(major, "dragon_pci_mem");

    return ret;
  }

  return 0;
}

static void __exit dragon_pci_mem_exit(void)
{
  pci_unregister_driver(&dragon_pci_mem_driver);

  unregister_chrdev(major, "dragon_pci_mem");
}

/*
 * Module entry points
 */
module_init(dragon_pci_mem_init);
module_exit(dragon_pci_mem_exit);
