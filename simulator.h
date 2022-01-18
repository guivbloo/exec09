#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "types.h"

void sim_set_debug(BOOLEAN bool);
void sim_set_binary(BOOLEAN bool);
void sim_set_machine_name(char *machine);
void sim_set_prog_name(char *name);
BOOLEAN sim_get_debug_status();

int sim_get_os9call(void);

int sim_init();
int sim_run();

void sim_error (const char *format, ...);
void sim_exit (uint8_t exit_code);

#endif /* SIMULATOR_H */
