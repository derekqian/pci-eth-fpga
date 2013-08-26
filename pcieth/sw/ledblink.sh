#!/bin/sh
#
X=0

./test_dragon /dev/dragon --set-mode 1

while [ 1 ]
do
    ./test_dragon /dev/dragon --set-led $X

    # Switch b0
    X=$(expr $X + 1)

    # Blink every sec
    sleep 1
done
