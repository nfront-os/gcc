/*
 * 9front-gcc: POSIX.1-2024's asprintf and vasprintf, which APE lacks
 * (libapecompat.a, compat/lib); popen and pclose, which APE declares only
 * with _BSD_EXTENSION (libbsd; the driver links both).
 */
#include_next <stdio.h>

#ifndef __9FRONT_GCC_STDIO
#define __9FRONT_GCC_STDIO
#ifdef __cplusplus
extern "C" {
#endif
extern int	asprintf(char **, const char *, ...)
		__attribute__((__format__(__printf__, 2, 3)));
extern int	vasprintf(char **, const char *, va_list)
		__attribute__((__format__(__printf__, 2, 0)));
#ifndef _BSD_EXTENSION
extern FILE	*popen(const char *, const char *);
extern int	pclose(FILE *);
#endif
#ifdef __cplusplus
}
#endif
#endif
