/*
 * 9front-gcc: POSIX's <strings.h>, which APE lacks; the functions are
 * libbsd's (bsd.h declares them without const and with an int length;
 * the same ABI for lengths below 2^31).
 */
#ifndef __9FRONT_GCC_STRINGS_H
#define __9FRONT_GCC_STRINGS_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _BSD_EXTENSION
extern int	ffs(int);
extern int	strcasecmp(const char *, const char *);
extern int	strncasecmp(const char *, const char *, size_t);
#endif
#ifdef __cplusplus
}
#endif
#endif
