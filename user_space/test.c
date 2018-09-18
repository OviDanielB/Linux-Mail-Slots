#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <fcntl.h>

#include "../mailslot.h"

#define FILE_PATH "/dev/mail0"

int main(int argc, char **argv){
    int fd;
    char *file = "";

    fd = open(FILE_PATH, O_RDWR );
    if(fd < 0){
      fprintf(stderr, "Error in opening file %s\n", FILE_PATH );
      return -1;
    }
    printf("Opened file %s\n", FILE_PATH );

    close(fd);
}
