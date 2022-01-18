#include <stdio.h>
#include "bus_access.h"
#include "types.h"
#include "device.h"
#include "machine.h"
#include "command.h"
#include "logging.h"


/**
 * Called by the CPU to read a byte.
 * This is the bottleneck in terms of performance.  Consider
 * a caching scheme that cuts down on some of this.
 * There is also a 16-bit version that is more efficient when
 * a full word is needed, but it implies that no reads will ever
 * occur across a device boundary.
 */
uint8_t bus_read8 (unsigned int addr)
{
	struct bus_map *map = machine_find_map (addr);
	struct hw_device *dev = machine_find_device (addr, map->devid);
	struct hw_class *class_ptr = dev->class_ptr;
	unsigned long phy_addr = map->offset + addr % BUS_MAP_SIZE;

	if (!(map->flags & MAP_READABLE))
	{
		//machine->fault (addr, FAULT_NOT_READABLE);
        log_message(ERROR,"Attempt to read a not readable address");
	}
	command_read_hook (absolute_from_reladdr (map->devid, phy_addr));
	return (*class_ptr->read) (dev, phy_addr);
}

uint16_t bus_read16 (unsigned int addr)
{
	struct bus_map *map = machine_find_map (addr);
	struct hw_device *dev = machine_find_device (addr, map->devid);
	struct hw_class *class_ptr = dev->class_ptr;
	unsigned long phy_addr = map->offset + addr % BUS_MAP_SIZE;

	if (!(map->flags & MAP_READABLE))
		//do_fault (addr, FAULT_NOT_READABLE);
        log_message(ERROR, "Attempt to read a not readable address");
	command_read_hook (absolute_from_reladdr (map->devid, phy_addr));
	return ((*class_ptr->read) (dev, phy_addr) << 8)
			| (*class_ptr->read) (dev, phy_addr+1);
}

/**
 * Called by the CPU to write a byte.
 */
void bus_write8 (unsigned int addr, uint8_t val)
{
        //printf("write 0x%04x<-0x%02x\n", addr, val);
        //fprintf(log_file,"wr 0x%04x<-0x%02x\n", addr, val);
	struct bus_map *map = machine_find_map (addr);
	struct hw_device *dev = machine_find_device (addr, map->devid);
	struct hw_class *class_ptr = dev->class_ptr;
	unsigned long phy_addr = map->offset + addr % BUS_MAP_SIZE;

        /* Unlike the read case, where we still return data on
           an access error, ignore write data on access error.
           The cpu_running check allows ROMs to be loaded at
           startup (but maybe it would be better if ROM load
           used absolute access so that this routine was not
           used at all for that purpose) */
	if (map->flags & MAP_WRITABLE)
    {
        (*class_ptr->write) (dev, phy_addr, val);
    }
    else if (map->flags & MAP_IGNOREWRITE)
    {
        /* silently ignore the write */
    }
    /* do this regardless (may trigger watchpoint) */
     command_write_hook (absolute_from_reladdr (map->devid, phy_addr), val);
}

void bus_write8_abs (absolute_address_t addr, uint8_t val)
{
	unsigned int id = addr >> 28;
	unsigned long phy_addr = addr & 0xFFFFFFF;
	//struct hw_device *dev = device_table[id];
    struct hw_device *dev = machine_find_device (0, id);
	struct hw_class *class_ptr = dev->class_ptr;
	class_ptr->write(dev, phy_addr, val);
}

uint8_t bus_read8_abs (absolute_address_t addr)
{
	// nac come here on dbg examine. Core dump on access to nxm.
	// nac what is "id" and how is it extracted from top 4 bits and
	// why does this need addr to be 64 bits?
	unsigned int id = addr >> 28;
	unsigned long phy_addr = addr & 0xFFFFFFF;
	// printf("In abs_read8 with address 0x%x, id 0x%x and phy_addr 0x%x\n",addr,id,phy_addr);
        // nac BUG! should not be doing this directly:
        // an attempt to access a non-existent location (a location whose device ID is FF)
        // results in an attempt to access a non-existent value in the device_table.
        // Actually it's doubly bad: there are 32 devices (max) but access to non-existent
        // device seems to yield an ID of 0xf rather than 0xff which, based on the 28-bit
        // shift, implies that the address was bad in the first place: it only had f instead of ff
        // In any case, the table should not be indexed with f or ff becasue neither are valid
        // devices.
        // BUT! my "fix" below is bad; the fault gets reported 2ce and
        // the data value gets reported as a 32-bit value instead of a uint8_t
        // eg, if null_read is set up to return 0xab it returns 0xffffffab
        // and if it's set up to return 0x3b it returns 0x3b -- ie, it is
        // being sign extended. Not sure why, though, becasue it looks identical
        // to the normal read; must be due to a path taken in the error handling?
        //
        // 2 scenarios: access to 0:0 ie direct access device 0 even though
        // that device is not mapped into the bus anywhere. Currently does not
        // report any error but does return 0xffffffff (sign extended). Not reporting an error
        // is fine (I suppose) because the access is not really being checked
        // -- it doesn't correspond to a CPU address.
        // Other scenario is access to 0x7c80 in smii. This is a real CPU
        // address but is mapped to a non-existent device. Currently get 2
        // errors reported: the first is due to an access through cpu_read8
        // and the error is a page fault, address 0x7c80 -- the error is
        // triggered by a check of the map entry. The second is due
        // to an access through abs_read8 and the error is a page fault,
        // address 0xf000.0000 -- thinks it's device 0xff but truncated.
        // For this one the data comes back as 0xffffffff (sign-extended).
        //
        // Maybe need to switch to using CPU addresses everywhere user-facing
        // and allow device:offset addressing only on the command line?
        //
        // Another option that might help a fix is to switch to initialising
        // the map with device 0 rather than with non-such-device. Then,
        // no-such-device can be a more fatal error...

        // orig: -- core dumps!!
        //struct hw_device *dev = device_table[id];
        // replacement: -- still not right.
	struct hw_device *dev = machine_find_device (addr, id);

	struct hw_class *class_ptr = dev->class_ptr;
	return (*class_ptr->read) (dev, phy_addr);
}

uint16_t bus_read16_abs(absolute_address_t addr)
{
    return (bus_read8_abs(addr) << 8) | bus_read8_abs(addr+1);
}

void bus_write16 (unsigned int addr, uint16_t val)
{
    bus_write8(addr+1, val & 0xFF); 
    bus_write8(addr, (val >> 8) & 0xFF);
}

absolute_address_t absolute_from_reladdr (unsigned int device, unsigned long reladdr)
{
   return (device * 0x10000000L) + reladdr;
}


absolute_address_t to_absolute (unsigned long cpuaddr)
{
	/* if it's greater than 0xffff, it's already absolute
           and we cannot convert it again. If it's less than
           0x10000 it might already be absolute but it's safe
           to convert it a second time.
        */
	if (cpuaddr > 0xffff) return (absolute_address_t)cpuaddr;

	struct bus_map *map = machine_find_map (cpuaddr);
	unsigned long phy_addr = map->offset + cpuaddr % BUS_MAP_SIZE;
	return absolute_from_reladdr (map->devid, phy_addr);
}