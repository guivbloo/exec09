#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "types.h"

void sim_set_machine_name(char *machine);

int sim_init(char *prog_name);
int sim_run();
void sim_error (const char *format, ...);
void sim_exit (uint8_t exit_code);

#endif /* SIMULATOR_H */
