#include"headers.h"
#include"declarations.h"


int send_command_to_dev(uint8_t*, uint8_t, uint32_t, uint8_t*);
int get_csw(uint8_t);

sector_t sectors_to_logical(sector_t sector)
{
	return sector >> (ilog2(SECTOR_SIZE) -9);
}


int test_unit_ready()
{

	uint8_t command[16] = {0};
	uint8_t tag;

	command[0] = 0x00; 				// read capacity

	// SEND COMMAND TO USB DEVICE
	if( send_command_to_dev(command, USB_EP_IN,0, &tag) != 0 ) {
		log("Send command error\n");
		log("resetting device .....\n");
		usb_reset_device(ginfo.device);
		return -1;
	}

	if ( get_csw(tag) == -1 ) {
		log("Device not ready\n");
		return -1;
	}

	return 0;

}


int write_10(sector_t start_sector, uint16_t sectors_to_xfer, uint8_t *buffer)
{
	uint8_t command[16] = {0};
	uint8_t tag;
	int ret;
	int size = 0;
	int retry = 0;

	memset(command,0,16);

	// WRITE(10)
	command[0] = 0x2A;
	put_unaligned_be32(start_sector,&command[2]);
	put_unaligned_be16(sectors_to_xfer,&command[7]);
	

	// SEND COMMAND TO USB DEVICE
	if( send_command_to_dev(command, USB_EP_OUT, sectors_to_xfer*SECTOR_SIZE, &tag) != 0 ) {
		log("Send command error\n");
		return -1;
	}

	// WRITING DATA TO THE DEVICE
	do {
		ret = usb_bulk_msg(ginfo.device, usb_sndbulkpipe(ginfo.device,ginfo.epout), buffer, sectors_to_xfer*SECTOR_SIZE, &size, 2000);
		retry++;
	}while( retry< MAX_RETRY && ret!=0 );

	if ( ret !=0 ) {
		log("Writing endpoint command error ::%d\n",ret);
		return -1;
	}
	else 
		log("Wrote %d bytes to device\n", size);


	if ( get_csw(tag) == -1 ) {
		log("cannot get command status block\n");
		return -1;
	}
	return 0;
}


int read_10(sector_t start_sector, uint16_t sectors_to_xfer, uint8_t *buffer)
{
	uint8_t command[16] = {0};
	uint8_t tag;
	int ret;
	int size = 0;
	int retry = 0;

	memset(command,0,16);

	// READ(10)
	command[0] = 0x28;
	put_unaligned_be32(start_sector,&command[2]);
	put_unaligned_be16(sectors_to_xfer,&command[7]);
	

	// SEND COMMAND TO USB DEVICE
	if( send_command_to_dev(command, USB_EP_IN, sectors_to_xfer*SECTOR_SIZE, &tag) != 0 ) {
		log("Send command error\n");
		return -1;
	}

	// READING DATA FROM DEVICE
	do {
		ret = usb_bulk_msg(ginfo.device, usb_rcvbulkpipe(ginfo.device,ginfo.epin), buffer, sectors_to_xfer*SECTOR_SIZE, &size, 5000);
		retry++;
		if(ret!=0)
			usb_clear_halt(ginfo.device, usb_rcvbulkpipe(ginfo.device,ginfo.epin));
	}while( retry<5 && ret!=0 );

	if ( ret !=0 ) {
		log("Reading endpoint command error ::%d\n",ret);
		return -1;
	}
	else 
	//	log("Read %d bytes from device\n", size);

	if ( get_csw(tag) == -1 ) {
		log("cannot get command status block\n");
		return -1;
	}
	return 0;
}




int get_csw(uint8_t tag)
{
	int retry = 0;
	int ret,size;
	struct command_status_wrapper *csw;

	if( !(csw = (struct command_status_wrapper*)kmalloc(sizeof(struct command_status_wrapper), GFP_KERNEL)) ) {
		log("Cannot allocate memory for command status buffer\n");
		return -1;
	}


	do {
		ret = usb_bulk_msg(ginfo.device,usb_rcvbulkpipe(ginfo.device, ginfo.epin), (unsigned char*)csw, 13, &size, 5000);
		if (ret != 0 ) 
			usb_clear_halt(ginfo.device, usb_sndbulkpipe(ginfo.device,ginfo.epin));
		retry++;
	} while ( (ret!=0) && (retry < MAX_RETRY));
	

	if( ret!=0 ) {
		log("csw read error:%d\n", ret);
		kfree(csw);
		return -1;
	}
		
	kfree(csw);
	return ret;
	
}


int send_command_to_dev(uint8_t *cmd, uint8_t dir, uint32_t data_len, uint8_t *rtag)
{
	struct command_block_wrapper *cbw = NULL;
	static uint32_t tag = 100; 							
	uint8_t cmd_len;
	int ret,retry =0;
	int size = 0;
	int pipe;
	
	// check if command array is valid or not
	if (cmd == NULL)
		return -1;
	
	pipe = usb_sndbulkpipe(ginfo.device, ginfo.epout);
	cbw = (struct command_block_wrapper*)kmalloc(sizeof(struct command_block_wrapper), GFP_KERNEL);
	if ( cbw == NULL ) {
		log("Cannot allocate memory\n");
		return -1;
	}
	memset(cbw,'\0',sizeof(struct command_block_wrapper));


	*rtag = tag;
	cmd_len = cmd_length[cmd[0]];
	cbw->dCBWSignature[0] = 'U';
	cbw->dCBWSignature[1] = 'S';
	cbw->dCBWSignature[2] = 'B';
	cbw->dCBWSignature[3] = 'C';
	cbw->dCBWTag = tag++;
	cbw->dCBWDataTransferLength = data_len;
	cbw->bmCBWFlags = dir;
	cbw->bCBWLUN = 0;
	cbw->bCBWCBLength = cmd_len;
	memcpy(cbw->CBWCB, cmd, cmd_len);

	// SENDING THE COMMAND
 	do {
		ret = usb_bulk_msg(ginfo.device, pipe, (unsigned char*)cbw, 31, &size, 2000);
		if( ret != 0 )
			usb_clear_halt(ginfo.device, pipe);
		retry++;
	} while( ret!=0 && (retry < MAX_RETRY) );


	if ( ret !=0 ) {
		log("send endpoint command error %d\n",ret);
		kfree(cbw);
		return -1;
	}

	kfree(cbw);
	return 0;

}


