/*
 * 9front-gcc: the errno values of C99 and POSIX.1-2008 APE lacks
 * (libstdc++'s std::errc, ports). APE's own errno.h first; libap never
 * sets these, strerror calls them "Unknown error". The numbers are
 * outside APE's ranges (1-62, 1000-1002).
 */
#include_next <errno.h>

#ifndef __9FRONT_GCC_ERRNO
#define __9FRONT_GCC_ERRNO

#ifndef EILSEQ
#define EILSEQ		1100
#endif

#ifdef _POSIX_SOURCE
#ifndef EALREADY
#define EALREADY	1101
#endif
#ifndef ECONNRESET
#define ECONNRESET	1102
#endif
#ifndef ELOOP
#define ELOOP		1103
#endif
#ifndef ENOMSG
#define ENOMSG		1104
#endif
#ifndef ENOTSUP
#define ENOTSUP		EOPNOTSUPP
#endif
#ifndef EOVERFLOW
#define EOVERFLOW	1105
#endif
#endif /* _POSIX_SOURCE */

#endif
