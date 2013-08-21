KNJN Dragon PCI Linux drivers
=============================

IO			IORESOURCE_IO driver (64 bytes)
MEM 			IORESOURCE_MEM driver (64 Kbytes)
dragon_pci_test.c	Test program
meduse.txt, larose.txt	Sample data to send to Dragon with cat + dd
RTDM			RTDM drivers (IO) for Xenomai

0- Introduction

Here are 2 Linux drivers and 1 RTDM driver for KNJN Dragon PCI boards.

Of course you should first load the PCI/P&P bitstream to the 
board (Projects\ -\ PCI\ P\&P/HDL/PCI_PnP.100K.bit). 

As described in HDL source file (PCI_PnP.v), design includes definition 
for both IO and MEM PCI space using BAR0 and BAR1.

// Choose which PCI space you want to use, i.e. define one (or both) of 
//the following
`define PCI_IOSPACE   // implemented using BAR0
`define PCI_MEMSPACE  // implemented using BAR1

PCI interrupt is not enabled by default in the design.

I- Standard "Linux" driver
--------------------------

1- Building + installing

$ make

You can install modules (.ko) + test program with:

# make install

2- Loading driver

You can load driver with:

# insmod dragon_pci_io.ko

or

# modprobe dragon_pci_io (if driver was installed with "make install")

Once driver is loaded you should see output in kernel log (use "dmesg") :

# insmod  dragon_pci_io.ko                                                      
dragon_pci_io: found 100:0                                                      
dragon_pci_io: using major 254 and minor 0 for this device                      
dragon_pci_io: BAR 0 (0x00ec00-0x00ec3f), len=64, flags=0x020101                
dragon_pci_io: I/O memory 0x00ec00 (length 64) has been saved                   
dragon_pci_io: BAR 1 (0xd6000000-0xd600ffff), len=65536, flags=0x020200         
dragon_pci_io: no IRQ!                             


<WARNING>

Please note you *can't* load dragon_pci_io and dragon_pci_mem at the same 
time. Of course it's possible to build a single driver for both IO *and* MEM 
but our main goal was to provide simpler source code in 2 files. 
More info about Linux drivers is available at the following URL (LDD3) :

http://lwn.net/images/pdf/LDD3

</WARNING>

You should remove previous driver then load new driver:

# rmmod dragon_pci_io
# modprobe dragon_pci_mem

dragon_pci_mem: found 100:0                                                     
dragon_pci_mem: using major 254 and minor 0 for this device                     
dragon_pci_mem: BAR 0 (0x00ec00-0x00ec3f), len=64, flags=0x020101               
dragon_pci_mem: BAR 1 (0xd6000000-0xd600ffff), len=65536, flags=0x020200        
pci: I/O memory has been remaped at 0xcf8a0000                                  
dragon_pci_mem: no IRQ!   


Once driver is loaded you should get dynamic major value from /proc/devices 
then create device node.

# grep dragon_pci /proc/devices                       
254 dragon_pci_io
# mknod /dev/dragon c 254 0

3- Testing

Program "dragon_pci_test" sends N x long words (32 bits) to Dragon board.

# dragon_pci_test
Usage: dragon_pci_test dragon_device_name addr count [data inc]

# dragon_pci_test /dev/dragon 0 2 0 <- write 2 x 0x0 @ 0x0
buf[0] = 0x00000000
buf[1] = 0x11111111
Wrote 8 chars
Read 8 chars
buf[0] = 0x00000000
buf[1] = 0x11111111

You can also use "hexdump" od "od" to check result:

# hexdump -C /dev/dragon  | more
00000000  00 00 00 00 11 11 11 11  22 22 22 22 33 33 33 33  |........""""3333|
...

# od -x /dev/dragon  | more
0000000 0000 0000 1111 1111 2222 2222 3333 3333
0000020 4444 4444 5555 5555 6666 6666 7777 7777
...


You can use cat + dd commands to send raw data to Dragon. The following 
command sends 32 bytes from meduse.txt file (a very famous song by 
french singer Georges Brassens).

# cat meduse.txt | dd bs=32 count=1 > /dev/dragon 
1+0 records in
1+0 records out
32 bytes (32B) copied, 0.008942 seconds, 3.5KB/s

Then you can check result with "hexdump" :

# hexdump -C /dev/dragon  | more
00000000  4e 6f 6e 2c 20 63 65 20  6e 27 65 74 61 69 74 20  |Non, ce n'etait |
00000010  70 61 73 20 6c 65 20 72  61 64 65 61 75 20 44 65  |pas le radeau De|

II- RTDM driver for Xenomai
---------------------------

RTDM (Real Time Driver Model) is designed for use with Linux real-time 
extension such as Xenomai (http://www.xenomai.org). RTDM driver is a
Linux module (.ko) but is based on dedicated API to run with Xenomai co-kernel
instead of Linux kernel. As an example, kernel function "copy_from_user" is 
replaced by "rtdm_copy_to_user". Full RTDM documentation is available from:

http://www.xenomai.org/documentation/trunk/html/api/group__rtdm.html

Using RTDM driver needs a Xenomai kernel based on the ADEOS micro-kernel patch 
and Xenomai extension. You can get the latest Xenomai+ADEOS distribution from
http://www.xenomai.org.

Current example is based on a RTDM driver + Xenomai application. The application
is a periodic task which write 0/1 to the Dragon PCI board in order to change
LED value. The result of the application is a blinking LED + "jitter" display.

Building driver and application needs a Xenomai environnement to be installed
on the host computer. Xenomai compilation is based on "xeno-config" utility 
provided with Xenomai distribution. Access to "xeno-config" should be added to
the PATH variable.

$ export DESTDIR=~/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/staging
$ export PATH=$PATH:$DESTDIR/usr/xenomai/bin
$ xeno-config --skin=posix --cflags
-I/home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/staging/usr/xenomai/include -D_GNU_SOURCE -D_REENTRANT -Wall -pipe -D__XENO__ -I/home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/staging/usr/xenomai/include/posix

Once "xeno-config" is usable, you just have to type "make" :

$ make
make[1]: entrant dans le répertoire « /home/pierre/developpement/Perso/KNJN/PF/PCI/RTDM/driver »
make -C /home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/build/linux-2.6.34.5 SUBDIRS=/home/pierre/developpement/Perso/KNJN/PF/PCI/RTDM/driver modules
make[2]: entrant dans le répertoire « /home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/build/linux-2.6.34.5 »
  CC [M]  /home/pierre/developpement/Perso/KNJN/PF/PCI/RTDM/driver/pci_io_rtdm.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/pierre/developpement/Perso/KNJN/PF/PCI/RTDM/driver/pci_io_rtdm.mod.o
  LD [M]  /home/pierre/developpement/Perso/KNJN/PF/PCI/RTDM/driver/pci_io_rtdm.ko
make[2]: quittant le répertoire « /home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/build/linux-2.6.34.5 »
make[1]: quittant le répertoire « /home/pierre/developpement/Perso/KNJN/PF/PCI/RTDM/driver »
make[1]: entrant dans le répertoire « /home/pierre/developpement/Perso/KNJN/PF/PCI/RTDM/posix »
/home/pierre/x-tools/i586-glibc/bin/i586-crosstools-linux-gnu-gcc --sysroot=/home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/staging -o xenomai_pci xenomai_pci.c -I/home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/staging/usr/xenomai/include -D_GNU_SOURCE -D_REENTRANT -Wall -pipe -D__XENO__ -I/home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/staging/usr/xenomai/include/posix -g -Wl,@/home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/staging/usr/xenomai/lib/posix.wrappers -L/home/pierre/Enseignement/cours-epita-pf/2011/GISTR/LIRT/KNJN/buildroot-ow/output/staging/usr/xenomai/lib -lpthread_rt -lxenomai -lpthread -lrt  -g -lrtdm -I../driver/
make[1]: quittant le répertoire « /home/pierre/developpement/Perso/KNJN/PF/PCI/RTDM/posix »

Once driver and application are available, you can test them on the target
board:

# insmod pci_io_rtdm.ko                                                         
PCI IO RTDM driver loading                                                      
pci_io_pci_probe: found 100:0                                                   
pci_io_pci_probe: IO resource base address : ec00                               

# xenomai_pci  -p 1000000 -k
Using RTDK                                                                   
pci_io_open: opening device 0
== Period: 1000 us                                                   
DOMAIN SWITCH !!                                                              
Loop= 2000 dt= 0 999759 (-241 ns)
Loop= 4000 dt= 0 999759 (-241 ns)
Loop= 6000 dt= 0 999759 (-241 ns) 
Loop= 8000 dt= 0 999759 (-241 ns)
Loop= 10000 dt= 0 1000597 (597 ns)
...



Enjoy.

Pierre Ficheux
pierre.ficheux@gmail.com

08/2011
