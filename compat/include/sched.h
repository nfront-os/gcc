/*
 * 9front-gcc: <sched.h>, which APE lacks: sched_yield (libapecompat.a,
 * compat/lib/time.c).
 */
#ifndef __9FRONT_GCC_SCHED
#define __9FRONT_GCC_SCHED
#ifdef __cplusplus
extern "C" {
#endif
extern int	sched_yield(void);
#ifdef __cplusplus
}
#endif
#endif
