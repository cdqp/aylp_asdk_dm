
ifneq ($(KERNELRELEASE),)
obj-y := gph2x72c/ dpg0100/ dpg0101/
else
KDIR ?= /lib/modules/`uname -r`/build

default: all

all:
	$(MAKE) -C $(KDIR) M=$$PWD

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	
load:
	modprobe cp2x72c
	modprobe dpg0101

unload:
	rmmod cp2x72c dpg0101 dpg0100
	
uninstall:	unload
	rm -f /lib/modules/`uname -r`/extra/cp2x72c
	rm -f /lib/modules/`uname -r`/extra/dpg0101
	rm -f /lib/modules/`uname -r`/extra/dpg0100
	
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

endif

