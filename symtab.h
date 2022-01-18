#ifndef SYMTAB_H
#define SYMTAB_H

#include "types.h"


#define MAX_SYMBOL_HASH 1009

typedef struct
{
   unsigned char format;
   unsigned int size;
} datatype_t;


struct symbol
{
	char *name;
	unsigned long value;
	datatype_t ty;
	unsigned int type;
	struct symbol *name_chain;
	struct symbol *value_chain;
};

typedef struct
{
   int used : 1;
   datatype_t type;
   char expr[128];
} display_t;


struct symtab
{
   struct symbol *syms_by_name[MAX_SYMBOL_HASH];
   struct symbol *syms_by_value[MAX_SYMBOL_HASH];
   struct symtab *parent;
};

enum symtab_type {
   PROGRAM_SYMTAB_T,
   INTERNAL_SYMTAB_T,
   AUTO_SYMTAB_T
};

struct symbol *sym_add (enum symtab_type symtable, const char *name, unsigned long value, unsigned int type);
void sym_set (enum symtab_type symtable, const char *name, unsigned long value, unsigned int type);
int sym_find (enum symtab_type symtable, const char *name, unsigned long *value, unsigned int type);
const char *sym_lookup (enum symtab_type symtable, unsigned long value);

void sym_init (void);
void symtab_print (enum symtab_type symtable);

#endif
