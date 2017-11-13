//Laboratoire #2 par Vincent Gosselin, 2017.
//Webcam Driver for QuickCam® OrbitTM MP Logitech

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
#include <linux/usb.h>
#include <linux/usb/video.h>
#include <linux/completion.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>


#define DEV_MINOR       0x00
#define DEV_MINORS      0x01

MODULE_LICENSE("Dual BSD/GPL");

static struct usb_device_id usb_device_id [] = {
    {USB_DEVICE(0x0000, 0x0000)},
    {},
};
MODULE_DEVICE_TABLE(usb, usb_device_id);

static ssize_t cam_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static int cam_driver_open(struct inode *inode, struct file *file);
static long cam_driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t cam_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos);
static int cam_driver_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void cam_driver_disconnect (struct usb_interface *intf);

static struct usb_driver cam_driver = {
  .name = "HelloWorld Usb Driver",
  .probe = cam_driver_probe,
  .disconnect = cam_driver_disconnect,
  .id_table = usb_device_id,
};

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = cam_driver_read,
  .open = cam_driver_open,
  .unlocked_ioctl = cam_driver_ioctl,
};

static struct usb_class_driver class_driver = {
  .name = "Hello%d",
  .fops = &fops,
  .minor_base = DEV_MINOR,
};


// Event when the device is opened
int cam_driver_open(struct inode *inode, struct file *file) {
  struct usb_interface *intf;
  int subminor;

  printk(KERN_WARNING "cam_driver -> Open\n");

  subminor = iminor(inode);
  intf = usb_find_interface(&cam_driver, subminor);
  if (!intf) {
    printk(KERN_WARNING "cam_driver -> Open: Ne peux ouvrir le peripherique\n");
    return -ENODEV;
  }

  file->private_data = intf;
  return 0;
}

// Probing USB device
int cam_driver_probe(struct usb_interface *interface, const struct usb_device_id *id) {
  struct usb_host_interface *iface_desc;

  printk(KERN_INFO "cam_driver -> Probe\n");




  return 0;
}

void cam_driver_disconnect (struct usb_interface *intf) {
  printk(KERN_INFO "cam_driver -> Disconnect\n");

  usb_deregister_dev(intf, &class_driver);
}

long cam_driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  struct usb_interface *interface = file->private_data;
  struct usb_device *udev = interface_to_usbdev(interface);


  return 0;
}

ssize_t cam_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos) {
  struct usb_interface *interface = file->private_data;
  struct usb_device *udev = interface_to_usbdev(interface);


  return 0;
}

//module_init(cam_driver_init);
//module_exit(cam_driver_cleanup);
module_usb_driver(cam_driver);