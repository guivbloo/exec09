#include <string.h>
#include <stdlib.h>
 
 
#include "types.h"
#include "device.h"
#include "device_ram.h"


void ram_reset (struct hw_device *dev)
{
	memset (dev->priv, 0, dev->size);
}

uint8_t ram_read (struct hw_device *dev, unsigned long addr)
{
	char *buf = dev->priv;
	return buf[addr];
}

void ram_write (struct hw_device *dev, unsigned long addr, uint8_t val)
{
	char *buf = dev->priv;
	buf[addr] = val;
}

struct hw_class ram_class =
{
	.name = "RAM",
	.readonly = 0,
	.reset = ram_reset,
	.read = ram_read,
	.write = ram_write,
	.dump = NULL,
};

struct hw_device *ram_create (unsigned long size)
{
	void *buf = malloc (size);
	return device_create (&ram_class, size, buf);
}
