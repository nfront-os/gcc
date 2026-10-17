/*
 * 9front-gcc: POSIX's clocks and C11's timespec_get, which APE lacks
 * (libapecompat.a, compat/lib/time.c: /dev/bintime). APE has struct
 * timespec and nanosleep (with _POSIX_SOURCE, which the driver sets).
 */
#include_next <time.h>

#ifndef __9FRONT_GCC_TIME
#define __9FRONT_GCC_TIME
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_REALTIME			0
#define CLOCK_MONOTONIC			1
#define CLOCK_PROCESS_CPUTIME_ID	2
#define CLOCK_THREAD_CPUTIME_ID		3

#define TIME_UTC	1

extern int	clock_gettime(clockid_t, struct timespec *);
extern int	clock_getres(clockid_t, struct timespec *);
extern int	timespec_get(struct timespec *, int);

#ifdef __cplusplus
}
#endif
#endif
