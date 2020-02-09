#include <stdio.h> 
#include<stdlib.h>
#include<fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include<sys/ioctl.h>
#include "chardev.h"

int main()

{ 
  int fd;
  short int num;
  int channel,allignment;
  fd=open("/dev/ADC8", O_RDWR);
  
  if(fd  == -1)
  {
  
    printf("cannot read source file adc\n");
    return 0;
  }

    printf("Enter the ADC Channel No. (0-7):\n");
    scanf("%d",&channel);
    printf("Writing Value to Driver\n");
    ioctl(fd, CHANNEL, (int32_t*) &channel); 

    printf("Enter the Allignment No. (0:Lower Bit Allignment, 1:Higher Bit Allignment):\n");
    scanf("%d",&allignment);
    printf("Writing Value to Driver\n");
    ioctl(fd, ALLIGNMENT, (int32_t*) &allignment); 
  
  read(fd,&num,2);
  printf("adc value: %d\n",num);

  close(fd);
  
  
  return 0;
}
