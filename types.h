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
