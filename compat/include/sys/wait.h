/*
 * 9front-gcc: wait3 and wait4 (libap has them) with a complete struct
 * rusage, which APE declares only with _BSD_EXTENSION.
 */
#ifndef __9FRONT_GCC_SYS_WAIT_H
#define __9FRONT_GCC_SYS_WAIT_H
#include_next <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#ifndef _BSD_EXTENSION
#ifdef __cplusplus
extern "C" {
#endif
pid_t	wait3(int *, int, struct rusage *);
pid_t	wait4(pid_t, int *, int, struct rusage *);
#ifdef __cplusplus
}
#endif
#endif
#endif
