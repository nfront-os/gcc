/*
 * 9front-gcc: the <stdint.h> macros of C99 APE's lacks: the constant
 * macros (INT64_C, ...) and the limits of intptr_t, ptrdiff_t,
 * sig_atomic_t, wchar_t and wint_t. For APE's types: 64 bit are long
 * long, intptr_t long long on amd64 and int on 386; ptrdiff_t and
 * wchar_t are the compiler's (stddef.h).
 *
 * intmax_t and uintmax_t are the compiler's (__INTMAX_TYPE__: long on
 * amd64, APE's long long the same size), as printf's %jd and C++'s
 * std::intmax_t take them; APE's typedefs are renamed while its header
 * is read.
 */
#define intmax_t	__ape_intmax_t
#define uintmax_t	__ape_uintmax_t
#include_next <stdint.h>
#undef intmax_t
#undef uintmax_t

#ifndef __9FRONT_GCC_STDINT
#define __9FRONT_GCC_STDINT
typedef __INTMAX_TYPE__ intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;
#undef INTMAX_MIN
#undef INTMAX_MAX
#undef UINTMAX_MIN
#undef UINTMAX_MAX
#define INTMAX_MIN	(-__INTMAX_MAX__-1)
#define INTMAX_MAX	__INTMAX_MAX__
#define UINTMAX_MIN	0
#define UINTMAX_MAX	__UINTMAX_MAX__

#ifndef INT8_C
#define INT8_C(c)	c
#define INT16_C(c)	c
#define INT32_C(c)	c
#define INT64_C(c)	c ## LL
#define UINT8_C(c)	c
#define UINT16_C(c)	c
#define UINT32_C(c)	c ## U
#define UINT64_C(c)	c ## ULL
#define INTMAX_C(c)	__INTMAX_C(c)
#define UINTMAX_C(c)	__UINTMAX_C(c)
#endif

#ifndef INTPTR_MAX
#if __SIZEOF_POINTER__ == 8
#define INTPTR_MIN	INT64_MIN
#define INTPTR_MAX	INT64_MAX
#define UINTPTR_MAX	UINT64_MAX
#else
#define INTPTR_MIN	INT32_MIN
#define INTPTR_MAX	INT32_MAX
#define UINTPTR_MAX	UINT32_MAX
#endif
#endif
#ifndef PTRDIFF_MAX
#define PTRDIFF_MIN	(-__PTRDIFF_MAX__-1)
#define PTRDIFF_MAX	__PTRDIFF_MAX__
#endif
#ifndef SIG_ATOMIC_MAX
#define SIG_ATOMIC_MIN	INT32_MIN
#define SIG_ATOMIC_MAX	INT32_MAX
#endif
#ifndef WCHAR_MAX
#define WCHAR_MIN	__WCHAR_MIN__
#define WCHAR_MAX	__WCHAR_MAX__
#endif
#ifndef WINT_MAX
#define WINT_MIN	__WINT_MIN__
#define WINT_MAX	__WINT_MAX__
#endif

#endif
