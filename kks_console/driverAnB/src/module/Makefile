include $(PWD)/Makefile.inc

obj-m               += $(MODULE_NAME).o
$(MODULE_NAME)-objs += dma.o
$(MODULE_NAME)-objs += dev_entry.o
$(MODULE_NAME)-objs += dev_pool.o
$(MODULE_NAME)-objs += pci_device_misc.o
$(MODULE_NAME)-objs += pci_device.o
$(MODULE_NAME)-objs += main.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	sudo rm -f *.mod.c *.ko *.o *.order 
	sudo rm -rf .tmp_versions/
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	
install: 
	sudo insmod $(MODULE_NAME).ko
    
remove:
	sudo rmmod $(MODULE_NAME).ko
        
