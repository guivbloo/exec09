#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils_time.h"
#include "types.h"
#include "simulator.h"
#include "gui_monitor.h"
#include "cli_monitor.h"


int do_help (const char *arg __attribute__((unused)));
void set_use_gui(BOOLEAN val);
char *program_name = NULL;
int use_gui = FALSE;


struct option
{
	char o_short;
	const char *o_long;
	const char *help;
	unsigned int can_negate : TRUE;
	unsigned int takes_arg : TRUE;
	unsigned int is_int: TRUE;
	int (*handler) (const char *arg);
	void (*setter_d) (BOOLEAN arg);
	void (*setter_s) (char *arg);
} 

option_table[] = {
	{ 'h', "help", "Show this help message",
		FALSE, FALSE, FALSE, do_help, NULL, NULL},
	{ 's', "machine", "Specify the machine (exact hardware) to emulate",
		FALSE, TRUE, FALSE, NULL, NULL, sim_set_machine_name},
	{ 'g', "gui", "Use the GUI monitor",
		TRUE, FALSE, TRUE, NULL, set_use_gui, NULL},
	{ '\0', NULL },
};

void set_use_gui(BOOLEAN val)
{
	use_gui = val;
}

int do_help (const char *arg __attribute__((unused)))
{
	struct option *opt = option_table;

	printf ("EXEC09: Motorola 6809 Simulator\n");
	printf ("m6809-run [options] [program]\n\n");
	printf ("Options:\n");
	while (opt->o_long != NULL)
	{
		if (opt->help)
		{
			if (opt->o_short == '-')
				printf ("   --%-16.16s    %s\n", opt->o_long, opt->help);
			else
				printf ("   -%c, --%-16.16s%s\n", opt->o_short, opt->o_long, opt->help);
		}
		opt++;
	}
	exit (0);	
}

/**
 * Returns positive if an argument was taken.
 * Returns zero if no argument was taken.
 * Returns negative on error.
 */
int process_option (struct option *opt, char *arg)
{
	int rc;
	int var;
	/* printf ("Processing option '%s'\n", opt->o_long); */
	if (opt->takes_arg)
	{
		if (!arg)
		{
			printf ("  Takes argument but none given.\n");
			rc = 0;
		}
		else
		{
			if (opt->is_int)
			{
				var = strtoul (arg, NULL, 0);
				opt->setter_d(var);
			}
			else
			{
				/* *(opt->string_value) = arg; */
				opt->setter_s(arg);
			}
			rc = 1;
		}
	}
	else
	{
		if (opt->is_int)
		{
			opt->setter_d(TRUE);
		}
		rc = 0;
	}

	if (opt->handler)
	{
		rc = opt->handler (arg);
		//printf ("  Handler called, rc=%d\n", rc);
	}

	if (rc < 0)
		sim_exit (0x70);
	return rc;
}

/* Set program name to execute */
void process_plain_argument (char *arg)
{
	//printf ("plain argument '%s'\n", arg);
	program_name = arg;
}


void parse_args (int argc, char *argv[])
{
	int argn = 1;
	struct option *opt;

next_arg:
	while (argn < argc)
	{
		char *arg = argv[argn];
		if (arg[0] == '-')
		{
			if (arg[1] == '-')
			{
				char *rest = strchr (arg+2, '=');
				if (rest)
					*rest++ = '\0';

				opt = option_table;
				while (opt->o_long != NULL)
				{
					if (!strcmp (opt->o_long, arg+2))
					{
						argn++;
						(void)process_option (opt, rest);
						goto next_arg;
					}
					opt++;
				}
				printf ("long option '%s' not recognized.\n", arg+2);
			}
			else
			{
				opt = option_table;
				while (opt->o_long != NULL)
				{
					if (opt->o_short == arg[1])
					{
						argn++;
						if (process_option (opt, argv[argn]))
							argn++;
						goto next_arg;
					}
					opt++;
				}
				printf ("short option '%c' not recognized.\n", arg[1]);
			}
			argn++;
		}
		else
		{
			process_plain_argument (argv[argn++]);
		}
	}
}



int main (int argc, char *argv[])
{
	int rc = 0;
	parse_args (argc, argv);
	if(use_gui == FALSE)
	{
		cli_monitor_init();
	}
	else
	{
		gui_monitor_init();
	}
	rc = sim_init(program_name);
	if (rc != 0)
	{
		sim_exit (rc);
	}

	if(use_gui == FALSE)
	{
		do
		{
			rc = cli_monitor_run();
			if(rc == 0)
				sim_run();
		}
		while(rc == 0);
	}
	else
	{
		gui_monitor_run();
	}


	sim_exit (0);
	return (0);
}
