KDIR= /lib/modules/$(shell uname -r)/build

PWD= $(shell pwd)

obj-m := dragon_pci_io.o

all: $(USER_PROG)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules_install

clean:
	rm -f *~ Module.markers Modules.symvers
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

