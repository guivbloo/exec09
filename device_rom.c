 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include "types.h"
 #include "device.h"
 #include "device_rom.h"
 #include "io_file.h"

uint8_t rom_read (struct hw_device *dev, unsigned long addr)
{
	char *buf = dev->priv;
	return buf[addr];
}

void rom_write (struct hw_device *dev, unsigned long addr, uint8_t val)
{
	char *buf = dev->priv;
	buf[addr] = val;
}

void rom_dump (struct hw_device *dev)
{
	FILE *in_file;
	int i;
	char *buf = dev->priv;
	in_file  = fopen("rom.bin", "w"); 
    if(in_file != NULL)
	{
		printf("(dbg) -- ROM --\n");
		printf("(dbg) rom.bin file exported\n");
		for(i=0;i<dev->size;i++)
		{
			fputc(buf[i],in_file);
		}
		fclose(in_file);
	}
}

void rom_reset (struct hw_device *dev)
{
	(void) dev;	// silence warning unused parameter
}

struct hw_class rom_class =
{
	.name = "ROM",
	.readonly = 1,
	.reset = rom_reset,
	.read = rom_read,
	.write = rom_write,
	.dump = rom_dump,
	.check_interrupt = NULL,
};

struct hw_device *rom_create (const char *filename, unsigned int maxsize)
{
	FILE *fp;
	struct hw_device *dev;
	unsigned int image_size;
	char *buf;

	if (filename)
	{
		fp = file_open (NULL, filename, "rb");
		if (!fp)
			return NULL;
		image_size = sizeof_file (fp);
	}

	buf = malloc (maxsize);
	dev = device_create (&rom_class, maxsize, buf);
	if (filename)
	{
		fread (buf, image_size, 1, fp);
		fclose (fp);
		maxsize -= image_size;
		while (maxsize > 0)
		{
			memcpy (buf + image_size, buf, image_size);
			buf += image_size;
			maxsize -= image_size;
		}
	}

	return dev;
}
