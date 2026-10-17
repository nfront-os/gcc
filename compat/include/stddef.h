/*
 * 9front-gcc: <stddef.h> over APE's:
 * - ptrdiff_t is the compiler's type (long; APE: long long, the same
 *   size), as libstdc++'s std::ptrdiff_t is: else the two are ambiguous.
 *   APE's typedef is renamed while its header is read.
 * - C11's max_align_t, which APE lacks, as gcc's own stddef.h has it.
 * - offsetof as gcc's builtin (APE's null-pointer macro is no constant
 *   expression in C++).
 * - wchar_t is the compiler's (C: int, 32 bits, the type of L"..."; C++:
 *   the keyword), not APE's unsigned short: _WCHAR_T keeps APE's typedef
 *   out. APE's functions with wchar_t (mbtowc & co.) store 16 bits;
 *   compat <stdlib.h> gives their names to 32-bit ones.
 */
#ifndef __9FRONT_GCC_STDDEF
#define __9FRONT_GCC_STDDEF
#ifndef _WCHAR_T
#define _WCHAR_T
#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif
#endif
#define ptrdiff_t	__ape_ptrdiff_t
#include_next <stddef.h>
#undef ptrdiff_t
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#undef offsetof
#define offsetof(t, m)	__builtin_offsetof(t, m)

#if (defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L) \
	|| (defined __cplusplus && __cplusplus >= 201103L)
typedef struct {
	long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
	long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;
#endif

#endif
