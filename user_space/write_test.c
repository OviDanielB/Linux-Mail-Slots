#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>


int msgw(char* dev, char* s) {
    printf("start %s\n", dev);
    int fd = open(dev, O_RDWR );
    if (fd < 0) {
        printf("%s - err open\n", dev);
        fprintf( stderr, "errno is %d\n", errno );
        return -1;
    }
    char data[100];

    int ret;
    int l= (int)strlen(s);
    ret = write(fd, s, l);
    if (ret < 0) {
        printf("%s - err write\n", dev);
        fprintf( stderr, "errno is %d\n", errno );
        return -1;
    }


    close(fd);
}

int main(int argc, char **argv){
  msgw("/dev/mail0", "quaglia");
  msgw("/dev/mail0", "francesco");

}
