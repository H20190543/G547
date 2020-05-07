obj-m := usb.o
usb-objs := main.o usb_commands.o block.o handle_request.o
all:
	$(MAKE)  -C /lib/modules/$(shell uname -r)/build  M=$(PWD) modules
	@rm *.o

clean:
	$(MAKE)  -C /lib/modules/$(shell uname -r)/build  M=$(PWD) clean
