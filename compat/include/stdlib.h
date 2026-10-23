/*
 * 9front-gcc: what C99, C11 and POSIX add to APE's <stdlib.h>
 * (libapecompat.a): strtof and strtold (musl's, compat/lib/stdio.c),
 * llabs, lldiv, _Exit, quick_exit and at_quick_exit, realpath.
 *
 * mblen, mbtowc, wctomb, mbstowcs and wcstombs for gcc's 32-bit wchar_t
 * (compat/lib/multibyte.c; APE's store 16 bits): APE's declarations are
 * renamed while its header is read, ours carry the link names
 * __apecompat_* (only those: names libap does not have keep their own,
 * as configure's link tests look for them).
 */
#define mblen		__ape_mblen
#define mbtowc		__ape_mbtowc
#define wctomb		__ape_wctomb
#define mbstowcs	__ape_mbstowcs
#define wcstombs	__ape_wcstombs
#include_next <stdlib.h>
#undef mblen
#undef mbtowc
#undef wctomb
#undef mbstowcs
#undef wcstombs

#ifndef __9FRONT_GCC_STDLIB
#define __9FRONT_GCC_STDLIB
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { long long quot, rem; } lldiv_t;

extern float	strtof(const char *, char **);
extern long double	strtold(const char *, char **);
extern long long	llabs(long long);
extern lldiv_t	lldiv(long long, long long);
extern _Noreturn void	_Exit(int);
extern int	at_quick_exit(void (*)(void));
extern _Noreturn void	quick_exit(int);
extern char	*realpath(const char *, char *);

extern int	mblen(const char *, size_t)
		__asm__("__apecompat_mblen");
extern int	mbtowc(wchar_t *, const char *, size_t)
		__asm__("__apecompat_mbtowc");
extern int	wctomb(char *, wchar_t)
		__asm__("__apecompat_wctomb");
extern size_t	mbstowcs(wchar_t *, const char *, size_t)
		__asm__("__apecompat_mbstowcs");
extern size_t	wcstombs(char *, const wchar_t *, size_t)
		__asm__("__apecompat_wcstombs");
#ifdef __cplusplus
}
#endif
/* alloca, which glibc's <stdlib.h> gives with _GNU_SOURCE or _DEFAULT_SOURCE */
#if (defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE)) && !defined(alloca)
#define alloca(n)	__builtin_alloca(n)
#endif
#endif
