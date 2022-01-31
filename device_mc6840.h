#ifndef DEVICE_MC6840_H
#define DEVICE_MC6840_H

#include "device.h"

/* Programmable Timer Module (PTM) */

struct m6840;
struct hw_device *m6840_create (unsigned long size);

/* User supplied */
extern void m6840_output_change(struct m6840 *ptm, uint8_t outputs);

#endif