#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "types.h"

void debugger_load_image(char *prog_name);
void debugger_init();
void debugger_get_status();
void debugger_set_status(BOOLEAN status);
void debugger_periodic();
void debugger_exit();
BOOLEAN debugger_get_exitcmd(void);

#endif