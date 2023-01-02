#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
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

void ram_dump (struct hw_device *dev)
{
	FILE *in_file;
	int i;
	char *buf = dev->priv;
	in_file  = fopen("ram.bin", "w"); 
    if(in_file != NULL)
	{
		printf("(dbg) -- RAM --\n");
		printf("(dbg) ram.bin file exported\n");
		for(i=0;i<dev->size;i++)
		{
			fputc(buf[i],in_file);
		}
		fclose(in_file);
	}
}

struct hw_class ram_class =
{
	.name = "RAM",
	.readonly = 0,
	.reset = ram_reset,
	.read = ram_read,
	.write = ram_write,
	.dump = ram_dump,
	.check_interrupt = NULL,
};

struct hw_device *ram_create (unsigned long size)
{
	void *buf = malloc (size);
	return device_create (&ram_class, size, buf);
}
