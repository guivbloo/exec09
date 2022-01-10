#include <stdlib.h>
#include "device.h"

/**
 * Create a device
 */
struct hw_device *device_create (struct hw_class *class_ptr, unsigned int size, void *priv)
{
	struct hw_device *dev = malloc (sizeof (struct hw_device));
	dev->class_ptr = class_ptr;
	dev->size = size;
	dev->priv = priv;
	return dev;
}