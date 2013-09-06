sudo insmod ./dragon.ko
sleep 1
sudo mknod /dev/dragon c 251 0
sleep 1
sudo chown blin:blin /dev/dragon 
sleep 1
./test_dragon /dev/dragon --set-low 555
