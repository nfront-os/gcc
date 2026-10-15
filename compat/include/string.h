/*
 * 9front-gcc: <string.h> as POSIX.1-2008 and the usual C libraries have
 * it, over APE's: stpcpy, stpncpy and strndup (libapecompat.a; APE
 * declares strndup, without const, but has no implementation; its
 * declaration is renamed while APE's header is read) and strcasecmp &
 * co. from <strings.h>, which glibc, musl and the BSDs declare here too.
 */
#ifndef __9FRONT_GCC_STRING
#define __9FRONT_GCC_STRING
#define strndup	__ape_strndup
#include_next <string.h>
#undef strndup
#include <strings.h>
#ifdef __cplusplus
extern "C" {
#endif
extern char	*stpcpy(char *, const char *);
extern char	*stpncpy(char *, const char *, size_t);
extern char	*strndup(const char *, size_t);
#ifdef __cplusplus
}
#endif
#endif
