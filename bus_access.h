#ifndef BUS_ACCESS_H
#define BUS_ACCESS_H

#include "types.h"

typedef unsigned long absolute_address_t;

absolute_address_t to_absolute (unsigned long cpuaddr);
absolute_address_t absolute_from_reladdr (unsigned int device, unsigned long reladdr);

// Read and write hooks for monitoring bus accesses
extern void (*bus_read_hook)(absolute_address_t addr);
extern void (*bus_write_hook)(absolute_address_t addr, uint8_t val);


uint8_t bus_read8 (unsigned int addr);
uint16_t bus_read16 (unsigned int addr);
uint8_t bus_read8_abs (absolute_address_t addr);
uint16_t bus_read16_abs (absolute_address_t addr);
void bus_write8 (unsigned int addr, uint8_t val);
void bus_write8_abs (absolute_address_t addr, uint8_t val);
void bus_write16 (unsigned int addr, uint16_t val);
#endif /* BUS_ACCESS_H */



