/*
 * 9front-gcc: <fcntl.h> over APE's: POSIX.1-2008's openat with
 * AT_FDCWD (compat/lib/at.c).
 */
#ifndef __9FRONT_GCC_FCNTL_H
#define __9FRONT_GCC_FCNTL_H
#include_next <fcntl.h>
#define AT_FDCWD	(-100)
#ifdef __cplusplus
extern "C" {
#endif
extern int	openat(int, const char *, int, ...);
#ifdef __cplusplus
}
#endif
#endif
