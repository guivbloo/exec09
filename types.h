/*
 * Copyright 2001 by Arto Salmi and Joze Fabcic
 * Copyright 2006 by Brian Dominy <brian@oddchange.com>
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


#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t UINT8;
typedef signed char INT8;

typedef uint16_t UINT16;
typedef signed short INT16;

typedef uint32_t UINT32;

typedef signed int INT32;

typedef uint8_t BOOLEAN;

typedef unsigned long absolute_address_t;
typedef uint16_t target_addr_t;

#define ACTIVATED 1
#define DEACTIVATED 0

#define TRUE 1
#define FALSE 0

#endif /* TYPES_H */
