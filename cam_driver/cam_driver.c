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


//completion for allowing one thread to tell another that the job is done
DECLARE_COMPLETION(my_completion);

static struct usb_device_id usb_device_id [] = {
    //{USB_DEVICE(0x0000, 0x0000)},
//	{USB_DEVICE(0x046d, 0x08cc)},
	{USB_DEVICE(0x046d, 0x0994)},
    {},
};
MODULE_DEVICE_TABLE(usb, usb_device_id);

//Char driver, CamCharDev
struct Cam_Dev {
	struct semaphore sem;
	struct class * char_driver_class;
	dev_t devno;
	struct cdev char_driver_cdev;
	unsigned int myStatus, myLength, myLengthUsed;
	char myData[42666];
	int urb_completed;
	struct urb * myUrb[5];
	unsigned int data_transfered;
	int mysize;
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

	//ISOCHRONUS Endpoints, buffers, URB.
	struct usb_endpoint_descriptor *iso_in_endpoint;
	struct urb *iso_in_urb;
	struct urb *myUrb;
	int iso_in_running;
	int iso_in_size;
	int iso_in_endpointAddr;
	char *iso_in_buffer;
	//unsigned int myStatus, myLength, myLengthUsed;
	//char myData[42666];
	

	//Control Endpoints, buffers, URB.
	char *control_buffer; //8 bytes buffer for control messages.
	struct urb *control_urb;
	struct usb_control_request *control_direction; //packet information.
	
};
	


static ssize_t cam_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static int cam_driver_open(struct inode *inode, struct file *file);
static int cam_driver_release(struct inode *inode, struct file *file);
static long cam_driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
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
	//camdev.myUrb = NULL;

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

				//For Videostreaming
							//if(interface->altsetting->desc.bInterfaceSubClass == SC_VIDEOCONTROL){
							if(interface->altsetting->desc.bInterfaceSubClass == SC_VIDEOSTREAMING){

						
							printk(KERN_WARNING "cam_driver -> SC_VIDEOSTREAMING\n");
							printk(KERN_WARNING "cam_driver -> endpoint->bEndpointAddress is at 0x%x\n",endpoint->bEndpointAddress);
							printk(KERN_WARNING "cam_driver -> endpoint->bmAttributes is at 0x%x\n",endpoint->bmAttributes);
							printk(KERN_WARNING "cam_driver -> intf->desc.bInterfaceNumber is at 0x%x\n",intf->desc.bInterfaceNumber);
							printk(KERN_WARNING "cam_driver -> altSetNum is at 0x%x\n",altSetNum);
							printk(KERN_WARNING "cam_driver -> dev->devnum is at : %d\n",dev->devnum);


							camdev = kmalloc(sizeof(struct usb_skel), GFP_KERNEL);//alloue la structure locale du pilote.
							camdev->usbdev = usb_get_dev(dev);

							camdev->iso_in_size = endpoint->wMaxPacketSize;
							printk(KERN_WARNING "cam_driver -> camdev->iso_in_size is at %d\n",camdev->iso_in_size);
							camdev->iso_in_endpointAddr = endpoint->bEndpointAddress;

							camdev->iso_in_buffer = kmalloc(sizeof(camdev->iso_in_size), GFP_KERNEL);

							//init des variables pour CALLBACK?
							//camdev->myStatus = 0;
							//camdev->

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

static void complete_callback(struct urb *urb){


	int ret;
	int i;	
	unsigned char * data;
	unsigned int len;
	unsigned int maxlen;
	unsigned int nbytes;
	void * mem;

	//printk(KERN_WARNING "cam_driver -> complete_callback STARTS\n");
	
	if(urb->status == 0){
		
		//CamCharDev.data_transfered++;//counting number of bytes transfered.

		for (i = 0; i < urb->number_of_packets; ++i) {
			if(CamCharDev.myStatus == 1){
				continue;
			}
			if (urb->iso_frame_desc[i].status < 0) {
				continue;
			}
			
			data = urb->transfer_buffer + urb->iso_frame_desc[i].offset;
			if(data[1] & (1 << 6)){
				continue;
			}
			len = urb->iso_frame_desc[i].actual_length;
			if (len < 2 || data[0] < 2 || data[0] > len){
				continue;
			}
		
			len -= data[0];
			maxlen = CamCharDev.myLength - CamCharDev.myLengthUsed ;
			mem = CamCharDev.myData + CamCharDev.myLengthUsed;
			nbytes = min(len, maxlen);
			memcpy(mem, data + data[0], nbytes);
			CamCharDev.myLengthUsed += nbytes;
	
			if (len > maxlen) {				
				CamCharDev.myStatus = 1; // DONE
				printk(KERN_WARNING "cam_driver -> complete_callback DONE\n");
			}
	
			//Mark the buffer as done if the EOF marker is set. 
			
			if ((data[1] & (1 << 1)) && (CamCharDev.myLengthUsed != 0)) {						
				CamCharDev.myStatus = 1; // DONE
			}					
		}
	
		if (!(CamCharDev.myStatus == 1)){				
			if ((ret = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
				//printk(KERN_WARNING "");
			}
		}else{
			///////////////////////////////////////////////////////////////////////
			//  Synchronisation
			///////////////////////////////////////////////////////////////////////
						printk(KERN_WARNING "cam_driver -> CamCharDev.urb_completed is at %d\n",CamCharDev.urb_completed);
						//printk(KERN_WARNING "cam_driver -> isMyData filled with data?\n");
						//{
						//int j;
							//for(j=0;j<10;j++){
								//printk(KERN_WARNING "cam_driver -> URB should be accepted here!\n");
								//printk(KERN_WARNING "cam_driver -> isMyData[%d] is : %c\n",j,CamCharDev.myData[j]);
							//}
						//}

						CamCharDev.urb_completed++;
						if(CamCharDev.urb_completed == 5){
							//telling read that image aquisition is done.
							 complete(&my_completion);
							 printk(KERN_WARNING "cam_driver -> COMPLETE!\n");
							printk(KERN_WARNING "cam_driver -> CamCharDev.data_transfered is at %d\n",CamCharDev.data_transfered);
							printk(KERN_WARNING "cam_driver -> CamCharDev.myLengthUsed is at %d\n",CamCharDev.myLengthUsed);
							//printk(KERN_WARNING "cam_driver -> len is at %d\n",len);
							//printk(KERN_WARNING "cam_driver -> maxlen is at %d\n",maxlen);
							//printk(KERN_WARNING "cam_driver -> nbytes is at %d\n",nbytes);
						}
			
		}			
	}else{
		//printk(KERN_WARNING "");
		printk(KERN_WARNING "cam_driver -> Landed here?\n");
	}

	
	//printk(KERN_WARNING "cam_driver -> complete_callback DONE\n");
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
			{
			//printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_STREAMON\n");
			unsigned long user_command,pipe;
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_STREAMON\n");
			printk(KERN_WARNING "cam_driver -> dev->devnum is at : %d\n",dev->devnum);
			ret = __get_user(tmp, (int __user *)arg);
			user_command = tmp;
			//printk(KERN_WARNING "cam_driver -> User sends : 0x%x\n",user_command);
			pipe = usb_sndctrlpipe(dev, 0);
			//printk(KERN_WARNING "cam_driver -> Pipe is : 0x%d\n",pipe);
			usb_control_msg(dev, pipe, 0x0b, 
									USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE, 
									0x0004, 0x0001, NULL, 0, 0);
			break;
			}
		case CAMERA_IOCTL_STREAMOFF:
			{
			//printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_STREAMOFF\n");
			unsigned long user_command,pipe;
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_STREAMOFF\n");
			printk(KERN_WARNING "cam_driver -> dev->devnum is at : %d\n",dev->devnum);
			ret = __get_user(tmp, (int __user *)arg);
			user_command = tmp;
			//printk(KERN_WARNING "cam_driver -> User sends : 0x%d\n",user_command);
			pipe = usb_sndctrlpipe(dev, 0);
			//printk(KERN_WARNING "cam_driver -> Pipe is : 0x%d\n",pipe);
			usb_control_msg(dev, pipe, 0x0b, 
									USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE, 
									0x0000, 0x0001, NULL, 0, 0);
			break;
			}
		case CAMERA_IOCTL_GRAB:
			{
			
		
			//Init URB
				struct usb_host_interface * cur_altsetting;
				struct usb_endpoint_descriptor endpointDesc;
				//struct urb * myUrb[5];
				//int nbPackets, myPacketSize,size,nbUrbs,i,j;
				int nbPackets,size,nbUrbs,i,j;
				unsigned int myPacketSize;
				//dma_addr_t *dma = NULL;
				//void * buf = NULL;//comme dans cours #5, p.13	
				

				//Variables utilisee par la callback function 
				CamCharDev.myStatus = 0;
				CamCharDev.myLength = 42666;
				CamCharDev.myLengthUsed = 0;
				CamCharDev.urb_completed = 0;
				//CamCharDev.myData = {0};
				CamCharDev.data_transfered = 0;
				CamCharDev.mysize = 0;
				
				init_completion(&my_completion);
		
	
				//printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_GRAB\n");			

				
				cur_altsetting = interface->cur_altsetting;
				endpointDesc = cur_altsetting->endpoint[0].desc;

				nbPackets = 40;  // The number of isochronous packets this urb should contain			
				myPacketSize = le16_to_cpu(endpointDesc.wMaxPacketSize);
				//printk(KERN_WARNING "cam_driver -> endpointDesc.wMaxPacketSize is at : %d\n",endpointDesc.wMaxPacketSize);
				//printk(KERN_WARNING "cam_driver -> myPacketSize is at : %d\n",myPacketSize);			
				size = myPacketSize * nbPackets;
				CamCharDev.mysize = size;
				//printk(KERN_WARNING "cam_driver -> size is at : %d\n",size);	
				nbUrbs = 5;

										
										for (i = 0; i < nbUrbs; ++i) {
											//printk(KERN_WARNING "cam_driver -> GOT HERE FIRST!\n");	
											usb_free_urb(CamCharDev.myUrb[i]); // Pour être certain
											//is DMA involved?
											CamCharDev.myUrb[i] = usb_alloc_urb(nbPackets,GFP_KERNEL);
										 	 if (CamCharDev.myUrb[i] == NULL) {
											 	//printk(KERN_WARNING "");		
												 return -ENOMEM;
										  		}
				

					
										//  myUrb[i]->transfer_buffer = usb_buffer_alloc(dev->udev,);
											//printk(KERN_WARNING "cam_driver -> GOT HERE FIRST!\n");
											CamCharDev.myUrb[i]->transfer_buffer = usb_alloc_coherent(dev,size, GFP_KERNEL, &(CamCharDev.myUrb[i]->transfer_dma));//good
											//printk(KERN_WARNING "cam_driver -> GOT HERE SECOND!\n");	
										  if (CamCharDev.myUrb[i]->transfer_buffer == NULL) {
											 //printk(KERN_WARNING "");		
											 usb_free_urb(CamCharDev.myUrb[i]);
											 return -ENOMEM;
										  }
					

										  CamCharDev.myUrb[i]->dev = dev;
										  CamCharDev.myUrb[i]->context = dev;
										  //printk(KERN_WARNING "cam_driver -> GOT HERE FIRST!\n");
										  CamCharDev.myUrb[i]->pipe = usb_rcvisocpipe(dev, endpointDesc.bEndpointAddress);
										  //printk(KERN_WARNING "cam_driver -> GOT HERE SECOND!\n");	
										  CamCharDev.myUrb[i]->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
										  CamCharDev.myUrb[i]->interval = endpointDesc.bInterval;
										//printk(KERN_WARNING "cam_driver -> GOT HERE FIRST!\n");
										  CamCharDev.myUrb[i]->complete = complete_callback;
										//printk(KERN_WARNING "cam_driver -> GOT HERE SECOND!\n");
										  CamCharDev.myUrb[i]->number_of_packets = nbPackets;
										  CamCharDev.myUrb[i]->transfer_buffer_length = myPacketSize;

										//printk(KERN_WARNING "cam_driver -> GOT HERE FIRST!\n");
										  for (j = 0; j < nbPackets; ++j) {
											 CamCharDev.myUrb[i]->iso_frame_desc[j].offset = j * myPacketSize;
											 CamCharDev.myUrb[i]->iso_frame_desc[j].length = myPacketSize;
										  }
										//printk(KERN_WARNING "cam_driver -> GOT HERE SECOND!\n");								
										}

										for(i = 0; i < nbUrbs; i++){
										//printk(KERN_WARNING "cam_driver -> GOT HERE FIRST!\n");
										  if ((ret = usb_submit_urb(CamCharDev.myUrb[i],GFP_KERNEL)) < 0) {
											 //printk(KERN_WARNING "");		
											 return ret;
										  }
										//printk(KERN_WARNING "cam_driver -> GOT HERE SECOND!\n");
										printk(KERN_WARNING "cam_driver -> URB SENT\n");	
										}
										//printk(KERN_WARNING "cam_driver -> URB SENT\n");
									
			break;
			}
		case CAMERA_IOCTL_PANTILT:
			{
			//printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_PANTILT\n");
			//moving motors.
			unsigned long user_command,pipe;
			printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_PANTILT\n");
			/*
			int usb_control_msg(struct usb_device *dev, unsigned int pipe, __u8 request,
		    __u8 requesttype, __u16 value, __u16 index, void *data,
		    __u16 size, int timeout)*/
			printk(KERN_WARNING "cam_driver -> dev->devnum is at : %d\n",dev->devnum);
			ret = __get_user(tmp, (int __user *)arg);
			user_command = tmp;
			//printk(KERN_WARNING "cam_driver -> User sends : 0x%d\n",user_command);
			pipe = usb_sndctrlpipe(dev, 0);
			//printk(KERN_WARNING "cam_driver -> Pipe is : 0x%d\n",pipe);
			usb_control_msg(dev, pipe, 0x01, 
									USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
									0x0100, 0x0900, &user_command, 4, 0);
			
			break;
			}
		case CAMERA_IOCTL_PANTILT_RESET:
			{
				//printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_PANTILT_RESET\n");
				//moving motors.
				unsigned long user_command;
				unsigned long pipe;
				printk(KERN_WARNING "cam_driver -> CAMERA_IOCTL_PANTILT_RESET\n");
				/*
				int usb_control_msg(struct usb_device *dev, unsigned int pipe, __u8 request,
				 __u8 requesttype, __u16 value, __u16 index, void *data,
				 __u16 size, int timeout)*/
				printk(KERN_WARNING "cam_driver -> dev->devnum is at : %d\n",dev->devnum);
				ret = __get_user(tmp, (int __user *)arg);
				user_command = tmp;
				//printk(KERN_WARNING "cam_driver -> User sends : 0x%d\n",user_command);
				pipe = usb_sndctrlpipe(dev, 0);
				//printk(KERN_WARNING "cam_driver -> Pipe is : 0x%d\n",pipe);
				usb_control_msg(dev, pipe, 0x01, 
										USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
										0x0200, 0x0900, &user_command, 1, 0);
			break;
			}
	}
  return 0;
}

ssize_t cam_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos) {

	//printk(KERN_WARNING "cam_driver -> READ\n");
  struct usb_interface *interface = file->private_data;
  struct usb_device *udev = interface_to_usbdev(interface);

	printk(KERN_WARNING "cam_driver -> READ\n");

	//attendre que la fonction callback est terminee
	wait_for_completion(&my_completion);
	
	//copy to user
	//copy_to_user((buf+index_for_string_reconstruct), BDev.tampon_for_user, number_of_bytes_transfered);
	//				return number_of_bytes_transfered;
	copy_to_user(buffer, CamCharDev.myData, 42666);
	
	//Tout les urb?
		//usb_kill_urb() sur URB courant
		//usb_buffer_free() sur le transfer du URB courant
		//usb_free_urb() sur le URB courant.
		{
			
			int i;
			for(i=0;i<5;i++){
					usb_kill_urb(CamCharDev.myUrb[i]);
					//usb_buffer_free(CamCharDev.myUrb[i]->transfer_buffer);
					//static inline void usb_buffer_free(struct usb_device *dev, size_t size, void *addr, dma_addr_t dma)
					//usb_buffer_free(udev, CamCharDev.mysize, CamCharDev.myUrb[i]->transfer_buffer, &(CamCharDev.myUrb[i]->transfer_dma));
					//usb_free_coherent(struct usb_device *dev, size_t size, void *addr, dma_addr_t dma)
					usb_free_coherent(udev, CamCharDev.mysize, CamCharDev.myUrb[i]->transfer_buffer, (CamCharDev.myUrb[i]->transfer_dma));
					usb_free_urb(CamCharDev.myUrb[i]);		
			}
			
		}

	//return number of data transfered.
	//return CamCharDev.data_transfered;
	return CamCharDev.myLengthUsed;


  return 0;
}

module_init(cam_driver_init);
module_exit(cam_driver_cleanup);
//module_usb_driver(cam_driver);
