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
 */
#ifndef __9FRONT_GCC_UNISTD
#define __9FRONT_GCC_UNISTD

#include <sys/types.h>
#define execv	__ape_execv
#define execve	__ape_execve
#define execvp	__ape_execvp
#include_next <unistd.h>
#undef execv
#undef execve
#undef execvp

#ifdef __cplusplus
extern "C" {
#endif
extern int	execv(const char *, char *const *);
extern int	execve(const char *, char *const *, char *const *);
extern int	execvp(const char *, char *const *);
extern int	truncate(const char *, off_t);
extern ssize_t	readlink(const char *, char *, size_t) __asm__("__apecompat_readlink");
#ifndef _BSD_EXTENSION
extern int	getopt(int, char *const *, const char *);
extern char	*optarg;
extern int	optind, opterr, optopt;
extern char	*mktemp(char *);
extern int	symlink(const char *, const char *);
#endif
#ifdef __cplusplus
}
#endif

#endif
