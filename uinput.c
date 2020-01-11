/**** uinput.c *****************************/
/* M. Moller   2013-01-16                  */
/*   Universal RPi GPIO keyboard daemon    */
/*******************************************/

/*
   Copyright (C) 2013 Michael Moller.
   This file is part of the Universal Raspberry Pi GPIO keyboard daemon.

   This is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The software is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
//#include <linux/uinput.h>
#include "uinput_linux.h"
//#include "config.h"
//include "daemon.h"
#include "uinput.h"
#include <time.h>

/****************************************************************
 * Defines
 ****************************************************************/

#define DEBUG_UINPUT_PRINTF (0)
#if DEBUG_UINPUT_PRINTF
  #define UINPUT_PRINTF(...) printf(__VA_ARGS__);
#else
  #define UINPUT_PRINTF(...)
#endif
    
#define SLEEP_TIME_SEND_STOP_KEY_US   20*1000

#define die(str, args...) do { \
        perror(str); \
        return(EXIT_FAILURE); \
    } while(0)


/****************************************************************
 * Static functions declaration
 ****************************************************************/
static int sendRel(int dx, int dy);
static int sendSync(void);

/****************************************************************
 * Static variables
 ****************************************************************/
static struct input_event     uidev_ev;
static int uidev_fd;
/*static keyinfo_s lastkey;*/

/****************************************************************
 * Functions Implementation
 ****************************************************************/
int init_uinput(void)
{
  int fd;
  struct uinput_user_dev uidev;
  int i;

  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if(fd < 0)
    die("/dev/uinput");

  if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
    die("error: ioctl");
  if(ioctl(fd, UI_SET_EVBIT, EV_REP) < 0)
    die("error: ioctl");
  if(ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0)
    die("error: ioctl");

  if(ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)
    die("error: ioctl");
  if(ioctl(fd, UI_SET_RELBIT, REL_X) < 0)
    die("error: ioctl");
  if(ioctl(fd, UI_SET_RELBIT, REL_Y) < 0)
    die("error: ioctl");

  /* don't forget to add all the keys! */
  for(i=0; i<256; i++){
    if(ioctl(fd, UI_SET_KEYBIT, i) < 0)
      die("error: ioctl");
  }

  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-sample");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor  = 0x1;
  uidev.id.product = 0x1;
  uidev.id.version = 1;

  /*if(write(fd, &uidev, sizeof(uidev)) < 0)
    die("error: write");*/
  
  if(ioctl(fd, UI_DEV_SETUP, &uidev) < 0)
	die("error: ioctl");
	
  if(ioctl(fd, UI_DEV_CREATE) < 0)
    die("error: ioctl");

  uidev_fd = fd;
  
  sleep(1);

  return 0;
}

int test_uinput(void)
{
  int                    dx, dy;
  int                    i,k;

  //srand(time(NULL));
  int op = 1;

  while(1) {
    //switch(rand() % 4) {
    switch(op) {
    case 0:
      dx = -10;
      dy = -1;
      break;
    case 1:
      dx = 10;
      dy = 1;
      break;
    case 2:
      dx = -1;
      dy = 10;
      break;
    case 3:
      dx = 1;
      dy = -10;
      break;
    }

    /*k = send_gpio_keys(21, 1);
    sendKey(k, 0);*/
    sendKey(KEY_D, 0);

    for(i = 0; i < 20; i++) {
      //sendKey(KEY_D, 1);
      //sendKey(KEY_D, 0);
      sendRel(dx, dy);

      usleep(15000);
    }

    sleep(10);
  }
}

int close_uinput(void)
{
  sleep(2);

  if(ioctl(uidev_fd, UI_DEV_DESTROY) < 0)
    die("error: ioctl");

  close(uidev_fd);

  return 0;
}

int sendKey(int key, int value)
{
  struct input_event ie;
  //memset(&uidev_ev, 0, sizeof(struct input_event));
  //gettimeofday(&uidev_ev.time, NULL);
  ie.type = EV_KEY;
  ie.code = key;
  ie.value = value;
  ie.time.tv_sec = 0;
  ie.time.tv_usec = 0;
  UINPUT_PRINTF("sendKey: %d = %d\n", key, value);
  if(write(uidev_fd, &ie, sizeof(struct input_event)) < 0)
    die("error: write");

  sendSync();

  return 0;
}

int sendKeyAndStopKey(int key)
{
  sendKey(key, 1);
  usleep(SLEEP_TIME_SEND_STOP_KEY_US);
  sendKey(key, 0);

  return 0;
}

static int sendRel(int dx, int dy)
{
  memset(&uidev_ev, 0, sizeof(struct input_event));
  uidev_ev.type = EV_REL;
  uidev_ev.code = REL_X;
  uidev_ev.value = dx;
  if(write(uidev_fd, &uidev_ev, sizeof(struct input_event)) < 0)
    die("error: write");

  memset(&uidev_ev, 0, sizeof(struct input_event));
  uidev_ev.type = EV_REL;
  uidev_ev.code = REL_Y;
  uidev_ev.value = dy;
  if(write(uidev_fd, &uidev_ev, sizeof(struct input_event)) < 0)
    die("error: write");

  sendSync();

  return 0;
}

static int sendSync(void)
{
  UINPUT_PRINTF("sendSync\n");
  //memset(&uidev_ev, 0, sizeof(struct input_event));
  struct input_event ie;
  ie.type = EV_SYN;
  ie.code = SYN_REPORT;
  ie.value = 0;
  ie.time.tv_sec = 0;
  ie.time.tv_usec = 0;
  if(write(uidev_fd, &ie, sizeof(struct input_event)) < 0)
    die("error: sendSync - write");

  return 0;
}

