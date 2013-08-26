KDIR := /lib/modules/`uname -r`/build

obj-m := dragon.o

default:
	make -C $(KDIR) M=`pwd` modules

