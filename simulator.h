#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "types.h"

void sim_set_debug(int bool);
void sim_set_binary(int bool);
void sim_set_trace(int bool);
void sim_set_machine_name(const char *machine);
void sim_set_prog_name(const char *name);

int sim_get_os9call(void);

int sim_init();
int sim_run();

void sim_error (const char *format, ...);
void sim_exit (uint8_t exit_code);

#endif /* SIMULATOR_H */
