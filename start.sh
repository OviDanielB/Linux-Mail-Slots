make clean
make

sudo insmod mailslots.ko

sudo mknod /dev/mail0 c 246 0
sudo chmod 666 /dev/mail0
