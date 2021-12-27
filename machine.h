#ifndef M6809_MACHINE_H
#define M6809_MACHINE_H


#include "device.h"

/* This file defines structures used to build generic machines on a 6809. */



typedef unsigned long absolute_address_t;

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

struct bus_map
{
	unsigned int devid; /* The devid mapped here */
	unsigned long offset; /* The offset within the device */
	unsigned char flags;
};

#define NUM_BUS_MAPS (MAX_CPU_ADDR / BUS_MAP_SIZE)


/* The machine structure collects everything about the abstract machine.
The pointer 'machine' points to the machine that is being run. */

extern struct machine *machine;

struct machine
{
	const char *name;
	void (*init) (const char *boot_rom_file);
	void (*fault) (unsigned int addr, unsigned char type);
	void (*dump_thread) (unsigned int thread_id);
	void (*periodic) (void);
	void (*dump) (void);
	void (*tick) (void);
	unsigned long cycles_per_sec;
};

void machine_init (const char *machine_name, const char *boot_rom_file);

void fault (unsigned int addr, unsigned char type);
uint8_t cpu_read8 (unsigned int addr);
uint16_t cpu_read16 (unsigned int addr);
void cpu_write8 (unsigned int addr, uint8_t val);
uint8_t abs_read8 (absolute_address_t addr);
void abs_write8 (absolute_address_t addr, uint8_t val);
void cpu_is_running (void);

absolute_address_t to_absolute (unsigned long cpuaddr);
void dump_machine(void);
void describe_machine (void);
void machine_update (void);
void print_device_name (unsigned int devno);
void device_define (struct hw_device *dev,
        unsigned long offset,
        unsigned int addr,
        unsigned int len,
        unsigned int flags);
void bus_map (unsigned int addr,
        unsigned int devid,
        unsigned long offset,
        unsigned int len,
        unsigned int flags);
void bus_unmap (unsigned int addr, unsigned int len);
struct hw_device *device_attach (struct hw_class *class_ptr, unsigned int size, void *priv);

#endif /* _M6809_MACHINE_H */
