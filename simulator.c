#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "types.h"
#include "simulator.h"
#include "utils_time.h"
#include "symtab.h"
#include "command.h"
#include "6809.h"
#include "monitor.h"
#include "machine.h"

#define INT_MAX 0

/* Nonzero if SWI2 should be reported with a postbyte */
int os9call = 0;

/* The frequency of the emulated CPU, in megahertz */
unsigned int sim_freq = 1;

/* When nonzero, indicates that the machine's tick routine should be
   triggered periodically, every so many cycles. Typically this is
   used by the machine to generate a timer interrupt. Off By default.
*/
unsigned int cycles_per_tick = 0;

/* Nonzero if debugging support is turned on */
int debug_enabled = 1;

/* Nonzero if tracing is enabled */
int trace_enabled = 0;

/* When nonzero, causes the program to print the total number of cycles
on a successful exit. */
int dump_cycles_on_success = 0;

/* When nonzero, indicates the total number of cycles before an automated
exit.  This is to help speed through test cases that never finish. */
int max_cycles = INT_MAX;

/* When nonzero, says that the state of the machine is persistent
across runs of the simulator. */
int machine_persistent = 0;

/* The file to be loaded is a .bin file */
static int binary = 0;

const char *machine_name = "bloo";

const char *prog_name = NULL;

void sim_set_debug(int bool)
{
    if(bool == 0)
        debug_enabled = 0;
    else
        debug_enabled = 1;
} 


void sim_set_binary(int bool)
{
    if(bool == 0)
        binary = 0;
    else
        binary = 1;
} 

void sim_set_trace(int bool)
{
    if(bool == 0)
        trace_enabled = 0;
    else
        trace_enabled = 1;
} 

void sim_set_machine_name(const char *name)
{
    machine_name = name;
} 

void sim_set_prog_name(const char *name)
{
    prog_name = name;
}   

int sim_get_os9call(void)
{
    return (os9call);
}  

/*
 * Check if the CPU should idle.
 */
void idle_loop (void)
{
	struct timeval now;
	static struct timeval last = { 0, 0 };
	int real_ms;
	static unsigned long last_cycles = 0;
	unsigned long cycles;
	int sim_ms;
	const int cycles_per_ms = 2000;
	static int period = 30;
	static int count = 30;
	int delay;
	static int total_ms_elapsed = 0;
	static int cumulative_delay = 0;

	if (--count > 0)
		return;

	if (last.tv_sec == 0 && last.tv_usec == 0)
		gettimeofday (&last, NULL);

	gettimeofday (&now, NULL);
	real_ms = time_diff (&last, &now);
	last = now;

	cycles = get_cycles ();
	sim_ms = (cycles - last_cycles) / cycles_per_ms;
	if (sim_ms < 0)
		sim_ms += cycles_per_ms;
	last_cycles = cycles;

	total_ms_elapsed += sim_ms;
	if (total_ms_elapsed > 100)
	{
		total_ms_elapsed -= 100;
		if (machine->periodic) machine->periodic ();
		command_periodic ();
	}

	delay = sim_ms - real_ms;
	cumulative_delay += delay;
	if (cumulative_delay > 0)
	{
		usleep (50 * 1000UL);
		cumulative_delay -= 50;
	}

	count = period;
}

int sim_init()
{
    init_time();
    /* Init symbol table */
    sym_init();
    if (binary)
	{
		/* Binary option: Load directly the .bin during machine_init*/
		machine_init (machine_name, prog_name);
	}
	else
	{
		/* The machine loader cannot deal with image files, so initialize the machine first, passing it a NULL
		filename, then load the image file in S19 or hex format afterwards. */
		machine_init (machine_name, NULL);
		if (prog_name && load_image (prog_name))
            printf("error");
	}
    /* Try to load a map file */
	if (prog_name)
		load_map_file (prog_name);

	/* Enable debugging if no executable given yet or debug_enabled option is activated */
	if (!prog_name || debug_enabled == 1)
		debug_activate();
	
	/* OK, ready to run.  Reset the CPU first. */
	if (prog_name)
		cpu_reset ();

	monitor_init ();
	keybuffering_defaults();
	keybuffering(0);
}

int sim_run()
{
/* Now, iterate through the instructions.
           Without -I, we can just call cpu_execute() and let it run
           for a long time; otherwise, we need to come back here
           periodically and call the machine's ->tick() routine */
        //[NAC HACK 2017Mar30] need to schedule this properly instead of this one-or-the-other approach
        //.. need to track the rate of each and work out who's next.
	for (cpu_quit = 1; cpu_quit != 0;)
	{
		/* Call each device that needs periodic processing. */
		machine_update ();

		if (cycles_per_tick == 0)
		{
			/* Simulate some CPU time, either 1ms worth or up to the
			next possible tick */
			cpu_execute (sim_freq * 1000);
		}
		else
		{
			cpu_execute (cycles_per_tick);
			if (machine->tick) machine->tick ();
		}

		/* Align with real time*/
		idle_loop ();

		/* Check for a rogue program that won't end */
		if ((max_cycles > 0) && (get_cycles() > max_cycles))
		{
			sim_error ("maximum cycle count exceeded at %s\n",
				monitor_addr_name (get_pc ()));
		}
	}

	sim_exit (0);
	keybuffering (1);
	return 0;
}

void sim_error (const char *format, ...)
{
	va_list ap;

	va_start (ap, format);
	//fprintf (stderr, "m6809-run: (at PC=%04X) ", iPC);
	vfprintf (stderr, format, ap);
	va_end (ap);

	if (debug_enabled)
		monitor_activate();
	else {
		keybuffering (1);
		exit (2);
        }
}

void sim_exit (uint8_t exit_code)
{
	char *s;

	/* On a nonzero exit, always print an error message. */
	if (exit_code != 0)
	{
		printf ("m6809-run: program exited with %d\n", exit_code);
		if (exit_code)
			monitor_backtrace ();
	}

	/* If a cycle count should be printed, do that last. */
	if (dump_cycles_on_success)
	{
		printf ("%s : %ld cycles, %ld ms\n", prog_name, get_cycles (),
			get_elapsed_realtime ());
	}

	if ((s = getenv ("LOG6809")) != NULL)
	{
		FILE *fp = fopen (s, "a");
		if (fp)
		{
			fprintf (fp, "%s : %ld cycles, %ld ms\n", prog_name, get_cycles (),
				get_elapsed_realtime ());
			fclose (fp);
		}
	}
	keybuffering (1);
	exit (exit_code);
}