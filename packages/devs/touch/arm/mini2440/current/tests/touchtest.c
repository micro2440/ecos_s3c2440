#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

/* now declare (and allocate space for) some kernel objects,
   like the two threads we will use */
cyg_thread thread_s[1];		/* space for two thread objects */

char stack[1][4096];		/* space for two 4K stacks */

/* now the handles for the threads */
cyg_handle_t touch_test;

/* and now variables for the procedure which is the thread */
cyg_thread_entry_t simple_program;

/* and now a mutex to protect calls to the C library */
cyg_mutex_t cliblock;

#define TOUCHDEVICE	"/dev/ts"			/* iPAQ*/
static int pd_fd = -1;

static int PD_Open()
{
 	/*
	 * open up the touch-panel device.
	 * Return the fd if successful, or negative if unsuccessful.
	 */
    diag_printf("Opening touch panel...\n");
	pd_fd = open(TOUCHDEVICE, O_RDONLY | O_NONBLOCK);
	if (pd_fd < 0) {
		diag_printf("Error  opening touch panel\n");
		return -1;
	}
	return pd_fd;
}

static int PD_Read(short *px, short *py, short *pz, int *pb)
{
	/* read a data point */
	short data[4];
	int bytes_read;

	bytes_read = read(pd_fd, data, sizeof(data));

	if (bytes_read != sizeof(data)) {
		if (errno == EINTR || errno == EAGAIN)
			return 0;
		/*
		 * kernel driver bug: select returns read available,
		 * but read returns -1
		 * we return 0 here to avoid GsError above
		 */
		/*return -1;*/
		return 0;
	}

	*px = (short)data[1];
	*py = (short)data[2];

	*pb = (data[0] ? 1 : 0);

	*pz = 0;

	if(! *pb )
	  return 3;			/* only have button data */
	else 
	  return 2;			/* have full set of data */
}


/* we install our own startup routine which sets up threads */
void cyg_user_start(void)
{
  printf("Entering touch test' cyg_user_start() function\n");
  cyg_thread_create(4, simple_program, (cyg_addrword_t) 0,
		    "Touch Test", (void *) stack[0], 4096,
		    &touch_test, &thread_s[0]);
  cyg_thread_resume(touch_test);
}

/* this is a simple program which runs in a thread */
void simple_program(cyg_addrword_t data)
{
  int message = (int) data;
  int delay;
  short px,py,pz,pb;

  printf("Beginning execution; thread data is %d\n", message);

  PD_Open();
  for (;;) {
    cyg_mutex_lock(&cliblock); {
      printf("Thread %d: and now a delay of %d clock ticks\n",
	     message, delay);
    }
	PD_Read(&px,&py,&pz,&pb);
	diag_printf("x = %d, y = %d, pressed = %d\n",px ,py ,pb);
    cyg_mutex_unlock(&cliblock);
    cyg_thread_delay(delay);
  }
}
