#ifndef MONITOR_H
#define MONITOR_H

#include <stdio.h>
#include "types.h"

struct cpu_regs {
	unsigned X, Y, S, U, PC;
	unsigned A, B, DP;
	unsigned H, N, Z, OV, C;
	unsigned EFI;
};


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
void monitor_call (unsigned int flags);
void monitor_return (void);
/* --- Functions for monitor manipulation --- */
void monitor_init (void); 
int check_break (void);
int monitor6809 (void);
int dasm (char *, absolute_address_t);

/* --- Functions for debug manipulation --- */
void monitor_set_debug(BOOLEAN status);
BOOLEAN monitor_get_debug_status (void);

const char * monitor_addr_name (target_addr_t addr);
//const char * absolute_addr_name (absolute_address_t addr);
void monitor_backtrace (void);
const char* monitor_addr_name (target_addr_t target_addr);

int monitor_load_map_file (const char *name);
int monitor_load_image (const char *name);


#endif
