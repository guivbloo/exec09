#include "cli_monitor.h"
#include "monitor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_DISPLAYS 32
#define MAXLINE 256

struct termios old_tio, new_tio;

unsigned int display_count = 0;
display_t displaytab[MAX_DISPLAYS];

FILE *command_input;

void cmd_print (void);
void cmd_examine (void);
void cmd_set (void);

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


void cmd_print (void)
{
   char *arg = getarg ();

   if (arg)
      do_print (arg);
   else
      do_print ("$");
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

int command_exec_file (const char *filename)
{
   FILE *infile;
   extern int command_exec (FILE *);

   infile = file_open (NULL, filename, "r");
   if (!infile)
      return 0;

   command_input = infile;
   return 1;
}


display_t* display_alloc (void)
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

void print_current_insn (void)
{
   print_insn_long(to_absolute(m6809_get_pc()));
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
   /* [TODO] Why ? */
	(void)command_exec_file (".dbinit");
   keybuffering_defaults();
    keybuffering(0);
}

void cli_monitor_run() 
{
    /* C'est le point d'entrée.*/
    unsigned long nb_cycles_executed;
    /* [TODO] En fonction d'un parametre passé sur la ligne de commande, part en exécution sans afficher le prompt*/
    /* [TODO] Gestion du CTR+C pour sortir de l'exécution*/
    /* Affiche le prompt, la prochaine instruction, recupere la commande utilisateur, l'execute*/
    command_loop();
    /* [TODO] il faut appeler machine run qui s'exécutera jusqu'à ce qu'une hook s'active  ou autre condition ou CTR+C*/
    nb_cycles_executed = machine_run();
    /*Check if we have to stop due to a runfor in progress. A mettre dans monitor. On lui fourni le nb cycle qu'on vient d'exécuter*/
    /* Attention quelque part il faut convertir en temps*/
    command_periodic(nb_cycles_executed);
    /* Connexion des hooks fournis par le proc et le bus dans le monitor*/
    /* De la meme maniere, on peut imaginer des hooks dans les devices qui se connectent à des simulateurs dans l'interface graphique*/
    /* Les conditions d'arret du simu:
        - breakpoints
        - duree de run
        - CTR+C
        
        */
}