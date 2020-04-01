#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/string.h>
#include<linux/slab.h>
#include<linux/usb.h>


#define RETRY_MAX 10
#define READ_CAPACITY_LENGTH 0x08
#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])

//usb 1
#define SANDISK_VID_1 0x0781 
#define SANDISK_PID_1 0x5590

//usb 2
#define SANDISK_VID_2 0x0781 
#define SANDISK_PID_2 0x5567

struct command_block_wrapper {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
};

struct command_status_wrapper {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
};

static uint8_t cdb_length[256] = {
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


static struct usb_device_id usbdev_table[] = {
	{USB_DEVICE(SANDISK_VID_1,SANDISK_PID_1)},
	{USB_DEVICE(SANDISK_VID_2,SANDISK_PID_2)},
	{}
};

static void usbdev_disconnect(struct usb_interface *interface)
{
	printk(KERN_INFO "USBDEV Device Removed\n");
	return;
}

static int read_capacity_cmd(struct usb_device *device, uint8_t ep_in, uint8_t ep_out)
{
int  retry=0;
uint8_t cdb_len;
uint8_t cdb[16];
struct command_block_wrapper *cbw = NULL;
struct command_status_wrapper *csw = NULL;
uint8_t *buffer = NULL;


long max_lba,block_size;
long usb_size;
int testval,count,size;


if ( !(cbw = (struct command_block_wrapper *)kmalloc(sizeof(struct command_block_wrapper), GFP_KERNEL)) ) {
		printk(KERN_ERR "Memory allocation to cbw failed\n");
		return -1;
}

if ( !(buffer = (uint8_t *)kmalloc(sizeof(uint8_t)*64, GFP_KERNEL)) ) {
		printk(KERN_ERR "Memory allocation to buffer failed\n");
		return -1;
}
if ( !(csw = (struct command_status_wrapper *)kmalloc(sizeof(struct command_status_wrapper), GFP_KERNEL)) ) {
		printk(KERN_ERR "Memory allocation to csw failed\n");
		return -1;
}


memset(cdb, 0, sizeof(cdb));
memset(cbw, 0, sizeof(struct command_block_wrapper));
memset(csw, 0, sizeof(struct command_status_wrapper));
cdb[0] = 0x25; 
cdb_len = cdb_length[cdb[0]];


memset(cbw,'\0',sizeof(struct command_block_wrapper));
cbw->dCBWSignature[0] = 'U';
cbw->dCBWSignature[1] = 'S';
cbw->dCBWSignature[2] = 'B';
cbw->dCBWSignature[3] = 'C';
cbw->dCBWTag = 10;
cbw->dCBWDataTransferLength = READ_CAPACITY_LENGTH;
cbw->bmCBWFlags = 0x80;
cbw->bCBWLUN = 0; 
cbw->bCBWCBLength = cdb_len;
memcpy(cbw->CBWCB, cdb, cdb_len); 


usb_reset_device(device);		


do {
				
		testval = usb_bulk_msg(device,usb_sndbulkpipe(device, ep_out), (unsigned char*)cbw, 31, &size, 2000);
		if (testval != 0) {
			usb_clear_halt(device,usb_sndbulkpipe(device, ep_out));		
		}
		retry++;
} while ((retry<RETRY_MAX)&&(testval!=0));

if(testval == 0)
{	
	
	testval = usb_bulk_msg(device,usb_rcvbulkpipe(device, ep_in),(unsigned char*)buffer,READ_CAPACITY_LENGTH, &count, 2000);
	if (testval == 0) {
		max_lba =  be_to_int32(&buffer[0]);
		block_size = be_to_int32(&buffer[4]);
		usb_size = ((max_lba+1)*block_size/(1024*1024*1024));
		printk(KERN_INFO "Num of logic blocks : %ld\nblock size : %ld \nPendrive size : %ld\n",(max_lba+1),block_size,usb_size);
		retry = 0;
		do {	
			testval = usb_bulk_msg(device,usb_rcvbulkpipe(device, ep_in),(unsigned char*)csw,13, &count, 2000);
			if (testval != 0 ) 
				usb_clear_halt(device,usb_sndbulkpipe(device, ep_out));
			retry++;
		} while ((retry<RETRY_MAX)&&(testval!=0));
	}	
	else 
		return -1;
	
}
else
	printk(KERN_INFO "Sending CDB failed. testval:%d\n",testval);

kfree(cbw);
kfree(csw);
kfree(buffer);
return 0;
}

static int usbdev_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	int i;
	uint8_t epAddr,epAttr;
	uint8_t ep_in = 0;
	uint8_t ep_out = 0; 
	struct usb_device *device;
	
	struct usb_endpoint_descriptor *ep_desc;
	
	printk(KERN_INFO "USB device vid : %x", id->idVendor);
	printk(KERN_INFO "USB device pid : %x", id->idProduct);
	
	
	device = interface_to_usbdev(interface);
	printk(KERN_INFO "USB_DEVICE_CLASS : %x",interface->cur_altsetting->desc.bInterfaceClass);
	printk(KERN_INFO "USB_DEVICE_SUBCLASS : %x", interface->cur_altsetting->desc.bInterfaceSubClass);
	printk(KERN_INFO "USB_DEVICE_PROTOCOL : %x",interface->cur_altsetting->desc.bInterfaceProtocol);
	
	if(id->idProduct == SANDISK_PID_1 && id->idVendor == SANDISK_VID_1)
	{
		printk(KERN_INFO "Known device Sandisk plugged in.\n");
	} 
	
	if(id->idProduct == SANDISK_PID_2 && id->idVendor == SANDISK_VID_2)
	{
		printk(KERN_INFO "Known device Cruzer blade is plugged in.\n");
	} 
	
	printk(KERN_INFO "Num of Altsettings : %d\n",interface->num_altsetting);
	printk(KERN_INFO "Num of Endpoints : %d\n",interface->cur_altsetting->desc.bNumEndpoints);
	for(i=0;i<interface->cur_altsetting->desc.bNumEndpoints;i++)
	{
		ep_desc = &interface->cur_altsetting->endpoint[i].desc;
		epAddr = ep_desc->bEndpointAddress;
		epAttr = ep_desc->bmAttributes;
		if((epAttr & USB_ENDPOINT_XFERTYPE_MASK)==USB_ENDPOINT_XFER_BULK)
		{
			if(epAddr & 0x80)
			{
				printk(KERN_INFO "EP %d is Bulk IN\n",i);
				ep_in = epAddr;
			}		
			else
			{
				printk(KERN_INFO "EP %d is Bulk OUT\n",i);
				ep_out = epAddr;
			}		
		}

		
		if((epAttr & USB_ENDPOINT_XFERTYPE_MASK)==USB_ENDPOINT_XFER_INT)
		{
			if(epAddr & 0x80)
				printk(KERN_INFO "EP %d is interrupt IN\n",i);

			else
				printk(KERN_INFO "EP %d is interrupt OUT\n",i);
		}
		
		if((epAttr & USB_ENDPOINT_XFERTYPE_MASK)==USB_ENDPOINT_XFER_ISOC)
		{
			if(epAddr & 0x80)
				printk(KERN_INFO "EP %d is isochronous IN\n",i);

			else
				printk(KERN_INFO "EP %d is isochronous OUT\n",i);
		}

	}
	
		
	if(interface->cur_altsetting->desc.bInterfaceClass == 0x08 && interface->cur_altsetting->desc.bInterfaceSubClass == 0x06 && interface->cur_altsetting->desc.bInterfaceProtocol== 0x50)
	{
		printk(KERN_INFO "SCSI commands supported.....\n");
		if(read_capacity_cmd(device,ep_in,ep_out))
			printk(KERN_ERR "Read capacity command failed\n");
						
	}
	else
		printk(KERN_INFO "SCSI commands not supported");
	
	return 0;	
}


static struct usb_driver usbdev_driver = {
	.name  = "usbdev",
	.probe = usbdev_probe,
	.disconnect = usbdev_disconnect,
	.id_table = usbdev_table, 
};



int device_init(void)
{
	int result = usb_register(&usbdev_driver);
	if(result)
		printk(KERN_ALERT "USB driver registration failed\n");
	else 
		printk(KERN_INFO "USB READ Capacity Driver Inserted\n");	
	return 0;
}

void device_exit(void)
{
	usb_deregister(&usbdev_driver);
	printk(KERN_NOTICE "Leaving Kernel\n");

}

MODULE_LICENSE("GPL");
module_init(device_init);
module_exit(device_exit);


