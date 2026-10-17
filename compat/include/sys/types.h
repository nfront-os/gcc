/*
 * 9front-gcc: the <sys/types.h> types of POSIX APE lacks: clockid_t
 * (clock_gettime, compat/lib/time.c) and useconds_t (usleep).
 */
#include_next <sys/types.h>

#ifndef __9FRONT_GCC_SYS_TYPES
#define __9FRONT_GCC_SYS_TYPES
typedef int clockid_t;
typedef unsigned int useconds_t;
#endif
