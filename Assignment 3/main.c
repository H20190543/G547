#include"headers.h"
#include"declarations.h"

uint8_t cmd_length[256] = {
//	 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  0
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  1
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  2
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  3
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  4
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  5
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  6
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  7
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  8
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  9
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  A
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  B
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  C
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  D
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  E
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  F
};



blkdev *gdev;
dev_info ginfo;
int probe_flg = 0;
int gmajor_num;
sector_t dev_capacity = 0;

module_param(dev_capacity, long, 0660);

static void usbdev_disconnect(struct usb_interface *interface)
{
	log("USBDEV Device Removed\n");
	return;
}

static struct usb_device_id usbdev_table [] = {
	{USB_DEVICE(SANDISK_VID, SANDISK_PID)},
	{USB_DEVICE(HP_VID, HP_PID)},
	{} 
};


static int myUSBdev_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	int i,tu_ret, tu_try = 0;
	uint8_t epin=0, epout=0; 												// In endpoint and out endpoint address
	struct usb_endpoint_descriptor *ep_desc;

	// get device structure out of interface
	ginfo.device = interface_to_usbdev(interface);
	if ( ginfo.device == NULL ) {
		log("cannot fetch device structure from interface structure\n");
		return -1;
	}

	// Getting VID and PID of the device from device descriptor
	log("VID of the device: %04x\n", ginfo.device->descriptor.idVendor);
	log("PID of the device: %04x\n", ginfo.device->descriptor.idProduct);

	
	if( (id->idProduct == SANDISK_PID) || (id->idProduct == HP_PID) )
	{
		log("Known USB drive detected\n");
	}
	else {
		log("Unknown device\n");
		return -1;
	}

	// Getting other info of the interface of the device
	log("USB Interface class : %04x\n", interface->cur_altsetting->desc.bInterfaceClass);
	log("USB Interface Subclass : %04x\n", interface->cur_altsetting->desc.bInterfaceSubClass);
	log("USB Interface Protocol : %04x\n", interface->cur_altsetting->desc.bInterfaceProtocol);
	log("No. of Endpoints = %d\n", interface->cur_altsetting->desc.bNumEndpoints);
	log("No. of Altsettings = %d\n",interface->num_altsetting);

	for(i=0; i < interface->cur_altsetting->desc.bNumEndpoints; i++)
	{
		ep_desc = &interface->cur_altsetting->endpoint[i].desc;

		if ( (ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) & USB_ENDPOINT_XFER_BULK ) {
			log( "Bulk transfer endpoint found\n");
			if (ep_desc->bEndpointAddress & USB_EP_IN) {
				if(!epin)
					ginfo.epin = ep_desc->bEndpointAddress;
				log("IN endpoint found with address %d\n",ginfo.epin);
			}
			else {
				if(!epout)
					ginfo.epout = ep_desc->bEndpointAddress;
				log("OUT endpoint found with address %d\n",ginfo.epout);
			}
		}
	}

	dev_capacity ? (ginfo.capacity = dev_capacity+800) : (ginfo.capacity = CAPACITY);

	// always reset the device first
	usb_reset_device(ginfo.device);

	// check if device is ready to read/write
	do{
		tu_ret = test_unit_ready();
		if( tu_ret!= 0 )
			log("Device not yet ready\n");
		tu_try++;
	} while( tu_try < TEST_DEVICE_RETRY);

	if( tu_ret == 0 )
		log("Device ready to receive read/write request\n");
	else {
		log("Device not ready. So aborting ....\n");
		return -1;
	}

	// add block device driver
	if(add_usb_dev() != 0 ) {
		log("Cannot register usb block dev\n");
		return -1;
	}

	probe_flg = 1;

	return 0;
}



/*Operations structure*/
static struct usb_driver usbdev_driver = {
	name: "udriver",  											//name of the device
	probe: myUSBdev_probe, 											// Whenever Device is plugged in
	disconnect: usbdev_disconnect, 									// When we remove a device
	id_table: usbdev_table, 										//  List of devices served by this driver
};


int device_init(void)
{
	log("Driver Inserted\n");
	usb_register(&usbdev_driver);
	return 0;
}

void device_exit(void)
{
	usb_deregister(&usbdev_driver);
	if(probe_flg) {
		del_gendisk(gdev->gd);
		flush_workqueue(gdev->dev_wq);
		destroy_workqueue(gdev->dev_wq);
		blk_cleanup_queue(gdev->rq);
		unregister_blkdev(gmajor_num, DEVICE_NAME);
		kfree(gdev);
	}
	log("Driver Removed\n");
}

module_init(device_init);
module_exit(device_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Atul Pant");
MODULE_DESCRIPTION("Block device driver for scsi mass storage devices");

