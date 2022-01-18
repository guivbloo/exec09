#ifndef M6809_H
#define M6809_H

#include "types.h"
#include "bus_access.h"

typedef uint16_t target_addr_t;




//extern int debug_enabled;
//extern int need_flush;
// extern unsigned long total;
//extern int dump_cycles_on_success;
//extern const char *prog_name;
//long get_elapsed_realtime (void);

/* Primitive read/write macros */
//#define read8(addr)        bus_read8 (addr)
//#define write8(addr,val)   do { bus_write8 (addr, val); } while (0)

/* 16-bit versions */
//#define read16(addr)       bus_read16(addr)
//#define write16(addr,val)  do { write8(addr+1, val & 0xFF); write8(addr, (val >> 8) & 0xFF); } while (0)

/* Fetch macros */
//#define abs_read16(addr)   bus_read16_abs(addr)
#define fetch8()           bus_read8_abs (pc++)
#define fetch16()          (pc += 2, bus_read16_abs(pc-2))


int cpu_execute (int);
void cpu_reset (void);

unsigned get_a  (void);
unsigned get_b  (void);
unsigned get_cc (void);
unsigned get_dp (void);
unsigned get_x  (void);
unsigned get_y  (void);
unsigned get_s  (void);
unsigned get_u  (void);
unsigned get_pc (void);
unsigned get_d  (void);
unsigned get_flags (void);
void set_a  (unsigned);
void set_b  (unsigned);
void set_cc (unsigned);
void set_dp (unsigned);
void set_x  (unsigned);
void set_y  (unsigned);
void set_s  (unsigned);
void set_u  (unsigned);
void set_pc (unsigned);
void set_d  (unsigned);


void command_irq_hook (unsigned long cycles);
unsigned long get_cycles (void);
int get_cpu_is_running (void);
void request_irq (unsigned int source);
void release_irq (unsigned int source);
void request_firq (unsigned int source);
void release_firq (unsigned int source);
void print_regs (void);

#endif /* M6809_H */
