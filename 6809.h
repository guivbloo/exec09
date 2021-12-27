/*
 * Copyright 2001 by Arto Salmi and Joze Fabcic
 * Copyright 2006 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of GCC6809.
 *
 * GCC6809 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * GCC6809 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GCC6809; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef M6809_H
#define M6809_H

#include "types.h"
#include "machine.h"

typedef uint16_t target_addr_t;


#define E_FLAG 0x80
#define F_FLAG 0x40
#define H_FLAG 0x20
#define I_FLAG 0x10
#define N_FLAG 0x08
#define Z_FLAG 0x04
#define V_FLAG 0x02
#define C_FLAG 0x01

/* For cwai state machine */
#define CWAI_STATE_IDLE     0
#define CWAI_STATE_WAIT     1


extern int debug_enabled;
extern int need_flush;
// extern unsigned long total;
extern int dump_cycles_on_success;
extern const char *prog_name;

//long get_elapsed_realtime (void);

/* Primitive read/write macros */
#define read8(addr)        cpu_read8 (addr)
#define write8(addr,val)   do { cpu_write8 (addr, val); } while (0)

/* 16-bit versions */
#define read16(addr)       cpu_read16(addr)
#define write16(addr,val)  do { write8(addr+1, val & 0xFF); write8(addr, (val >> 8) & 0xFF); } while (0)

/* Fetch macros */

#define abs_read16(addr)   ((abs_read8(addr) << 8) | abs_read8(addr+1))

#define fetch8()           abs_read8 (pc++)
#define fetch16()          (pc += 2, abs_read16(pc-2))

/* 6809.c */
extern int cpu_quit;
extern int cpu_execute (int);
extern void cpu_reset (void);

extern unsigned get_a  (void);
extern unsigned get_b  (void);
extern unsigned get_cc (void);
extern unsigned get_dp (void);
extern unsigned get_x  (void);
extern unsigned get_y  (void);
extern unsigned get_s  (void);
extern unsigned get_u  (void);
extern unsigned get_pc (void);
extern unsigned get_d  (void);
extern unsigned get_flags (void);
extern void set_a  (unsigned);
extern void set_b  (unsigned);
extern void set_cc (unsigned);
extern void set_dp (unsigned);
extern void set_x  (unsigned);
extern void set_y  (unsigned);
extern void set_s  (unsigned);
extern void set_u  (unsigned);
extern void set_pc (unsigned);
extern void set_d  (unsigned);


/* monitor.c */
extern int monitor_on;
extern int check_break (void);
extern void monitor_init (void); 
extern int monitor6809 (void);
extern int dasm (char *, absolute_address_t);





#define SYM_DEFAULT 0
#define SYM_AUTO 1





/* symtab.c */


typedef void (*command_handler_t) (void);

typedef void (*virtual_handler_t) (unsigned long *val, int writep);

typedef unsigned int thread_id_t;

typedef struct
{
   unsigned int id : 8;
   unsigned int used : 1;
   unsigned int enabled : 1;
   unsigned int conditional : 1;
   unsigned int threaded : 1;
   unsigned int on_read : 1;
   unsigned int on_write : 1;
   unsigned int on_execute : 1;
   unsigned int size : 4;
   unsigned int keep_running : 1;
	unsigned int temp : 1;
	unsigned int last_write : 16;
	unsigned int write_mask : 16;
   absolute_address_t addr;
   char condition[128];
   thread_id_t tid;
   unsigned int pass_count;
   unsigned int ignore_count;
} breakpoint_t;






typedef struct
{
   int id : 8;
   thread_id_t tid;
} thread_t;


#define MAX_BREAKS 32
#define MAX_DISPLAYS 32
#define MAX_HISTORY 10
#define MAX_THREADS 64

void command_irq_hook (unsigned long cycles);
unsigned long get_cycles (void);

void request_irq (unsigned int source);
void release_irq (unsigned int source);
void request_firq (unsigned int source);
void release_firq (unsigned int source);
void print_regs (void);

#endif /* M6809_H */
