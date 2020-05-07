#ifndef DECLARATIONS_H
#define DECLARATIONS_H

enum ep_directions {
	USB_EP_IN = 0x80,
	USB_EP_OUT = 0x00
};


//////////////////////////////////////// data structures ///////////////////////////////////////
typedef struct blkdev_private 
{
	struct request_queue *rq;
	struct gendisk *gd;
	spinlock_t lock;
	struct workqueue_struct *dev_wq;
}blkdev;

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

typedef struct usb_dev_info {
	struct usb_device *device;
	uint8_t epin,epout;
	sector_t capacity;

}dev_info;


struct dev_work {
	struct work_struct work;
	struct request *req;
};



//////////////////////////////////////////////////////////////////////////////////////////////////


// global variables
extern blkdev *gdev;
extern dev_info ginfo;
extern uint8_t cmd_length[256];
extern int gmajor_num;
extern sector_t dev_capacity;


// function declarations
int add_usb_dev(void);
void request_function(struct request_queue*);
int test_unit_ready(void);
int read_10(sector_t, uint16_t, uint8_t*);
int write_10(sector_t, uint16_t, uint8_t*);

#endif
