#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_DATA_SIZE 1024

char read_buf[MAX_DATA_SIZE];
char write_buf[MAX_DATA_SIZE];

int main()
{
  printf("*** Example driver that writes and reads to driver ***\n");

  int fd = open("/dev/example_device", O_RDWR);

  if (fd < 0) {
    printf("Cannot open device file...\n");

    return -1;
  }

  char option;
  while (1) {
    printf("*** Use one of following options ***\n");
    printf("  1. Write...\n");
    printf("  2. Read...\n");
    printf("  3. Exit...\n");

    scanf("%c", &option);
    printf("Your option is: %c\n", option);

    switch (option) {
      case '1':
        printf("Enter message to write to the driver: ");
        scanf("  %[^\t\n]s", write_buf);
        printf("Writing data...\n");
        write(fd, write_buf, strlen(write_buf) + 1);
        printf("Writing done!\n");
        break;
      
      case '2':
        printf("Reading data...\n");
        read(fd, read_buf, MAX_DATA_SIZE);
        printf("Reading done!\n");
        printf("Data = %s\n\n", read_buf);
        break;

      case '3':
        close(fd);
        exit(1);
        break;

      default:
        printf("Enter Valid option = %c\n",option);
        break;
    }
  }


  close(fd);
}