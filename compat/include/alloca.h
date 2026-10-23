/*
 * 9front-gcc: <alloca.h>, as glibc and musl have it: gcc's builtin.
 */
#ifndef __9FRONT_GCC_ALLOCA_H
#define __9FRONT_GCC_ALLOCA_H
#include <stddef.h>
#undef alloca
#define alloca(n)	__builtin_alloca(n)
#endif
