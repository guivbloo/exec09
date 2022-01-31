#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "machine.h"
#include "device.h"
#include "m6809.h"



/* The pointer 'machine' points to the machine that is being run. */
machine_t *machine;

unsigned int device_count = 0;
struct hw_device *device_table[MAX_BUS_DEVICES];

struct hw_device *null_device;

struct bus_map busmaps[NUM_BUS_MAPS];

struct bus_map default_busmaps[NUM_BUS_MAPS];



void do_fault (unsigned int addr, unsigned int type)
{
	if (m6809_get_cpu_is_running())
		machine->fault (addr, type);
}

/**
 * Attach a new device to the bus.  Only called during init.
 */
void machine_attach_device (struct hw_device *dev)
{
	dev->devid = device_count;
	device_table[device_count++] = dev;

	/* Attach implies reset */
	dev->class_ptr->reset (dev);
}

/**
 * Map a portion of a device into the CPU's address space.
 */
void bus_map (unsigned int addr,
	unsigned int devid,
	unsigned long offset,
	unsigned int len,
	unsigned int flags)
{
	struct bus_map *map;
	unsigned int start, count;

	/* Warn if trying to map too much */
	if (addr + len > MAX_CPU_ADDR)
	{
		fprintf(stderr, "warning: mapping %04X bytes into %04X causes overflow\n",
				len, addr);
	}

	/* Round address and length to be multiples of the map unit size. */
	addr = ((addr + BUS_MAP_SIZE - 1) / BUS_MAP_SIZE) * BUS_MAP_SIZE;
	len = ((len + BUS_MAP_SIZE - 1) / BUS_MAP_SIZE) * BUS_MAP_SIZE;
	offset = ((offset + BUS_MAP_SIZE - 1) / BUS_MAP_SIZE) * BUS_MAP_SIZE;

	/* Convert from byte addresses to unit counts */
	start = addr / BUS_MAP_SIZE;
	count = len / BUS_MAP_SIZE;

	/* Initialize the maps.  This will let the CPU access the device. */
	map = &busmaps[start];
	while (count > 0)
	{
		if (!(map->flags & MAP_FIXED))
		{
			map->devid = devid;
			map->offset = offset;
			map->flags = flags;
		}
		count--;
		map++;
		offset += BUS_MAP_SIZE;
	}
}

void machine_map_device (struct hw_device *dev,
	unsigned long offset,
	unsigned int addr,
	unsigned int len,
	unsigned int flags)
{
	/* Note: len must be a multiple of BUS_MAP_SIZE */
	bus_map(addr, dev->devid, offset, len, flags);
}

void bus_unmap (unsigned int addr, unsigned int len)
{
	unsigned int start, count;

	/* Round address and length to be multiples of the map unit size. */
	addr = ((addr + BUS_MAP_SIZE - 1) / BUS_MAP_SIZE) * BUS_MAP_SIZE;
	len = ((len + BUS_MAP_SIZE - 1) / BUS_MAP_SIZE) * BUS_MAP_SIZE;

	/* Convert from byte addresses to unit counts */
	start = addr / BUS_MAP_SIZE;
	count = len / BUS_MAP_SIZE;

	/* Set the maps to their defaults. */
	memcpy (&busmaps[start], &default_busmaps[start],
		sizeof (struct bus_map) * count);
}

/**
 * Generate a page fault.  ADDR says which address was accessed
 * incorrectly.  TYPE says what kind of violation occurred.
 */

struct bus_map *machine_find_map (unsigned int addr)
{
	return &busmaps[addr / BUS_MAP_SIZE];
}

struct hw_device *machine_find_device (unsigned int addr, unsigned char id)
{
	/* Fault if any invalid device is accessed */
	if ((id == INVALID_DEVID) || (id >= device_count))
	{
		do_fault (addr, FAULT_NO_RESPONSE);
		return null_device;
	}
	return device_table[id];
}


// Dump machine (if supported)
void machine_dump(void)
{
	/*
    if (machine->dump) {
        machine->dump();
    }
    else {
        printf("This machine does not provide a dump capability\n");
    }
	*/
	int i;
	for (i=0; i < device_count; i++)
	{
		struct hw_device *dev = device_table[i];
		if (dev->class_ptr->dump)
			dev->class_ptr->dump (dev);
	}
}

void machine_reset (void)
{
	int i;
	m6809_reset();
	for (i=0; i < device_count; i++)
	{
		struct hw_device *dev = device_table[i];
		if (dev->class_ptr->reset)
			dev->class_ptr->reset (dev);
	}
}


// Describe machine, devices and mapping.
void machine_describe (void)
{
	unsigned int devno;
	unsigned int mapno;
	unsigned int prev_devid = -1;
	unsigned int prev_offset = 0;
	unsigned int prev_flags = 0;
	unsigned int dot_dot = 0;

	/* machine */
	printf("Machine: %s\n", machine->name);

	/* devices */
	for (devno = 0; devno < device_count; devno++)
	{
		printf("Device %2d: %s\n",devno, device_table[devno]->class_ptr->name);
	}

	/* mapping */
	for (mapno = 0; mapno < NUM_BUS_MAPS; mapno++)
	{
		struct bus_map *map = &busmaps[mapno];
		if ( (map->devid == prev_devid) && (map->flags == prev_flags) &&
                    ((map->offset == prev_offset) || (map->devid == INVALID_DEVID)) )
		{
			/* nothing interesting to report */
			if (! dot_dot)
			{
				printf("..\n");
				dot_dot = 1;
			}
		}
		else
		{
			dot_dot = 0;
			if(map->devid != INVALID_DEVID)
			{
				printf ("Map %3d:  addr=%04X  dev=%d  offset=%04lX  size=%04X  flags=%02X\n",
					mapno, mapno * BUS_MAP_SIZE, map->devid, map->offset,
					device_table[map->devid]->size, map->flags);
			}
		}
		/* ready for next time */
		prev_devid = map->devid;
		prev_offset = map->offset + BUS_MAP_SIZE;
		prev_flags = map->flags;
	}
}

/**********************************************************
 * Simple fault handler
 **********************************************************/

void machine_fault (unsigned int addr, unsigned char type)
{
	if (m6809_get_cpu_is_running())
	{
		//sim_error (">>> Page fault: addr=%04X type=%02X PC=%04X\n", addr, type, m6809_get_pc ());
		//error
		printf("error");
	}
}

int machine_run (int cycles)
{
	return m6809_execute(cycles);
}


/**********************************************************/

void machine_update (void)
{
	int i;
	for (i=0; i < device_count; i++)
	{
		struct hw_device *dev = device_table[i];
		if (dev->class_ptr->update)
			dev->class_ptr->update (dev);
	}
}

int machine_dump_thread(void)
{
	return(0);
}

void machine_periodic (void)
{
	return;
}

unsigned long machine_get_cycles (void)
{
	return m6809_get_cycles();
}

void machine_tick (unsigned long nb_cycles)
{
	int i;
	for (i=0; i < device_count; i++)
	{
		struct hw_device *dev = device_table[i];
		if (dev->class_ptr->tick)
			dev->class_ptr->tick (dev, nb_cycles);
	}
}

int machine_match (const char *machine_name, machine_t *m)
{
	if (!strcmp (m->name, machine_name))
	{
		return 1;
	}
	return 0;
}

void machine_init (const char *machine_name, const char *boot_rom_file)
{
	//extern struct machine simple_machine;
	extern machine_t bloo_machine;
	//extern struct machine eon_machine;
	//extern struct machine eon2_machine;
	//extern struct machine smii_machine;
	//extern struct machine multicomp09_machine;
	//extern struct machine kipper1_machine;
	int i;

	/* Initialize CPU maps, so that no CPU addresses map to
	anything.  Default maps will trigger faults at runtime. */
	memset (busmaps, 0, sizeof (busmaps));
	for (i = 0; i < NUM_BUS_MAPS; i++)
		busmaps[i].devid = INVALID_DEVID;

	if (machine_match (machine_name, &bloo_machine))
	{
		machine = &bloo_machine;
		machine->init (boot_rom_file);
	}	
	//else if machine_match (machine_name, boot_rom_file, &simple_machine));
	//else if (machine_match (machine_name, boot_rom_file, &eon2_machine));
	//else if (machine_match (machine_name, boot_rom_file, &smii_machine));
	//else if (machine_match (machine_name, boot_rom_file, &kipper1_machine));
	else exit (1);

	/* Save the default busmap configuration, before the
	CPU begins to run, so that it can be restored if
	necessary. */
	memcpy (default_busmaps, busmaps, sizeof (busmaps));

	/* This should not be here */
	//if (!strcmp (machine_name, "eon"))
	//	mmu_reset_complete (mmu_device);
		
}

