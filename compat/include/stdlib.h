/*
 * 9front-gcc: realpath (POSIX), which APE lacks (libapecompat.a).
 */
#include_next <stdlib.h>

#ifndef __9FRONT_GCC_STDLIB
#define __9FRONT_GCC_STDLIB
#ifdef __cplusplus
extern "C" {
#endif
extern char	*realpath(const char *, char *);
#ifdef __cplusplus
}
#endif
#endif
