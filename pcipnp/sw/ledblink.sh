#!/bin/sh
#
# KNJN Dragon PCI led blink using PCI driver (IO)
#
#
X=0

while [ 1 ]
do
    ./dragon_pci_test /dev/dragon 0 1 $X 0

    # Switch b0
    X=$(expr $X + 1)

    # Blink every sec
    sleep 1
done
