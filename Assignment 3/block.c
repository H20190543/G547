#include"headers.h"
#include"declarations.h"


static int bdev_open(struct block_device *, fmode_t);
static void bdev_close(struct gendisk *, fmode_t);


// block devices operations
struct block_device_operations dops =
{
	.owner = THIS_MODULE,
	.open = bdev_open,
	.release = bdev_close
};


static int bdev_open(struct block_device *device, fmode_t mode)
{
	printk(KERN_INFO "Open function called\n");
	return 0;
}

static void bdev_close(struct gendisk *gd, fmode_t mode)
{
	printk(KERN_INFO "Release function called\n");
}



int add_usb_dev()
{
	// driver registration
	gmajor_num = register_blkdev(0,DEVICE_NAME);
	if( gmajor_num < 0 ) {
		log("Registering block driver failed\n");
		return -EBUSY;
	}
	else
		log("Registered driver with major number: %d\n", gmajor_num);


	// Memory allocation to my private structure of device
	gdev = kmalloc(sizeof(blkdev), GFP_KERNEL);
	if(gdev == NULL) {
		log("Cannot allocate memory to private device structure\n");
		unregister_blkdev(gmajor_num,DEVICE_NAME);
		return -ENOMEM;
	}
	memset(gdev,0,sizeof(struct blkdev_private));
	spin_lock_init(&gdev->lock); 									
	gdev->rq = blk_init_queue(request_function,&gdev->lock);
	if(!gdev->rq) {
		log_err("Cannot initialize init queue\n");
		return -1;
	}


	gdev->gd = alloc_disk(2); 									// use macro
	if( gdev->gd == NULL ) {
		log("alloc disk failure\n");
		unregister_blkdev(gmajor_num,DEVICE_NAME);
		blk_cleanup_queue(gdev->rq);
		kfree(gdev);
		return -1;
	}
	gdev->dev_wq = create_workqueue("bdev_queue"); //workqueue for deffering the work

	// fill gendisk structure
	gdev->gd->major = gmajor_num;
	gdev->gd->first_minor = 0;
	gdev->gd->fops = &dops;
	gdev->gd->queue = gdev->rq;
	gdev->gd->private_data = gdev;
	strcpy(gdev->gd->disk_name, DEVICE_NAME);
	set_capacity(gdev->gd,ginfo.capacity);
	
	add_disk(gdev->gd);
	log("Driver registered successfully!! with capacity %ld\n", get_capacity(gdev->gd));
	log("ENDED\n");

	return 0;
}
