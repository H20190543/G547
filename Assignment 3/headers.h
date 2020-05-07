#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/usb.h>
#include<linux/slab.h>
#include<linux/log2.h> 				// for log function
#include<linux/genhd.h>
#include<linux/blkdev.h>
#include<asm/unaligned.h> 			// for put_alligned func
#include<linux/spinlock.h> 			


#define log(fmt,...) ({ printk(KERN_INFO "[%s]:-->  ",__func__); \
		printk(fmt, ##__VA_ARGS__); \
})

#define log_err(fmt,...) ({ printk(KERN_ERR "[%s][%d]:--> ERROR: ",__func__, __LINE__); \
				printk(fmt, ##__VA_ARGS__); \
})

#define SANDISK_VID 		 0x0951
#define SANDISK_PID 		 0x1643
#define HP_VID 				 0x058f
#define HP_PID 				 0x6387
#define REQUEST_SENSE_LENGTH 0x12


#define SECTOR_SIZE 512

// read command attrib
#define READ_CMD 			0x28
#define READ_SIZE 			 512

// read capacity attrib
#define READ_CAPACITY_CMND  0x25
#define READ_CAPACITY_SIZE  0x08


// block device
#define DEVICE_NAME 		"usbDev"
#define MAJOR_NUM    		 251
#define CAPACITY 			 31948800
#define TEST_DEVICE_RETRY    4

#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])
#define MAX_RETRY 1

