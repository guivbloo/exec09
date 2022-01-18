#include "types.h"
#include "debugger.h"
#include "monitor.h"
#include "command.h"

void debugger_init()
{
    monitor_init();
	command_init();
    return;
}

void debugger_exit()
{
    keybuffering (1);
}

void debugger_load_image(char *prog_name)
{
    int rc;
    if (prog_name)
	{
		rc = monitor_load_image (prog_name);
		if (rc != 0)
		{
			/* Error to be maanged */
            printf("error");
		}
		/* Try to load a map file */
		monitor_load_map_file (prog_name);
	}
    return;
}

void debugger_periodic()
{
    command_periodic();
}

void debugger_set_status(BOOLEAN status)
{
    monitor_set_debug(status);
}

BOOLEAN debugger_get_exitcmd(void)
{
    return(command_get_exitcmd());
}