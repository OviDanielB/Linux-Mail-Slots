make clean
make

sudo insmod mailslots.ko #minor_bott=10 minor_top=20

sudo mknod /dev/mail0 c 246 0
sudo chmod 666 /dev/mail0
