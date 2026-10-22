/*
 * 9front-gcc: <unistd.h> as POSIX has it, over APE's:
 * - execv, execve, execvp take char *const[] (APE: const char **, which
 *   gcc 14 and later reject for POSIX callers): APE's declarations are
 *   renamed while its header is read, the POSIX ones bind to the same
 *   libap functions (same ABI: pointers).
 * - getopt and its variables, mktemp: POSIX (SUSv2) puts them here, APE
 *   only in bsd.h (_BSD_EXTENSION); libbsd has them, the driver links it.
 * - truncate (libapecompat.a; APE has only ftruncate).
 * - readlink, symlink (POSIX; APE: bsd.h). libbsd's readlink returns
 *   int, a POSIX caller reads an ssize_t (-1 would arrive as 2^32-1): the
 *   declaration binds to libapecompat's own (no symlinks on 9front).
 *   Not with an APE that declares them as POSIX has them (its unistd.h
 *   defines __APE_POSIX_LINKS; libbsd's are then POSIX's).
 * - pread, pwrite, usleep (libapecompat.a).
 * - the POSIX options compat has (_POSIX_TIMERS, _POSIX_THREADS, ...).
 * - sysconf with the number of processors, the page size and the pages
 *   of memory
 *   (libapecompat.a: link name __apecompat_sysconf, it asks APE's for
 *   the rest).
 */
#ifndef __9FRONT_GCC_UNISTD
#define __9FRONT_GCC_UNISTD

#include <sys/types.h>
#define execv	__ape_execv
#define execve	__ape_execve
#define execvp	__ape_execvp
#define sysconf	__ape_sysconf
#include_next <unistd.h>
#undef execv
#undef execve
#undef execvp
#undef sysconf

/* what compat has of POSIX's options (time.c, pthread.c); not
 * _POSIX_THREAD_SAFE_FUNCTIONS: APE's stdio has no locks */
#define _POSIX_TIMERS			200809L
#define _POSIX_MONOTONIC_CLOCK		200809L
#define _POSIX_CLOCK_SELECTION		200809L
#define _POSIX_THREADS			200809L
#define _POSIX_TIMEOUTS			200809L
#define _POSIX_READER_WRITER_LOCKS	200809L

#define _SC_PAGESIZE		30
#define _SC_PAGE_SIZE		_SC_PAGESIZE
#define _SC_NPROCESSORS_CONF	83
#define _SC_NPROCESSORS_ONLN	84
#define _SC_PHYS_PAGES		85

#ifdef __cplusplus
extern "C" {
#endif
extern int	execv(const char *, char *const *);
extern int	execve(const char *, char *const *, char *const *);
extern int	execvp(const char *, char *const *);
extern int	truncate(const char *, off_t);
#ifndef __APE_POSIX_LINKS
extern ssize_t	readlink(const char *, char *, size_t) __asm__("__apecompat_readlink");
#endif
extern ssize_t	pread(int, void *, size_t, off_t);
extern ssize_t	pwrite(int, const void *, size_t, off_t);
extern int	usleep(useconds_t);
extern long	sysconf(int) __asm__("__apecompat_sysconf");
#ifndef _BSD_EXTENSION
extern int	getopt(int, char *const *, const char *);
extern char	*optarg;
extern int	optind, opterr, optopt;
extern char	*mktemp(char *);
#ifndef __APE_POSIX_LINKS
extern int	symlink(const char *, const char *);
#endif
#endif
#ifdef __cplusplus
}
#endif

#endif
