#ifndef M6809_DEVICE_H
#define M6809_DEVICE_H

#include "types.h"

/* This file defines structures used to build generic devices on a 6809. */


/* A hardware device structure exists for each physical device
in the machine */

struct hw_device;

/* A hardware class structure exists for each type of device.
It defines the operations that are allowed on that device.
For example, if there are multiple ROM chips, then there is
a single "ROM" class and multiple ROM device objects. */

struct hw_class
{
	/* Descriptive */
	char *name;

	/* Nonzero if the device is readonly */
	int readonly;

	/* Resets the device */
	void (*reset) (struct hw_device *dev);

	/* Reads a byte at a given offset from the beginning of the device. */
	uint8_t (*read) (struct hw_device *dev, unsigned long phy_addr);

	/* Writes a byte at a given offset from the beginning of the device. */
	void (*write) (struct hw_device *dev, unsigned long phy_addr, uint8_t val);

	/* Update procedure.  This is called periodically and can be used for
	whatever purpose.  The minimum update interval is once per 1ms.  Leave
	NULL if not required */
	void (*update) (struct hw_device *dev);

	/* Update procedure called at every tick or giving the number of ticks elapsed since 
	previous activation */
	void (*tick) (struct hw_device *dev, int cycles);
	
	/* Dump procedure. This can be used to get memory dump of the device */
	void (*dump) (struct hw_device *dev);
};


/* The hardware device structure exists for each instance of a device. */

struct hw_device
{
	/* A pointer to the class object.  This says what kind of device it is. */
	struct hw_class *class_ptr;

	/* The device ID assigned to it.  This is filled in automatically
	by the simulator. */
	unsigned int devid;

	/* The total size of the device in bytes. */
	unsigned long size;

	/* The private pointer, which is interpreted differently for each type
	(hw_class) of device. */
	void *priv;
};

struct hw_device *device_create (struct hw_class *class_ptr, unsigned int size, void *priv);

#endif /* _M6809_DEVICE_H */