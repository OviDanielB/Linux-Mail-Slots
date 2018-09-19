
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

int msgr(char* dev, char *param) {
    printf("start %s\n", dev);
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        printf("%s - err open\n", dev);
        return -1;
    }
    char data[100];

    int ret;


    ret = read(fd, data, 100);
    if (ret < 0) {
        printf("%s - err read\n", dev);
        fprintf( stderr, "errno is %d\n", errno );
        return -1;
    }
    data[ret]='\0';

    printf("Read from %s: %s\n", param, data);

    close(fd);
}

int main(int argc, char **argv){
  msgr("/dev/mail0", argv[1]);
}
