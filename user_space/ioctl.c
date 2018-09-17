#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <fcntl.h>

#include "../mailslot.h"

int main() {
    char* dev="/dev/mail0\0";
    char* s="ciao\0";
    int mess_size, blocking_read, blocking_write, max_storage;
    printf("start %s\n", dev);
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        printf("%s - err open\n", dev);
        return -1;
    }
    char data[100];


    int l= (int)strlen(s);

    ioctl(fd,MS_IOC_GMESS_SIZE, &mess_size);
    printf("Got MESS_SIZE = %d\n", mess_size);

    mess_size++;
    ioctl(fd, MS_IOC_SMESS_SIZE, &mess_size);
    printf("Set MESS_SIZE = %d\n", mess_size);

    ioctl(fd, MS_IOC_GBLOCKING_READ, &blocking_read);
    printf("Got BLOCKING_READ = %d\n", blocking_read);

    blocking_read = MS_BLOCK_DISABLED;
    ioctl(fd, MS_IOC_SBLOCKING_READ, &blocking_read);
    printf("Set BLOCKING_READ = %d\n", blocking_read);

    ioctl(fd, MS_IOC_GBLOCKING_WRITE, &blocking_write);
    printf("Got BLOCKING_WRITE = %d\n", blocking_write);

    blocking_write = MS_BLOCK_DISABLED;
    ioctl(fd, MS_IOC_SBLOCKING_WRITE, &blocking_write);
    printf("Set BLOCKING_WRITE = %d\n", blocking_write);

    ioctl(fd, MS_IOC_GMAX_STORAGE, &max_storage);
    printf("Got Max storage = %d\n", max_storage);

    max_storage = max_storage + 10;
    ioctl(fd, MS_IOC_SMAX_STORAGE, &max_storage);
    printf("Set Max storage = %d\n", max_storage);



    close(fd);
    return 0;
}
