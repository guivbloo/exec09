#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "types.h"
#include "simulator.h"
#include "utils_time.h"
#include "machine.h"
#include "debugger.h"
#include "logging.h"
#include "symtab.h"

#define INT_MAX 0

/* Nonzero if SWI2 should be reported with a postbyte */
int os9call = 0;

/* The frequency of the emulated CPU, in megahertz */
unsigned int sim_freq = 1;

/* When nonzero, indicates that the machine's tick routine should be
   triggered periodically, every so many cycles. Typically this is
   used by the machine to generate a timer interrupt. Off By default.
*/
unsigned int cycles_per_tick = 10;

/* Nonzero if debugging support is turned on */
BOOLEAN debug_enabled = FALSE;

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
BOOLEAN binary = FALSE;

char *machine_name = "bloo";

char *prog_name = NULL;

void sim_set_debug(BOOLEAN bool)
{
	debug_enabled = bool;
} 

BOOLEAN sim_get_debug_status()
{
	return (debug_enabled);
}


void sim_set_binary(BOOLEAN bool)
{
        binary = bool;
} 

void sim_set_machine_name(char *name)
{
    machine_name = name;
} 

void sim_set_prog_name(char *name)
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

	cycles = machine_get_cycles ();
	sim_ms = (cycles - last_cycles) / cycles_per_ms;
	if (sim_ms < 0)
		sim_ms += cycles_per_ms;
	last_cycles = cycles;

	total_ms_elapsed += sim_ms;
	if (total_ms_elapsed > 100)
	{
		total_ms_elapsed -= 100;
		machine_periodic ();
		debugger_periodic();
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
	int rc;
    init_time();
	sym_init();
    if (binary)
	{
		/* Binary option: Load directly the .bin during machine_init*/
		machine_init (machine_name, prog_name);
	}
	else
	{
		/* The machine loader cannot deal with image files, so initialize the machine first, passing it a NULL
		filename, then load the image file in S19 or hex format through the debugger afterwards. */
		machine_init (machine_name, NULL);
		debugger_load_image(prog_name);
	}
	/* OK, ready to run.  Reset the machine first. */
	if (prog_name)
		machine_reset ();
	else
		sim_set_debug(TRUE);
		
	debugger_init ();
}

int sim_run()
{
/* Now, iterate through the instructions.
           Without -I, we can just call m6809_execute() and let it run
           for a long time; otherwise, we need to come back here
           periodically and call the machine's ->tick() routine */
        //[NAC HACK 2017Mar30] need to schedule this properly instead of this one-or-the-other approach
        //.. need to track the rate of each and work out who's next.
	unsigned long nb_cycles;
	do
	{
		/* Call each device that needs periodic processing. */
		machine_update ();
		if (cycles_per_tick == 0)
		{
			/* Simulate some CPU time, either 1ms worth or up to the
			next possible tick */
			machine_run (sim_freq * 1000);
		}
		else
		{
			nb_cycles = machine_run (cycles_per_tick);
			machine_tick (nb_cycles);
		}

		/* Align with real time*/
		idle_loop ();

		/* Check for a rogue program that won't end */
		/*
		if ((max_cycles > 0) && (m6809_get_cycles() > max_cycles))
		{
			sim_error ("maximum cycle count exceeded at %s\n",
				monitor_addr_name (m6809_get_pc ()));
		}
		*/
	} while(debugger_get_exitcmd() != TRUE);
	log_message(DEBUG, "Exiting");
	sim_exit (0);
	return 0;
}


/*
void sim_error (const char *format, ...)
{
	va_list ap;

	va_start (ap, format);
	//fprintf (stderr, "m6809-run: (at PC=%04X) ", iPC);
	vfprintf (stderr, format, ap);
	va_end (ap);

	if (debug_enabled)
		debugger_set_status(ACTIVATED);
	else {
		debugger_exit();
		exit (2);
        }
}
*/

void sim_exit (uint8_t exit_code)
{
	char *s;

	/* On a nonzero exit, always print an error message. */
	/*
	if (exit_code != 0)
	{
		printf ("m6809-run: program exited with %d\n", exit_code);
		if (exit_code)
			monitor_backtrace ();
	}
	*/

	/* If a cycle count should be printed, do that last. */
	/*
	if (dump_cycles_on_success)
	{
		printf ("%s : %ld cycles, %ld ms\n", prog_name, m6809_get_cycles (),
			get_elapsed_realtime ());
	}
	*/

	/*
	if ((s = getenv ("LOG6809")) != NULL)
	{
		FILE *fp = fopen (s, "a");
		if (fp)
		{
			fprintf (fp, "%s : %ld cycles, %ld ms\n", prog_name, m6809_get_cycles (),
				get_elapsed_realtime ());
			fclose (fp);
		}
	}
	*/
	debugger_exit();
	exit (exit_code);
}