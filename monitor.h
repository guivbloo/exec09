#ifndef MONITOR_H
#define MONITOR_H

#include <stdio.h>
#include "types.h"

#define MAX_BREAKS 32
#define MAX_TRACE 256
#define IRQ_CYCLE_COUNTS 128

#define MAKE_ADDR(devno, phyaddr) ((devno * 0x10000000L) + phyaddr)


struct cpu_regs {
	unsigned X, Y, S, U, PC;
	unsigned A, B, DP;
	unsigned H, N, Z, OV, C;
	unsigned EFI;
};

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

typedef void (*virtual_handler_t) (unsigned long *val, int writep);

typedef enum
{
   LVALUE,
   RVALUE,
} eval_mode_t;

struct x_symbol {
	int flags;
	union {
		struct named_symbol {
			char *id;
			char *file;
			target_addr_t addr;
		} named;

		struct line_symbol {
			struct named_symbol *name;
			int lineno;
		} line;

		int offset;
	} u;
};

#define FC_TAIL_CALL 0x1

struct function_call {
	target_addr_t entry_point;
	struct cpu_regs entry_regs;
	int flags;
};

//void add_named_symbol (const char *id, target_addr_t value, const char *filename);
struct x_symbol * find_symbol (target_addr_t value);
//void monitor_branch (void);

/* --- Functions for monitor manipulation --- */
void monitor_init (void); 
int monitor_run (); 
int check_break (void);
void brkfree_temps (void);
void brkfree (breakpoint_t *br);
int monitor6809 (void);
int dasm (char *, absolute_address_t);


/* --- Functions for debug manipulation --- */
void monitor_set_debug(BOOLEAN status);
BOOLEAN monitor_get_debug_status (void);
breakpoint_t* brkalloc (void);
breakpoint_t* brkfind_by_id (unsigned int id);
breakpoint_t* brkfind_by_addr (absolute_address_t addr);
void command_trace_insn (target_addr_t addr);
const char * monitor_addr_name (target_addr_t addr);
//const char * absolute_addr_name (absolute_address_t addr);
void monitor_backtrace (void);
const char* monitor_addr_name (target_addr_t target_addr);
unsigned long target_read (absolute_address_t addr, unsigned int size);
unsigned long eval(char *expr, char *eflag);
unsigned long eval_mem (char *expr, eval_mode_t mode, char *eflag);
int monitor_load_map_file (const char *name);
int monitor_load_image (const char *name);


#endif
