#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "types.h"
#include "logging.h"

char *level_char[] = {"DBG","LOG","ERR"};
enum output log_output;
FILE *fptr;


void print_log_level(int level)
{
    fprintf(fptr, "[%s]", level_char[level]);
}

int log_init(enum output out_flow, char * filename)
{
    int rc=0;
    log_output = out_flow;
    if(out_flow == FILES)
    {
        if (filename == NULL)
        {
            rc = -1;
        }
        else
        {
            fptr = fopen(filename, "w"); 
            if (fptr == NULL) 
            { 
                rc = -2;
            } 
        }
    }
    else
    {
        fptr = stdout;
    }
    return (rc);
}

void log_stop()
{
    fclose(fptr); 
}