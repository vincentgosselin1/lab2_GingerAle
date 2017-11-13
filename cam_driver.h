#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/usb.h>


static struct usb_driver udriver = {
  .name = "HelloWorld Usb Driver",
  .probe = ele784_probe,
  .disconnect = ele784_disconnect,
  .id_table = usb_device_id,
};

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = ele784_read,
  .open = ele784_open,
  .unlocked_ioctl = ele784_ioctl,
};

static struct usb_class_driver class_driver = {
  .name = "Hello%d",
  .fops = &fops,
  .minor_base = DEV_MINOR,
};

//module_init(ele784_init);
//module_exit(ele784_cleanup);
module_usb_driver(udriver);

static int cam_driver_init(void);

static void cam_driver_cleanup(void);

int cam_driver_open (struct inode *inode, struct file *flip);

int cam_driver_release (struct inode *inode, struct file *flip);

static ssize_t cam_driver_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops);

long cam_driver_ioctl (struct file *flip, unsigned int cmd, unsigned long arg);

void cam_driver_disconnect(struct usb_interface *intf);
