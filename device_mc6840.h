#ifndef DEVICE_MC6840_H
#define DEVICE_MC6840_H

#include "device.h"

/* Programmable Timer Module (PTM) */


struct mc6840_port
{
	uint8_t status_register;	
	unsigned long prev_cycles;
	unsigned int int_line;  /* Which interrupt to signal */
	uint8_t control_register[3];
	int reload[3]; /* Value to reload into the timer when it reaches zero */
	int count[3]; /* The current value of the timer */
	int status_read_since_int[3];
};



/* The I/O registers exposed by this driver */
#define HWT_COUNT     0  /* The 8-bit timer counter */
#define HWT_RELOAD    1  /* The 8-bit reload counter */
#define HWT_FLAGS     2  /* Misc. flags */
#define HWTF_INT   0x80   /* Generate interrupt at zero */



struct hw_device *mc6840_create (unsigned int int_line);


#endif