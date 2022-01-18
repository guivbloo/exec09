#ifndef COMMAND_H
#define COMMAND_H

#include "types.h"
#include "bus_access.h"



void keybuffering (int flag);
void command_periodic (void);
void command_exit_irq_hook (unsigned long cycles);
void command_insn_hook (void);
void command_init (void);
BOOLEAN command_get_exitcmd(void);
void print_current_insn (void);
int command_loop (void);
void command_read_hook (absolute_address_t addr);
void command_write_hook (absolute_address_t addr, uint8_t val);




#endif
