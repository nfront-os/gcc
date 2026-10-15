/*
 * 9front-gcc: APE declares signal handlers K&R-style, "void (*)()",
 * deliberately without parameter types (some take the note and a Ureg
 * too). C++ and C23 read "()" as (void): no handler fits. For them the
 * POSIX declarations, void (*)(int), of the same libap functions; APE's
 * are renamed while its header is read (struct sigaction keeps its
 * layout). C up to C17 gets APE's header as it is.
 */
#if defined __cplusplus || (defined __STDC_VERSION__ && __STDC_VERSION__ > 201710L)

#ifndef __9FRONT_GCC_SIGNAL_H
#define __9FRONT_GCC_SIGNAL_H
#define signal		__ape_signal
#define sigaction	__ape_sigaction
#include_next <signal.h>
#undef signal
#undef sigaction

#undef SIG_DFL
#undef SIG_ERR
#undef SIG_IGN
#define SIG_DFL	((void (*)(int))0)
#define SIG_ERR	((void (*)(int))-1)
#define SIG_IGN	((void (*)(int))1)

#ifdef __cplusplus
extern "C" {
#endif
extern void	(*signal(int, void (*)(int)))(int);
#ifdef _POSIX_SOURCE
struct sigaction {
	void		(*sa_handler)(int);
	sigset_t	sa_mask;
	int		sa_flags;
};
/* the same layout as APE's (a negative array size otherwise) */
typedef char __9front_gcc_sigaction_layout[
	sizeof(struct sigaction) == sizeof(struct __ape_sigaction) ? 1 : -1];
extern int	sigaction(int, const struct sigaction *, struct sigaction *);
#endif
#ifdef __cplusplus
}
#endif
#endif

#else
#include_next <signal.h>
#endif
