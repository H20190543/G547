#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/random.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "chardev.h"

static dev_t first;

static struct cdev c_dev;
static struct class *cls;
static short int num;
static int allignment;
static int channel;

//step 4: Driver callback functions

static int my_open(struct inode *i, struct file *f)
{
	printk(KERN_INFO "ADC: open()\n");
	return 0;
}

static int my_close(struct inode *i, struct file *f)
{
	printk(KERN_INFO "ADC: open()\n");
	return 0;
}


static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "ADC: read()\n");
	get_random_bytes(&num,4);
	num=(num & 0x01FF);
	if(allignment==1)
	{
	printk(KERN_INFO "High type allignment\n");
	num=((num<<6) & 0xFFC0);
	}
	else
	{
	printk(KERN_INFO "Low type allignment\n");
	}
	copy_to_user(buf, &num, 3);
	return 0;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "ADC: write()\n");
	
	return 0;
}

static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case ALLIGNMENT:
		allignment = arg;
		printk(KERN_INFO "Setting allignment\n");
		break;
		case CHANNEL:
		channel = arg;
		printk(KERN_INFO "Set Channel to %d\n", channel);
		break;
		default:
		return -ENOTTY;
	}
	return 0;
}

static const struct file_operations fops =
				{
					.owner = THIS_MODULE,
					.open = my_open,
					.release = my_close,
					.read = my_read,
					.write = my_write,
					.unlocked_ioctl = my_ioctl
				};

static int __init mychar_init(void)
{
	printk(KERN_INFO "Assignment 1");
	//step 1:reserve <major, minor>
	if(alloc_chrdev_region(&first, 0, 1, "ADC")<0)
	{
		return -1;
	}
	//Step 2: creation of device file
	if((cls=class_create(THIS_MODULE, "chardev"))==NULL)
	{
		unregister_chrdev_region(first, 1);
		return -1;
	}
	if(device_create(cls, NULL, first, NULL, "ADC8")==NULL)
	{
		class_destroy(cls);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	//step 3: link fops and cdev to the device node
	cdev_init(&c_dev, &fops);
	if(cdev_add(&c_dev, first, 1)==-1)
	{
		device_destroy(cls, first);
		class_destroy(cls);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	return 0;
}

static void __exit mychar_exit(void)
{
	cdev_del(&c_dev);
	device_destroy(cls, first);
	class_destroy(cls);
	unregister_chrdev_region(first, 1);
	printk(KERN_INFO "ADC8 unregistered\n\n");
}

module_init(mychar_init);
module_exit(mychar_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ADC");
MODULE_AUTHOR("AMAN PANDEY");

