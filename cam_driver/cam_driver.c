//Laboratoire #2 par Vincent Gosselin, 2017.
//Webcam Driver for QuickCam® Orbit MP Logitech

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
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/usb.h>
#include <linux/usb/video.h>
#include <linux/completion.h>

#include <linux/moduleparam.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#include <asm/ioctl.h>
#include "camera_ioctl.h"

#include "usbvideo.h"

//#define DEV_MAJOR 250
//#define DEV_MINOR	0
#define DEV_MINOR       0x00
#define DEV_MINORS      0x01

MODULE_LICENSE("Dual BSD/GPL");

static struct usb_device_id usb_device_id [] = {
    //{USB_DEVICE(0x0000, 0x0000)},
	{USB_DEVICE(0x046d, 0x08cc)},
//	{USB_DEVICE(0x046d, 0x0994)},
    {},
};
MODULE_DEVICE_TABLE(usb, usb_device_id);

//Char driver, CamCharDev
struct Cam_Dev {
	struct semaphore sem;
	struct class * char_driver_class;
	dev_t devno;
	struct cdev char_driver_cdev;
} CamCharDev;

//structure pour binder les endpoints/alt-setting etc..
struct usb_skel {
	struct usb_device * usbdev;
	struct usb_interface * interface;
	char minor;
	char serial_number[8]; //maybe not used.

	//access control
	int open_count;
	struct semaphore sem; //locks this structure
	//spinlock required?
	spinlock_t command_spinlock; //locks commands to send.

	//BULK Endpoints, buffers, URB.
	char *bulk_in_buffer;
	struct usb_endpoint_descriptor *bulk_in_endpoint;
	struct urb *bulk_in_urb;
	int bulk_in_running;

	//Control Endpoints, buffers, URB.
	char *control_buffer; //8 bytes buffer for control messages.
	struct urb *control_urb;
	struct usb_control_request *control_direction; //packet information.
	
	/*
	int bulk_in_endpointAddr;
	int bulk_in_size;
	char * bulk_in_buffer;
	*/	
};
	


static ssize_t cam_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static int cam_driver_open(struct inode *inode, struct file *file);
static int cam_driver_release(struct inode *inode, struct file *file);
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
	.release = cam_driver_release,
	.unlocked_ioctl = cam_driver_ioctl,
};

static struct usb_class_driver class_driver = {
  .name = "Hello%d",
  .fops = &fops,
  .minor_base = DEV_MINOR,
};


static int __init cam_driver_init (void) {

	int result;//for error catching

	printk(KERN_WARNING "cam_driver, Installing USB driver\n");

	result = usb_register(&cam_driver);
	if(result<0){
		printk(KERN_WARNING "cam_driver -> Init : usb_register failed. Error number is %d\n",result);
		return result;
	}

	//Doit se faire dans le probe.
	
	//Char driver
	result = alloc_chrdev_region(&CamCharDev.devno,0,1,"MyChar_driver");
	if(result){ CamCharDev.devno = MKDEV(250,0); }
	CamCharDev.char_driver_class = class_create(THIS_MODULE, "Char_driverClass");
	device_create(CamCharDev.char_driver_class, NULL, CamCharDev.devno, NULL, "etsele_cdev");
	cdev_init(&CamCharDev.char_driver_cdev, &fops);
	CamCharDev.char_driver_cdev.owner = THIS_MODULE;

	//cam_driver structure init.
	

		//init custom structures

	cdev_add(&CamCharDev.char_driver_cdev, CamCharDev.devno, 1);
	

	return 0;
}

static void __exit cam_driver_cleanup (void) {

	printk(KERN_WARNING "cam_driver, Uninstalling USB driver\n");

	usb_deregister(&cam_driver);


	//char Driver
	
	cdev_del(&CamCharDev.char_driver_cdev);
	unregister_chrdev_region(CamCharDev.devno,1);
	device_destroy(CamCharDev.char_driver_class, CamCharDev.devno);
	class_destroy(CamCharDev.char_driver_class);
	//kfree

}


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

int cam_driver_release(struct inode *inode, struct file *file) {

	printk(KERN_WARNING "cam_driver -> Release\n");
	return 0;
}

// Probing USB device
int cam_driver_probe(struct usb_interface *interface, const struct usb_device_id *id) {

	const struct usb_host_interface *intf;
	const struct usb_endpoint_descriptor *endpoint;
	struct usb_device *dev = interface_to_usbdev(interface);//recupere la structure du pilote USB.
	//struct usb_cam
	struct usb_skel *camdev = NULL;
	int n, m, altSetNum = -1;//, activeInterface = -1; 

	printk(KERN_WARNING "cam_driver -> Probe\n");

	/*	
	camdev = kmalloc(sizeof(struct usb_skel), GFP_KERNEL);//alloue la structure locale du pilote.
	camdev->usbdev = usb_get_dev(dev);
	*/

	//Pour passer au travers des reglages alternatifs de l'interface
	for(n=0; n<interface->num_altsetting; n++){

		intf = &interface->altsetting[n];
		altSetNum = intf->desc.bAlternateSetting;
		//Pour passer au travers des Endpoints de l'interface
			for(m=0; m< intf->desc.bNumEndpoints; m++){
				endpoint = &intf->endpoint[m].desc;

				/*
				//For Video Streaming
				if(((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) && 
						((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) && 
							(intf->desc.bInterfaceNumber == 1) && altSetNum == 4){

							//found Isochronus endpoint for transfers.
							printk(KERN_WARNING "cam_driver -> ENDPOINT IS DIR IN\n");
							printk(KERN_WARNING "cam_driver -> ENDPOINT is ISOCHRONUS\n");
							printk(KERN_WARNING "cam_driver -> endpoint->bEndpointAddress is at 0x%x\n",endpoint->bEndpointAddress);
							printk(KERN_WARNING "cam_driver -> endpoint->bmAttributes is at 0x%x\n",endpoint->bmAttributes);
							printk(KERN_WARNING "cam_driver -> intf->desc.bInterfaceNumber is at 0x%x\n",intf->desc.bInterfaceNumber);
							printk(KERN_WARNING "cam_driver -> altSetNum is at 0x%x\n",altSetNum);
							printk(KERN_WARNING "cam_driver -> dev->devnum is at : %d\n",dev->devnum);

							camdev = kmalloc(sizeof(struct usb_skel), GFP_KERNEL);//alloue la structure locale du pilote.
							camdev->usbdev = usb_get_dev(dev);

							usb_set_intfdata(interface,camdev);
							usb_register_dev(interface,&class_driver);
							usb_set_interface(dev, intf->desc.bInterfaceNumber, altSetNum);
							 

							//return 0;//found Isochronus endpoint for transfers.
				}
				*/
				//For VideoControl
				//if(((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) && 
				//		((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) && 
							if(interface->altsetting->desc.bInterfaceSubClass == SC_VIDEOCONTROL){

							//found Isochronus endpoint for transfers.
							printk(KERN_WARNING "cam_driver -> VideoControl\n");
							printk(KERN_WARNING "cam_driver -> endpoint->bEndpointAddress is at 0x%x\n",endpoint->bEndpointAddress);
							printk(KERN_WARNING "cam_driver -> endpoint->bmAttributes is at 0x%x\n",endpoint->bmAttributes);
							printk(KERN_WARNING "cam_driver -> intf->desc.bInterfaceNumber is at 0x%x\n",intf->desc.bInterfaceNumber);
							printk(KERN_WARNING "cam_driver -> altSetNum is at 0x%x\n",altSetNum);
							printk(KERN_WARNING "cam_driver -> dev->devnum is at : %d\n",dev->devnum);

							camdev = kmalloc(sizeof(struct usb_skel), GFP_KERNEL);//alloue la structure locale du pilote.
							camdev->usbdev = usb_get_dev(dev);

							usb_set_intfdata(interface,camdev);
							usb_register_dev(interface,&class_driver);
							usb_set_interface(dev, intf->desc.bInterfaceNumber, altSetNum);
							 

							return 0;//found VideoControl
				}
								


			}//endpoint loop			
	}//interface loop	

  return 0;
}

void cam_driver_disconnect (struct usb_interface *intf) {
  printk(KERN_INFO "cam_driver -> Disconnect\n");

  usb_deregister_dev(intf, &class_driver);
}

long cam_driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  struct usb_interface *interface = file->private_data;
  struct usb_device *dev = interface_to_usbdev(interface);
	
	int err = 0;
	int tmp,ret;

	printk(KERN_WARNING "cam_driver -> IOCTL\n");

	if(_IOC_TYPE(cmd) != CAMERA_IOCTL_MAGIC) { return -ENOTTY; }
	if(_IOC_NR(cmd) > CAMERA_IOCTL_MAXNR) { return -ENOTTY; }
	if(_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void __user *) arg, _IOC_SIZE(cmd));
	} else if(_IOC_DIR(cmd) & _IOC_WRITE){
		err = !access_ok(VERIFY_READ,	(void __user *) arg,	_IOC_SIZE(cmd));
	}
	if(err){ return -EFAULT; }
	switch(cmd){
		case CAMERA_IOCTL_GET:
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_GET\n");
			break;
		case CAMERA_IOCTL_SET:
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_SET\n");
			break;
		case CAMERA_IOCTL_STREAMON:
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_STREAMON\n");
			break;
		case CAMERA_IOCTL_STREAMOFF:
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_STREAMOFF\n");
			break;
		case CAMERA_IOCTL_GRAB:
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_GRAB\n");
			break;
		case CAMERA_IOCTL_PANTILT:
			{
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_PANTILT\n");
			//moving motors.
			unsigned long user_command,pipe;
			/*
			int usb_control_msg(struct usb_device *dev, unsigned int pipe, __u8 request,
		    __u8 requesttype, __u16 value, __u16 index, void *data,
		    __u16 size, int timeout)*/
			printk(KERN_WARNING "cam_driver -> dev->devnum is at : %d\n",dev->devnum);
			ret = __get_user(tmp, (int __user *)arg);
			user_command = tmp;
			printk(KERN_WARNING "cam_driver -> User sends : 0x%x\n",user_command);
			pipe = usb_sndctrlpipe(dev, 0);
			//pipe = 0x00000800;
			printk(KERN_WARNING "cam_driver -> Pipe is : 0x%x\n",pipe);
			usb_control_msg(dev, pipe, 0x01, 
									USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
									0x0100, 0x0900, &user_command, 4, 0);
			break;
			}
		case CAMERA_IOCTL_PANTILT_RESET:
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_PANTILT_RESET\n");
			break;
	}
  return 0;
}

ssize_t cam_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos) {
  struct usb_interface *interface = file->private_data;
  struct usb_device *udev = interface_to_usbdev(interface);

	printk(KERN_WARNING "cam_driver -> READ\n");


  return 0;
}

module_init(cam_driver_init);
module_exit(cam_driver_cleanup);
//module_usb_driver(cam_driver);
