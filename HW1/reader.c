#include "reader.h"
#include <linux/input.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>
#include <dirent.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>

#define BUFF_SIZE 64
//#define MAX_BUTTON 9

#define KEY_PRESS 1
#define KEY_RELEASE 0

unsigned char quit = 0;

void user_signal1(int sig){
  quit = 1;
}

void readFromDevice(int* shmaddr){
  struct input_event ev[BUFF_SIZE];
  int readKeyDesc, swButtonDesc;
  int size = sizeof(struct input_event);

  unsigned char push_sw_buff[MAX_BUTTON];

  char *readKeyDevice = "/dev/input/event0";
  char *swButtonDevice = "/dev/fpga_push_switch";

  if( (readKeyDesc = open(readKeyDevice, O_RDONLY | O_NONBLOCK)) < 0 ){
    close(readKeyDesc);
    perror("readKey device file");
    exit(1);
  }
  if( (swButtonDesc = open(swButtonDevice, O_RDWR)) < 0 ){
    close(swButtonDesc);
    perror("swButton device file");
    exit(1);
  }

  (void)signal(SIGINT, user_signal1);

  while(!quit){
    int i;
    int rd, swBuffSize = sizeof(push_sw_buff);

    /* readkey */
    if( (rd = read(readKeyDesc, ev, size*BUFF_SIZE)) >= size ){
      int value = ev[0].value;

      if( value == KEY_PRESS )
        shmaddr[0] = ev[0].code;
      else
        shmaddr[0] = 0;
    }

    /* sw button */
    read(swButtonDesc, &push_sw_buff, swBuffSize);
    for( i=1; i<1+MAX_BUTTON; i++)
      shmaddr[i] = push_sw_buff[i-1];
  }
  close(readKeyDesc);
  close(swButtonDesc);
}
