#include <stdlib.h>
#include "device.h"


/**
 * @brief Creates a new hardware device instance.
 *
 * Allocates and initializes a hardware device structure of the specified size,
 * associates it with the given hardware class, and attaches private data.
 *
 * @param class_ptr Pointer to the hardware class structure to associate with the device.
 * @param size Size (in bytes) of the device structure to allocate.
 * @param priv Pointer to private data to be associated with the device.
 * @return Pointer to the newly created hw_device structure, or NULL on failure.
 */
struct hw_device *device_create (struct hw_class *class_ptr, unsigned int size, void *priv)
{
	struct hw_device *dev = malloc (sizeof (struct hw_device));
	dev->class_ptr = class_ptr;
	dev->size = size;
	dev->priv = priv;
	return dev;
}