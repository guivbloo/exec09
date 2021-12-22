/*
 * Copyright 2008 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of the Portable 6809 Simulator.
 *
 * The Simulator is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Simulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SYMTAB_H
#define SYMTAB_H

#include "types.h"

#define MAX_STRINGSPACE 32000
#define MAX_SYMBOL_HASH 1009

typedef struct
{
   unsigned char format;
   unsigned int size;
} datatype_t;


struct stringspace
{
	char space[32000];
	unsigned int used;
};


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

extern struct symtab program_symtab;
extern struct symtab internal_symtab;
extern struct symtab auto_symtab;

struct symbol *sym_add (struct symtab *symtab, const char *name, unsigned long value, unsigned int type);
void sym_set (struct symtab *symtab, const char *name, unsigned long value, unsigned int type);
int sym_find (struct symtab *symtab, const char *name, unsigned long *value, unsigned int type);
const char *sym_lookup (struct symtab *symtab, unsigned long value);

void sym_init (void);
void symtab_print (struct symtab *symtab);

#endif
