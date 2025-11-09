#ifndef M6809_H
#define M6809_H

#include "types.h"

extern BOOLEAN (*m6809_insn_hook)(void);

#define E_FLAG 0x80
#define F_FLAG 0x40
#define H_FLAG 0x20
#define I_FLAG 0x10
#define N_FLAG 0x08
#define Z_FLAG 0x04
#define V_FLAG 0x02
#define C_FLAG 0x01

int m6809_execute (int);
void m6809_reset (void);
void m6809_print_regs (void);

unsigned m6809_get_a  (void);
unsigned m6809_get_b  (void);
unsigned m6809_get_cc (void);
unsigned m6809_get_dp (void);
unsigned m6809_get_x  (void);
unsigned m6809_get_y  (void);
unsigned m6809_get_s  (void);
unsigned m6809_get_u  (void);
unsigned m6809_get_pc (void);
unsigned m6809_get_d  (void);
unsigned m6809_get_flags (void);

void m6809_set_a  (unsigned);
void m6809_set_b  (unsigned);
void m6809_set_cc (unsigned);
void m6809_set_dp (unsigned);
void m6809_set_x  (unsigned);
void m6809_set_y  (unsigned);
void m6809_set_s  (unsigned);
void m6809_set_u  (unsigned);
void m6809_set_pc (unsigned);
void m6809_set_d  (unsigned);


//void command_irq_hook (unsigned long cycles);
unsigned long m6809_get_cycles (void);
int m6809_get_cpu_is_running (void);
void m6809_request_irq (unsigned int source);
void m6809_release_irq (unsigned int source);
void m6809_request_firq (unsigned int source);
void m6809_release_firq (unsigned int source);


#endif /* M6809_H */
