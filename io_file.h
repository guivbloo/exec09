/*
 * Copyright 2006-2009 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of GCC6809.
 *
 * GCC6809 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * GCC6809 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GCC6809; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FILEIO_H
#define FILEIO_H

#include <stdio.h>

struct pathlist
{
	int count;
	char *entry[32];
};


void path_init (struct pathlist *path);
void path_add (struct pathlist *path, const char *dir);
FILE * file_open (struct pathlist *path, const char *filename, const char *mode);
FILE * file_require_open (struct pathlist *path, const char *filename, const char *mode);
void file_close (FILE *fp);
int sizeof_file(FILE * file);

#endif /* _FILEIO_H */
