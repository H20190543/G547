#include"headers.h"
#include"declarations.h"


void delayed_work_function(struct work_struct*);
int send_data(struct request*);	


void request_function(struct request_queue *q)
{
	struct request *req;
	struct dev_work *usb_work = NULL;

	while( (req = blk_fetch_request(q)) != NULL ) {

		usb_work = (struct dev_work*)kmalloc(sizeof(struct dev_work), GFP_ATOMIC);  // we do not want this allocation to sleep
		if ( usb_work == NULL ) {
			log_err("Memory allocation for deferred work failed\n");
			 __blk_end_request_all(req, 0); 		
			continue;
		}

		usb_work->req = req;
		INIT_WORK(&usb_work->work, delayed_work_function);
		queue_work( gdev->dev_wq, &usb_work->work);
	}

}

void delayed_work_function(struct work_struct *work)
{
	struct dev_work *usb_work = NULL;
	struct request *cur_req = NULL;
	int ret = 0;
	unsigned long flags; 				// for spinlock

	usb_work = container_of(work, struct dev_work, work); 		// retriving my dev_work struct
	cur_req = usb_work->req;

	ret = send_data(cur_req);

	spin_lock_irqsave( &gdev->lock, flags); 		// here pdev is our structre for block devices that contained a spin lock
	
	__blk_end_request_all(cur_req, ret);
	

	spin_unlock_irqrestore( &gdev->lock, flags);

	kfree(usb_work);
	return;
}

int send_data(struct request *cur_req)
{
	int ret = 0;
	int direction = rq_data_dir(cur_req);
	sector_t dev_sector = blk_rq_pos(cur_req); 	// starting device sector for this request
	unsigned int total_sectors = blk_rq_sectors(cur_req); // number of sectors to process

	struct bio_vec bvec;
	struct req_iterator iter;
	sector_t sector_offset = 0;
	unsigned int bvec_sectors;
	uint8_t *buffer = NULL;
	uint8_t *temp_buffer = NULL;

	log("DISK REQUEST ATTRIBS: data direction: %d  start sector: %d   num_of_sectors_to_transfer: %d\n", direction,(int)dev_sector, total_sectors);

	rq_for_each_segment(bvec,cur_req,iter) {
		bvec_sectors = bvec.bv_len / SECTOR_SIZE; 
		temp_buffer = (uint8_t*)kmalloc(total_sectors*SECTOR_SIZE,GFP_ATOMIC);
		if(!temp_buffer) {
			log_err("Cannot allocate memoory to temp buffer\n");
			return -1;
		}

		if (direction == 0 ) {
			log("Read Request\n");

			if(read_10(iter.iter.bi_sector, bvec_sectors, temp_buffer) == 0)
				log("Read Success\n"); 
			else
				log_err("Read Failure\n");
			
			buffer = __bio_kmap_atomic(iter.bio, iter.iter);
			memcpy(buffer,temp_buffer,bvec_sectors*SECTOR_SIZE);
			__bio_kunmap_atomic(buffer);
			
		}
		else {
			log("Write request\n");
			buffer = __bio_kmap_atomic(iter.bio, iter.iter);
			memcpy(temp_buffer,buffer,bvec_sectors*SECTOR_SIZE);
			__bio_kunmap_atomic(buffer);
			if(write_10(iter.iter.bi_sector, bvec_sectors, temp_buffer) == 0)
				log("Write Success\n"); 
			else
				log_err("Write Failure\n");
		}
		
		kfree(temp_buffer);
		sector_offset += bvec_sectors; 
	}

	if ( sector_offset == total_sectors ) 
		ret = 0;
	else {
		log_err("cannot transfer all the requested sectors\n");
		ret = -EIO;
	}
	return ret;
}
