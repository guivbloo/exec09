
#include "monitor.h"
#include "m6809.h"
#include "machine.h"
#include "command.h"
#include "symtab.h"
#include "io_file.h"
#include "bus_access.h"
#include "types.h"
#include <sys/errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_READLINE
# include <stdio.h>
# include <readline/readline.h>
# include <readline/history.h>
#endif





#define MAX_THREADS 64


#define SYM_DEFAULT 0











typedef struct
{
   unsigned int size;
   unsigned int count;
   char **strings;
} cmdqueue_t;





/**********************************************************/
/********************* Global Data ************************/
/**********************************************************/







unsigned long eval_mem (char *expr, eval_mode_t mode, char *eflag);
static int print_insn_long (absolute_address_t addr);





/**********************************************************/
/******************** 6809 Functions **********************/
/**********************************************************/






/**********************************************************/
/*********************** Functions ************************/
/**********************************************************/

