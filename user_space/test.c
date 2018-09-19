#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <fcntl.h>

#include "../mailslot.h"

#define FILE_PATH "/dev/mail0"

int write_mess(int fd, char *s){
  int l, ret;

  l = (int) strlen(s);
  ret = write(fd,s,l);
  if(ret < 0){
    fprintf(stderr, "Error writing with errno %s.\n", errno );
    return EXIT_FAILURE;
  }
  return ret;
}


int read_mess(int fd){
  char data[100];
  int ret;

  ret = read(fd, data, 100);
  if (ret < 0) {
      fprintf(stderr, "Error reading with errno %d\n",errno);
      return NULL;
  }
  printf("Read %s\n", data );
  return 0;
}
int main(int argc, char **argv){
    int fd, i ;
    int max_mess_size, got;
    char *read;

    char *phrase[6] = { "Hello", "World", "I", "develop", "in", "C"};

    fd = open(FILE_PATH, O_RDWR );
    if(fd < 0){
      fprintf(stderr, "Error in opening file %s with errno %s\n", FILE_PATH, errno);
      return -1;
    }
    printf("Opened file %s\n", FILE_PATH );

    max_mess_size = 3;

    ioctl(fd, MS_IOC_SMESS_SIZE, &max_mess_size);

    for(i = 0; i < 6; i++){

      if(write_mess(fd, *(phrase + i)) == 0){
        printf("Message %s can't be written.\n", *(phrase + i));
      } else {
        printf("Written %s\n", *(phrase + i) );
      }
    }
    printf("\n\n\n" );
    for(i = 0; i < 6; i++){
      read_mess(fd);
    }

    close(fd);
}
