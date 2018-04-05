#include "reader.h"
#include <linux/input.h>
#include <signal.h>

#define BUFF_SIZE 64
#define MAX_BUTTON 9

unsigned char quit = 0;

void user_signal(int sig){
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
    for( i=1; i<MAX_BUTTON; i++)
      shmaddr[i] = push_sw_buff[i];
  }
  close(readKeyDesc);
  close(swButtonDesc);
}