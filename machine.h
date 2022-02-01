#ifndef MACHINE_H
#define MACHINE_H

#include "types.h"
#include "device.h"


struct bus_map
{
	unsigned int devid; /* The devid mapped here */
	unsigned long offset; /* The offset within the device */
	unsigned char flags;
};



#define MAX_CPU_ADDR 65536

/* The generic bus architecture. */

/* Up to 32 devices may be connected.  Each device is addressed by a 32-bit physical address */
#define MAX_BUS_DEVICES 32

#define INVALID_DEVID 0xff

/* Say whether or not the mapping is RO or RW (or neither). */
#define MAP_READABLE 0x1
#define MAP_WRITABLE 0x2
#define MAP_READWRITE 0x3

/* Usually, an attempt to write without MAP_WRITABLE will cause a fault.
   This allows a write and the data silently ignored (no fault) */
#define MAP_IGNOREWRITE 0x8

/* A fixed map cannot be reprogrammed.  Attempts to
bus_map something differently will silently be
ignored. */
#define MAP_FIXED 0x4

#define FAULT_NONE 0
#define FAULT_NOT_WRITABLE 1
#define FAULT_NO_RESPONSE 2
#define FAULT_NOT_READABLE 3

/* A bus map is assocated with part of the 6809 address space
and says what device and which part of it is mapped into that
area.  It also has associated flags which say how it is allowed
to be accessed.

A single bus map defines 128 bytes of address space; for a 64KB CPU,
that requires a total of 512 such structures.

Note that the bus map need not correspond to the page size that can
be configured by the MMU.  It allows for more granularity and is
needed in some *hardcoded* mapping cases. */

#define BUS_MAP_SIZE 128
#define NUM_BUS_MAPS (MAX_CPU_ADDR / BUS_MAP_SIZE)

typedef struct 
{
	const char *name;
	void (*init) (const char *boot_rom_file);
	void (*fault) (unsigned int addr, unsigned char type);
	void (*dump_thread) (unsigned int thread_id);
	void (*periodic) (void);
	void (*dump) (void);
	void (*tick) (void);
	void (*update) (void);
	uint8_t (*irq) (void);
	uint8_t (*firq) (void);
	unsigned long cycles_per_sec;
} machine_t;

void machine_init (const char *machine_name, const char *boot_rom_file);
void machine_fault (unsigned int addr, unsigned char type);
void machine_dump(void);
int machine_dump_thread(void);
void machine_describe (void);
void machine_update (void);
void machine_periodic (void);
void machine_tick (unsigned long nb_cycles);
int machine_run (int cycles);
void machine_reset (void);
unsigned int machine_check_irq (void);
unsigned int machine_check_firq (void);
void machine_attach_irq (struct hw_device *dev);
void machine_attach_firq (struct hw_device *dev);

unsigned long machine_get_cycles (void);
struct bus_map *machine_find_map (unsigned int addr);
struct hw_device *machine_find_device (unsigned int addr, unsigned char id);





/* Functions used by a machine to attach and map devices in the addressable space */
void machine_map_device (struct hw_device *dev,
        unsigned long offset,
        unsigned int addr,
        unsigned int len,
        unsigned int flags);
void machine_attach_device (struct hw_device *dev);

#endif /* MACHINE_H */
