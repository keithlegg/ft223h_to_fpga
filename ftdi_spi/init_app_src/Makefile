TOPDIR  := $(shell cd ../..; pwd)
include $(TOPDIR)/Rules.make

#the main shebang
APP = init_app


all: $(APP)

$(APP): main.c	
	$(CC) main.c -o $(APP) $(CFLAGS)	
	
clean:
	rm -f *.o ; rm $(APP)

reset:
	sudo rmmod ftdi_sio;sudo rmmod usbserial 
	