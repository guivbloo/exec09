#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include "cli_monitor.h"
#include "monitor.h"
#include "symtab.h"
#include "m6809.h"
#include "bus_access.h"
#include "types.h"
#include "machine.h"

#define MAX_DISPLAYS 32
#define MAXLINE 256
#define MAX_THREADS 64
#define MAX_CMD_QUEUES 8
#define IRQ_CYCLE_COUNTS 128
#define PROMPT "(dbg) "
#define SYM_AUTO 1

#define MAX_HISTORY 10


typedef void (*command_handler_t) (void);

/* Ensure breaktab is declared */
extern breakpoint_t breaktab[MAX_BREAKS];
extern int auto_break_insn_count;
extern unsigned int trace_offset;
extern target_addr_t trace_buffer[MAX_TRACE];
extern unsigned int active_break_count;
extern int dump_every_insn;


struct termios old_tio, new_tio;

extern unsigned int history_count;
extern unsigned long historytab[MAX_HISTORY];

absolute_address_t examine_addr = 0;
unsigned int examine_repeat = 1;
datatype_t examine_type;



typedef struct
{
   int id : 8;
   thread_id_t tid;
} thread_t;


thread_t threadtab[MAX_THREADS];


int command_stack_depth = -1;
typedef struct
{
   unsigned int size;
   unsigned int count;
   char **strings;
} cmdqueue_t;




cmdqueue_t command_stack[MAX_CMD_QUEUES];


int stop_after_ms = 0;

datatype_t print_type;

char *command_flags;

int exit_command_loop;

int exit_command = 1;

/* Display debugger */
BOOLEAN display_debug = TRUE;


unsigned int irq_cycle_tab[IRQ_CYCLE_COUNTS] = { 0, };
unsigned int irq_cycle_entry = 0;
unsigned long irq_cycles = 0;

unsigned int display_count = 0;
display_t displaytab[MAX_DISPLAYS];

absolute_address_t thread_id = 0;


FILE *command_input;

static void monitor_signal (int sigtype);

void monitor_set_display_debug(BOOLEAN status)
{
   if (status)
   {
      display_debug = TRUE;
      signal(SIGINT, SIG_DFL);
   }
   else
   {
      display_debug = FALSE;
      signal(SIGINT, monitor_signal);
   }
}

BOOLEAN monitor_get_display_debug (void)
{
	return display_debug;
}

void breakpoint_hit (breakpoint_t *br)
{
   /* TODO don't know how best to handle errors here. */
   char eflag = 0; /* unused */
   if (br->threaded && (thread_id != br->tid))
      return;
/*
   if (br->conditional)
   {
      if (eval (br->condition, &eflag) == 0)
         return;
   }
         */

   if (br->ignore_count)
   {
      --br->ignore_count;
      return;
   }

   if(br->keep_running == 0)
	{
		monitor_set_display_debug(TRUE);
	}
	else
	{
		monitor_set_display_debug(FALSE);	
	}
}

static void monitor_signal (int sigtype)
{
   (void) sigtype;
   putchar ('\n');
   monitor_set_display_debug(TRUE);
}


// 1. Fonctions utilitaires et helpers
void syntax_error (const char *string)
{
   fprintf (stderr, "error: %s\n", string);
}

void report_errors (const char eflag)
{
   if (eflag &    1) fprintf (stderr, "error: bad operator in expression\n");
   if (eflag &    2) fprintf (stderr, "error: bad lvalue in expression\n");
   if (eflag &    4) fprintf (stderr, "error: bad rvalue in expression\n");
   if (eflag &    8) fprintf (stderr, "error: non-existent symbol in expression\n");
   if (eflag & 0x10) fprintf (stderr, "error: non-existent $symbol in expression\n");
   if (eflag & 0x20) fprintf (stderr, "error: bad numeric literal\n");
   if (eflag & 0x40) fprintf (stderr, "error: unrecognised $symbol in assignment\n");
   if (eflag & 0x80) fprintf (stderr, "error: missing assignment\n");
}

void save_value (unsigned long val)
{
   historytab[history_count++ % MAX_HISTORY] = val;
}



void print_device_name (unsigned int devno)
{
   printf ("%02X", devno);
}

void print_addr (absolute_address_t addr)
{
   const char *name;
   print_device_name (addr >> 28);
   putchar (':');
   printf ("0x%04lX", addr & 0xFFFFFF);

   name = sym_lookup (PROGRAM_SYMTAB_T, addr);
   if (name)
      printf ("  %-18.18s", name);
   else
      printf ("%-20.20s", "");
}



void brkprint (breakpoint_t *brkpt)
{
   if (!brkpt->used)
      return;

   if (brkpt->on_execute)
      printf ("Breakpoint");
   else
   {
      printf ("Watchpoint");
      if (brkpt->on_read)
         printf ("(%s)", brkpt->on_write ? "RW" : "RO");
   }

   printf (" %d at ", brkpt->id);
   print_addr (brkpt->addr);
   if (!brkpt->enabled)
      printf (" (disabled)");
   if (brkpt->conditional)
      printf (" if %s", brkpt->condition);
   if (brkpt->threaded)
      printf (" on thread %d", brkpt->tid);
   if (brkpt->keep_running)
      printf (", print-only");
   if (brkpt->temp)
      printf (", temp");
   if (brkpt->ignore_count)
      printf (", ignore %d times\n", brkpt->ignore_count);
   if (brkpt->write_mask)
      printf (", mask 0x%02X\n", brkpt->write_mask);
   putchar ('\n');
}






/* Extract any valid format flags - ignore anything else
 * allows format and size flags to be mingled but provides no way
 * to detect and report unrecognised flags)
 */
void parse_format_flag (const char *flags, unsigned char *formatp)
{
   while (*flags)
   {
      switch (*flags)
      {
         case 'X':
         case 'x':
         case 'd':
         case 'u':
         case 'o':
         case 'a':
         case 's':
         case 'c':
            *formatp = *flags;
            break;
      }
      flags++;
   }
}

/* Extract any valid size flags - ignore anything else
 * (allows format and size flags to be mingled but provides no way
 * to detect and report unrecognised flags)
 */
void parse_size_flag (const char *flags, unsigned int *sizep)
{
   while (*flags)
   {
      switch (*flags++)
      {
         case 'b':
            *sizep = 1;
            break;
         case 'w':
            *sizep = 2;
            break;
      }
   }
}












void print_value (unsigned long val, datatype_t *typep)
{
   char f[8];

   switch (typep->format)
   {
      case 'a':
         print_addr (val);
         return;

      case 'c':
      {
         char c;
         c = val;
         if ((c < 32) | (c > 126)) c = '.';
         putchar(c);
         return;
      }

      case 's':
      {
         absolute_address_t addr = (absolute_address_t)val;
         char c;

         putchar ('"');
         while ((c = bus_read8_abs (addr++)) != '\0')
            putchar (c);
         putchar ('"');
         return;
      }

      case 't':
         /* TODO : print as binary integer */
         break;
   }

   if ((typep->format == 'x') | (typep->format == 'X'))
   {
      printf ("0x");
      sprintf (f, "%%0%d%c", typep->size * 2, typep->format);
   }
   else if (typep->format == 'o')
   {
      printf ("0");
      sprintf (f, "%%%c", typep->format);
   }
   else
      sprintf (f, "%%%c", typep->format);

   printf (f, val);
}



int print_insn (absolute_address_t addr)
{
   char buf[64];
   int size = dasm (buf, addr);
   printf ("%s", buf);
   return size;
}

void do_examine (void)
{
   unsigned int n;
   unsigned int objs_per_line = 16;

   if (isdigit (*command_flags))
      examine_repeat = strtoul (command_flags, &command_flags, 0);

   if (*command_flags == 'i')
      examine_type.format = *command_flags;
   else
      parse_format_flag (command_flags, &examine_type.format);

   parse_size_flag (command_flags, &examine_type.size);

   switch (examine_type.format)
   {
      case 'i':
         objs_per_line = 1;
         break;

      case 'w':
         objs_per_line = 8;
         break;

      case 'c':
         objs_per_line = 32;
         break;
   }

   for (n = 0; n < examine_repeat; n++)
   {
      if ((n % objs_per_line) == 0)
      {
         if (n > 0) putchar ('\n');
         print_addr (examine_addr);
         printf (": ");
      }

      switch (examine_type.format)
      {
         case 's': /* string */
            break;

         case 'i': /* instruction */
            examine_addr += print_insn (examine_addr);
            break;

         default:
            print_value (target_read (examine_addr, examine_type.size),
                         &examine_type);
            if (examine_type.format != 'c') putchar (' ');
            examine_addr += examine_type.size;
      }
   }
   putchar ('\n');
}



void do_set (char *expr)
{
   char eflag = 0;

   (void)eval (expr, &eflag);
   /* too late to prevent an assignment, but at least report problems */
   if (eflag)
      report_errors(eflag);
}

/* TODO - WPC */
#define THREAD_DATA_PC 3
#define THREAD_DATA_ROMBANK 9

void print_thread_data (absolute_address_t th)
{
   uint8_t b;
   uint16_t w;
   absolute_address_t pc;

   w = bus_read16_abs (th + THREAD_DATA_PC);
   b = bus_read8_abs (th + THREAD_DATA_ROMBANK);
   if (w >= 0x8000)
      pc = 0xF0000 + w;
   else
      pc = (b * 0x4000) + (w - 0x4000);
   pc = MAKE_ADDR (1, pc);
   print_addr (pc);
}

/* TODO command_stack and command_stack_* are unused */
void command_stack_push (unsigned int reason)
{
   cmdqueue_t *q = &command_stack[++command_stack_depth];
}

void command_stack_pop (void)
{
   cmdqueue_t *q = &command_stack[command_stack_depth];
   --command_stack_depth;
}

void command_stack_add (const char *cmd)
{
   cmdqueue_t *q = &command_stack[command_stack_depth];
}

static int print_insn_long (absolute_address_t addr)
{
   char buf[64];
   int i;
   int size = dasm(buf, addr);

   const char* name;
   printf(PROMPT);
   print_device_name(addr >> 28);
   putchar(':');
   printf("0x%04lX ", addr & 0xFFFFFF);

   for (i = 0; i < size; i++)
      printf("%02X", bus_read8_abs(addr + i));

   for (i = 0; i < 4 - size; i++)
      printf("  ");

   name = sym_lookup(PROGRAM_SYMTAB_T, addr);
   if (name)
      printf("  %-12.12s", name);
   else
      printf("%-14.14s", "");

   printf("%s", buf);
   putchar ('\n');
   return size;
}

display_t* display_alloc ()
{
   unsigned int n;
   for (n = 0; n < MAX_DISPLAYS; n++)
   {
      display_t *ds = &displaytab[n];
      if (!ds->used)
      {
         ds->used = 1;
         return ds;
      }
   }
   return NULL;
}



/**
 * @brief Retrieves the next argument from the command line input.
 *
 * This function is typically used to parse and return the next argument
 * provided by the user in a command-line interface. The returned string
 * points to the argument, or NULL if there are no more arguments.
 *
 * @return A pointer to the next argument as a null-terminated string,
 *         or NULL if no more arguments are available.
 */
char* getarg (void)
{
   return strtok (NULL, " \t\n");
}


void do_print (char *expr)
{
   char eflag = 0;
   unsigned long val = eval (expr, &eflag);

   parse_format_flag (command_flags, &print_type.format);
   parse_size_flag (command_flags, &print_type.size);

   if (eflag)
      report_errors(eflag);
   else
   {
      printf ("$%d = ", history_count);
      print_value (val, &print_type);
      putchar ('\n');
      save_value (val);
   }
}

int command_exec_file (const char *filename)
{
   FILE *infile;
   extern int command_exec (FILE *);

   infile = fopen (filename, "r");
   if (!infile)
      return 0;

   command_input = infile;
   return 1;
}

/
/**
 * Handles the "set" command, which allows the user to either set an internal variable
 * or write to memory. If the argument is "var", it creates or updates an entry in the
 * user symbol table. Otherwise, it performs a memory write operation.
 */
void cmd_set (void)
{
   char *arg = getarg ();

   if (!strcmp (arg, "var"))
   {
      /* this form allows the creation of entries
       * in the user symbol table.
       */
      char *p;
      char eflag = 0;
      unsigned long val;

      arg = getarg ();
      if ((p = strchr (arg, '=')) != NULL)
      {
         *p++ = '\0';
         val = eval (p, &eflag); /* Evaluate RHS */
         if (eflag)
            report_errors(eflag);
         else
            sym_set (INTERNAL_SYMTAB_T, arg, val, 0);
      }
      else
      {
         report_errors(0x80);
      }
   }
   else
   {
      /* this form is a memory write */
      if (arg)
         do_set (arg);
   }
}



void cmd_examine (void)
{
   char eflag = 0;
   char *arg = getarg ();
   if (arg)
      examine_addr = eval_mem (arg, LVALUE, &eflag);

   if (eflag)
      report_errors(eflag);
   else
      do_examine ();
}

void cmd_break (void)
{
   char eflag = 0;
   char *arg = getarg ();

   if (!arg)
      return;

   unsigned long val = eval_mem (arg, LVALUE, &eflag);

   if (eflag)
      report_errors(eflag);
   else
   {
      breakpoint_t *br = brkalloc ();
      br->addr = val;
      br->on_execute = 1;

      arg = getarg ();
      if (!arg);
      else if (!strcmp (arg, "if"))
      {
         br->conditional = 1;
         arg = getarg ();
         strcpy (br->condition, arg);
      }
      else if (!strcmp (arg, "ignore"))
      {
         br->ignore_count = atoi (getarg ());
      }

      brkprint (br);
   }
}

void cmd_watch1 (int on_read, int on_write)
{
   char eflag = 0;
   char *arg = getarg ();

   if (!arg)
      return;

   absolute_address_t addr = eval_mem (arg, LVALUE, &eflag);

   if (eflag)
      report_errors(eflag);
   else
   {
      breakpoint_t *br = brkalloc ();
      br->addr = addr;
      br->on_read = on_read;
      br->on_write = on_write;

      for (;;)
      {
         arg = getarg ();
         if (!arg)
            break;

         if (!strcmp (arg, "print"))
            br->keep_running = 1;
         else if (!strcmp (arg, "mask"))
         {
            arg = getarg ();
            br->write_mask = strtoul (arg, NULL, 0);
         }
         else if (!strcmp (arg, "if"))
         {
            arg = getarg ();
            br->conditional = 1;
            strcpy (br->condition, arg);
         }
      }

      brkprint (br);
   }
}

void cmd_watch (void)
{
   cmd_watch1 (0, 1);
}

void cmd_rwatch (void)
{
   cmd_watch1 (1, 0);
}

void cmd_awatch (void)
{
   cmd_watch1 (1, 1);
}



void cmd_break_list (void)
{
   unsigned int n;
   for (n = 0; n < MAX_BREAKS; n++)
      brkprint (&breaktab[n]);
}

void cmd_step (void)
{
   char *arg = getarg ();
   if (arg && (auto_break_insn_count = atoi(arg)));
   else
      auto_break_insn_count = 1;

   exit_command_loop = 0;
}

void cmd_next (void)
{
   char buf[128];
   breakpoint_t *br;

   unsigned long addr = to_absolute (m6809_get_pc ());
   addr += dasm (buf, addr);

   br = brkalloc ();
   br->addr = addr;
   br->on_execute = 1;
   br->temp = 1;

   /* TODO - for conditional branches, should also set a
      temp breakpoint at the branch target */

   exit_command_loop = 0;
}

void cmd_continue (void)
{
   monitor_set_display_debug(FALSE);
   exit_command_loop = 0;
}

void cmd_quit (void)
{
   exit_command = 0;
   exit_command_loop = 1;
}

void cmd_delete (void)
{
   const char *arg = getarg ();
   unsigned int id;

   if (!arg)
   {
      printf ("Deleting all breakpoints.\n");
      for (id = 0; id < MAX_BREAKS; id++)
      {
         breakpoint_t *br = brkfind_by_id (id);
         brkfree (br);
      }
      return;
   }

   id = atoi (arg);
   breakpoint_t *br = brkfind_by_id (id);
   if (br->used)
   {
      printf ("Deleting breakpoint %d\n", id);
      brkfree (br);
   }
}

void cmd_list (void)
{
   char eflag = 0;
   char *arg = getarg ();
   static absolute_address_t lastpc = 0;
   static absolute_address_t lastaddr = 0;
   absolute_address_t addr;
   int n;

   if (arg)
      addr = eval_mem (arg, LVALUE, &eflag);
   else
   {
      addr = to_absolute (m6809_get_pc ());
      if (addr == lastpc)
         addr = lastaddr;
      else
         lastaddr = lastpc = addr;
   }

   if (eflag)
      report_errors(eflag);
   else
   {
      for (n = 0; n < 16; n++)
      {
         addr += print_insn_long(addr);
      }

      lastaddr = addr;
   }
}

void cmd_symbol_file (void)
{
   char *arg = getarg ();
   if (arg)
      monitor_load_map_file (arg);
}

void cmd_display (void)
{
   char eflag = 0;
   char *arg;

   while ((arg = getarg ()) != NULL)
   {
      display_t *ds = display_alloc ();
      strcpy (ds->expr, arg);
      ds->type = print_type;
      parse_format_flag (command_flags, &ds->type.format);
      parse_size_flag (command_flags, &ds->type.size);
      if (eflag)
      {
         report_errors(eflag);
         break;
      }
   }
}



void cmd_source (void)
{
   char *arg = getarg ();
   if (!arg)
      return;

   if (command_exec_file (arg) == 0)
      fprintf (stderr, "can't open %s\n", arg);
}

void cmd_regs (void)
{
   m6809_print_regs();
}

void cmd_pc(void)
{
   char eflag = 0;
   unsigned long val;
   char* arg = getarg();

   if (!arg)
      return;

   val = eval_mem(arg, LVALUE, &eflag);

   if (eflag)
      report_errors(eflag);
   else
   {
      m6809_set_pc(val);
      cmd_list();
   }
}

void cmd_vars (void)
{
   char* arg = getarg();
   if (arg && !strcmp(arg, "auto"))
   {
      printf("Print auto\n");
      symtab_print (AUTO_SYMTAB_T);
   }
   else if (arg && !strcmp(arg, "internal"))
   {
      printf("Print internal\n");
      symtab_print (INTERNAL_SYMTAB_T);
   }
   else
   {
      symtab_print (PROGRAM_SYMTAB_T);
   }
   
}

void cmd_runfor (void)
{
   char eflag = 0;
   char *units;
   char *arg = getarg ();

   if (!arg)
      return;

   unsigned long val = atoi (arg);

   /* do the check here because, even if there is
      an error with the new args, we will abandon
      any previous runfor that's in progress
   */
   if (stop_after_ms != 0)
      printf ("Previous 'runfor' abandoned\n");

   units = getarg ();
   if (!units || !strcmp (units, "ms"))
      stop_after_ms = val;
   else if (!strcmp (units, "s"))
      stop_after_ms = val * 1000;
   else
      eflag = 1;

   if (eflag)
      fprintf (stderr, "error: bad time units\n");
   else if (val == 0)
      fprintf (stderr, "error: bad time value\n");
   else
      exit_command_loop = 0;
}

void cmd_measure (void)
{
   char eflag = 0;
   absolute_address_t addr;
   target_addr_t retaddr = m6809_get_pc ();
   breakpoint_t *br;

   /* Get the address of the function to be measured. */
   char *arg = getarg ();

   if (!arg)
      return;

   addr = eval_mem (arg, LVALUE, &eflag);
   if (eflag)
      report_errors(eflag);
   else
   {
      printf ("Measuring ");
      print_addr (addr);
      printf (" back to ");
      print_addr (to_absolute (retaddr));
      putchar ('\n');

      /* Push the current PC onto the stack for the
         duration of the measurement. */
      m6809_set_s (m6809_get_s () - 2);
      bus_write16 (m6809_get_s (), retaddr);

      /* Set a temp breakpoint at the current PC, so that
         the measurement will halt. */
      br = brkalloc ();
      br->addr = to_absolute (retaddr);
      br->on_execute = 1;
      br->temp = 1;

      /* Interrupts must be disabled for this to work ! */
      m6809_set_cc (m6809_get_cc () | 0x50);

      /* Change the PC to the function-under-test. */
      m6809_set_pc (addr);

      /* Go! */
      exit_command_loop = 0;
   }
}

void cmd_dump_insns (void)
{
   char *arg = getarg ();
   if (arg)
      dump_every_insn = strtoul (arg, NULL, 0);
   printf ("Instruction dump is %s\n",
           dump_every_insn ? "on" : "off");
}

void cmd_trace_dump (void)
{
   unsigned int off = (trace_offset + 1) % MAX_TRACE;
   do {
      target_addr_t pc = trace_buffer[off];
      absolute_address_t addr = to_absolute (pc);
      print_insn_long(addr);
      off = (off + 1) % MAX_TRACE;
   } while (off != trace_offset);
   fflush (stdout);
}

void cmd_dump (void)
{
   machine_dump();
}

void cmd_restore (void)
{
   printf("not implemented\n");
}

void cmd_info (void)
{
   machine_describe();
}

void cmd_print (void)
{
   char *arg = getarg ();

   if (arg)
      do_print (arg);
   else
      do_print ("$");
}

void cmd_help (void);



struct command_name
{
      const char *prefix;
      const char *name;
      command_handler_t handler;
      const char *help;
} cmdtab[] = {
   { "p", "print", cmd_print,
     "Print the value of an expression" },
   { "set", "set", cmd_set,
     "Set an internal variable/target memory" },
   { "x", "examine", cmd_examine,
     "Examine raw memory" },
   { "b", "break", cmd_break,
     "Set a breakpoint" },
   { "bl", "blist", cmd_break_list,
     "List all breakpoints/watchpoints" },
   { "d", "delete", cmd_delete,
     "Delete a breakpoint/watchpoint" },
   { "s", "step", cmd_step,
     "Step one (or more) instructions" },
   { "n", "next", cmd_next,
     "Break at the next instruction" },
   { "c", "continue", cmd_continue,
     "Continue the program" },
   { "fg", "foreground", cmd_continue, NULL },
   { "q", "quit", cmd_quit,
     "Quit the simulator" },
   { "re", "reset", m6809_reset,
     "Reset the CPU" },
   { "h", "help", cmd_help,
     "Display this help" },
   { "wa", "watch", cmd_watch,
     "Add a watchpoint on write" },
   { "rwa", "rwatch", cmd_rwatch,
     "Add a watchpoint on read" },
   { "awa", "awatch", cmd_awatch,
     "Add a watchpoint on read/write" },
   { "?", "?", cmd_help },
   { "l", "list", cmd_list },
   { "sym", "symbol-file", cmd_symbol_file,
     "Open a symbol table file" },
   { "di", "display", cmd_display,
     "Add a display expression" },
   { "so", "source", cmd_source,
     "Run a command script" },
   { "regs", "regs", cmd_regs,
     "Show all CPU registers" },
   { "vars", "vars", cmd_vars,
     "Show all program variables" },
   { "runfor", "runfor", cmd_runfor,
     "Run for a certain amount of time" },
   { "me", "measure", cmd_measure,
     "Measure time that a function takes" },
   { "dumpi", "dumpi", cmd_dump_insns,
     "Set dump-instruction flag" },
   { "td", "tracedump", cmd_trace_dump,
     "Dump the trace buffer" },
   { "dump", "du", cmd_dump,
     "Dump contents of memory to a file" },
   { "restore", "res", cmd_restore,
     "Restore contents of memory from a file" },
   { "i", "info", cmd_info,
     "Describe machine, devices and address mapping" },
   { "pc", "pc", cmd_pc,
     "Set program counter" },
#if 0
   { "cl", "clear", cmd_clear },
   { "co", "condition", cmd_condition },
   { "tr", "trace", cmd_trace },
   { "di", "disable", cmd_disable },
   { "en", "enable", cmd_enable },
   { "f", "file", cmd_file,
     "Choose the program to be debugged" },
   { "exe", "exec-file", cmd_exec_file,
     "Open an executable" },
#endif
   { NULL, NULL },
};


void cmd_help (void)
{
   struct command_name *cn = cmdtab;
   while (cn->prefix != NULL)
   {
      if (cn->help)
         printf ("(dbg) %s (%s) - %s\n",
                 cn->name, cn->prefix, cn->help);
      cn++;
   }
}









/* Non-blocking check for input character. If
 *   true, retrieve character using kbchar()
 */
int kbhit(void)
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int kbchar(void)
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}












void command_periodic ( unsigned long nb_cycles_executed)
{
   if (stop_after_ms)
   {
      stop_after_ms -= 100;
      if (stop_after_ms <= 0)
      {
         monitor_set_display_debug(TRUE);
         stop_after_ms = 0;
         printf ("Stopping after time elapsed.\n");
      }
   }
}

void pc_virtual (unsigned long *val, int writep) {
   if (writep) m6809_set_pc (*val);
   else *val = m6809_get_pc ();
}
void x_virtual (unsigned long *val, int writep) {
   if (writep) m6809_set_x (*val);
   else *val = m6809_get_x ();
}
void y_virtual (unsigned long *val, int writep) {
   if (writep)
      m6809_set_y (*val);
   else *val = m6809_get_y ();
}
void u_virtual (unsigned long *val, int writep) {
   if (writep)
      m6809_set_u (*val);
   else
      *val = m6809_get_u ();
}
void s_virtual (unsigned long *val, int writep) {
   if (writep)
      m6809_set_s (*val);
   else
      *val = m6809_get_s ();
}
void d_virtual (unsigned long *val, int writep) {
   if (writep)
      m6809_set_d (*val);
   else
      *val = m6809_get_d ();
}
void a_virtual (unsigned long *val, int writep) {
   if (writep)
      m6809_set_a (*val);
   else
      *val = m6809_get_a ();
}
void b_virtual (unsigned long *val, int writep) {
   if (writep)
      m6809_set_b (*val);
   else
      *val = m6809_get_b ();
}
void dp_virtual (unsigned long *val, int writep) {
   if (writep)
      m6809_set_dp (*val);
   else
      *val = m6809_get_dp ();
}
void cc_virtual (unsigned long *val, int writep) {
   if (writep)
      m6809_set_cc (*val);
   else
      *val = m6809_get_cc ();
}
void irq_load_virtual (unsigned long *val, int writep) {
   if (!writep)
      *val = irq_cycles / IRQ_CYCLE_COUNTS;
}

void cycles_virtual (unsigned long *val, int writep)
{
   if (!writep)
      *val = m6809_get_cycles ();
}

void et_virtual (unsigned long *val, int writep)
{
   static unsigned long last_cycles = 0;
   if (!writep)
      *val = m6809_get_cycles () - last_cycles;
   last_cycles = m6809_get_cycles ();
}

/**
 * Update the $irqload virtual register, which tracks the
 * average number of cycles spent in IRQ.  This function
 * maintains a rolling history of IRQ_CYCLE_COUNTS entries.
 */
void command_exit_irq_hook (unsigned long cycles)
{
   irq_cycles -= irq_cycle_tab[irq_cycle_entry];
   irq_cycles += cycles;
   irq_cycle_tab[irq_cycle_entry] = cycles;
   irq_cycle_entry = (irq_cycle_entry + 1) % IRQ_CYCLE_COUNTS;
}



BOOLEAN command_get_exitcmd(void)
{
   if(exit_command == 0)
      return TRUE;
   return FALSE;
}








command_handler_t command_lookup (const char *cmd)
{
   struct command_name *cn;
   char *p;

   p = strchr (cmd, '/');
   if (p)
   {
      *p = '\0';
      command_flags = p+1;
   }
   else
      command_flags = "";

   cn = cmdtab;
   while (cn->prefix != NULL)
   {
      if (!strcmp (cmd, cn->prefix))
         return cn->handler;
      if (!strcmp (cmd, cn->name))
         return cn->handler;
      /* TODO - look for a match anywhere between
       * the minimum prefix and the full name */
      cn++;
   }
   return NULL;
}


/* Lit la commande utilisateur et l'exécute en fonction de la défintion dans la table des commandes cmdtab */
int command_exec (FILE *infile)
{
   char buffer[MAXLINE];
   static char prev_buffer[MAXLINE];
   char *cmd;
   command_handler_t handler;

   // Read user input to buffer and store including \n
    #ifdef HAVE_READLINE
      if (infile == stdin)
      {
         char *line_read = readline(PROMPT);
      if (line_read == NULL)
         return -1;
      if (strlen(line_read) > sizeof(buffer) - 2)
      {
			syntax_error("line too long");
         return 0;
      }
		strcpy (buffer, line_read);
        strcat (buffer, "\n");
        if (buffer[0] != '\n')
         add_history (buffer);
        free(line_read);
        }
#else
   if (infile == stdin)
   {
      printf(PROMPT);
      fgets(buffer, 255, stdin);
		
   }
#endif
   if (infile != stdin)
   {
      fgets(buffer, sizeof(buffer), infile);
      if (feof(infile))
         return -1;
   }

   /* In terminal mode, a blank line means to execute
      the previous command. */
   if ((infile == stdin) && (buffer[0] == '\n'))
      strcpy (buffer, prev_buffer);

   /* Skip comments */
   if (*buffer == '#')
      return 0;

   cmd = strtok (buffer, " \t\n");
   if (!cmd)
      return 0;

   strcpy (prev_buffer, cmd);
   handler = command_lookup (cmd);
   if (!handler)
   {
      syntax_error ("no such command");
      return 0;
   }

   (*handler) ();
   return 0;
}




void display_free (display_t *ds)
{
}

void display_print (void)
{
   char eflag = 0;
   unsigned int n;
   char comma = '\0';

   for (n = 0; n < MAX_DISPLAYS; n++)
   {
      display_t *ds = &displaytab[n];
      if (ds->used)
      {
         char expr[256];
         strcpy (expr, ds->expr);
         printf ("%c %s = ", comma, expr);
         print_value (eval (expr, &eflag), &ds->type);
         comma = ',';
      }
   }

   if (comma)
      putchar ('\n');
   if (eflag)
      report_errors(eflag);
}


void keybuffering_defaults (void)
{
#ifndef _MSC_VER

   /* get two copies of the terminal settings for stdin */
   tcgetattr(STDIN_FILENO,&old_tio);
   tcgetattr(STDIN_FILENO,&new_tio);

   /* disable canonical mode (buffered i/o) and local echo */
   new_tio.c_lflag &=(~ICANON & ~ECHO);

#endif
}

void keybuffering (int flag)
{
#ifndef _MSC_VER
   if (flag) {
      tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);
   }
   else {
      tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
   }
#endif
}



void print_current_insn (void)
{
   print_insn_long(to_absolute(m6809_get_pc()));
}

void command_insn_hook (void)
{
   target_addr_t pc;
   absolute_address_t abspc;
   breakpoint_t *br;

   pc = m6809_get_pc ();
   command_trace_insn (pc);
   if(dump_every_insn != 0)
   {
      print_current_insn();
   }
   if (check_break () != 0)
			monitor_set_display_debug(TRUE);

   if (active_break_count == 0)
      return;

   abspc = to_absolute (pc);
   br = brkfind_by_addr (abspc);
   if (br && br->enabled && br->on_execute)
   {
      breakpoint_hit (br);
      if (monitor_get_display_debug() == 0)
         return;
      if (br->temp)
         brkfree (br);
      else
         printf ("Breakpoint %d reached.\n", br->id);
   }
}


void command_read_hook (absolute_address_t addr)
{
   breakpoint_t *br;

   if (active_break_count == 0)
      return;

   br = brkfind_by_addr (addr);
   if (br && br->enabled && br->on_read)
   {
      printf ("Watchpoint %d triggered. [pc=0x%04X ", br->id, m6809_get_pc());
      print_addr (addr);
      printf ("]\n");
      breakpoint_hit (br);
   }
}

void command_write_hook (absolute_address_t addr, uint8_t val)
{
   breakpoint_t *br;
   if (active_break_count != 0)
   {
      br = brkfind_by_addr (addr);
      if (br && br->enabled && br->on_write)
      {
         if (br->write_mask)
         {
            int mask_ok = ((br->last_write & br->write_mask) !=
                           (val & br->write_mask));
            br->last_write = val;
            if (!mask_ok)
               return;
         }

         breakpoint_hit (br);

         printf ("Watchpoint %d triggered. [pc=0x%04X ", br->id, m6809_get_pc());
         print_addr (addr);
         printf (" = 0x%02X]\n", val);
      }
   }
}

int command_loop (void)
{
    /* Active le keybuffering*/
   keybuffering (1);
   /* Vide la table des breakpoints*/
   /*[TODO] Pourquoi ?*/
   brkfree_temps ();
  restart:
    /*Affiche systématiquement une expression à chaque arrêt du debuggeur ou après next*/
    display_print ();
    /* Affiche la prochaine instruction à exécuter */
    print_current_insn ();
   exit_command_loop = -1;
   while (exit_command_loop < 0)
   {

      if (command_exec (command_input) < 0)
         break;
   }
   if (exit_command_loop == 0)
      keybuffering (0);

   if (feof (command_input) && command_input != stdin)
   {
      fclose (command_input);
      command_input = stdin;
      goto restart;
   }

   return (exit_command_loop);
}

void cli_monitor_init (void)
{
   monitor_init();
   /* Install virtual registers.  These are referenced in expressions
    * using a dollar-sign prefix (e.g. $pc).  The value of the
    * symbol is a pointer to a function (e.g. pc_virtual) which
    * computes the value dynamically. */
   sym_add (AUTO_SYMTAB_T, "pc", (unsigned long)pc_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "x", (unsigned long)x_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "y", (unsigned long)y_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "u", (unsigned long)u_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "s", (unsigned long)s_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "d", (unsigned long)d_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "a", (unsigned long)a_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "b", (unsigned long)b_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "dp", (unsigned long)dp_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "cc", (unsigned long)cc_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "cycles", (unsigned long)cycles_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "et", (unsigned long)et_virtual, SYM_AUTO);
   sym_add (AUTO_SYMTAB_T, "irqload", (unsigned long)irq_load_virtual, SYM_AUTO);
   
   examine_type.format = 'X'; /* hex with upper-case A-F */
   examine_type.size = 1;

   print_type.format = 'X';
   print_type.size = 1;

   command_input = stdin;
   bus_read_hook = command_read_hook;
   bus_write_hook = command_write_hook;
   m6809_insn_hook = command_insn_hook;
   /* [TODO] Why ? */
	command_exec_file (".dbinit");
   keybuffering_defaults();
   keybuffering(0);
   signal (SIGINT, monitor_signal);
}

int cli_monitor_run() 
{
    /* C'est le point d'entrée.*/
    /* [TODO] En fonction d'un parametre passé sur la ligne de commande, part en exécution sans afficher le prompt*/
    /* [TODO] Gestion du CTR+C pour sortir de l'exécution*/
    /* Affiche le prompt, la prochaine instruction, recupere la commande utilisateur, l'execute*/
    if(monitor_get_display_debug())
      command_loop();
   if(command_get_exitcmd())
   {
      return 1;
   }
   else
   {
      return 0;
   }
   
    /* [TODO] il faut appeler machine run qui s'exécutera jusqu'à ce qu'une hook s'active  ou autre condition ou CTR+C*/
//    nb_cycles_executed = machine_run();
    /*Check if we have to stop due to a runfor in progress. A mettre dans monitor. On lui fourni le nb cycle qu'on vient d'exécuter*/
    /* Attention quelque part il faut convertir en temps*/
    /* Connexion des hooks fournis par le proc et le bus dans le monitor*/
    /* De la meme maniere, on peut imaginer des hooks dans les devices qui se connectent à des simulateurs dans l'interface graphique*/
    /* Les conditions d'arret du simu:
        - breakpoints
        - duree de run
        - CTR+C
        
        */
}