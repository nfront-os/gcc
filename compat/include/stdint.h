/*
 * 9front-gcc: the <stdint.h> macros of C99 APE's lacks: the constant
 * macros (INT64_C, ...) and the limits of intptr_t, ptrdiff_t,
 * sig_atomic_t, wchar_t and wint_t. For APE's types: 64 bit and
 * intmax_t are long long, intptr_t long long on amd64 and int on 386;
 * ptrdiff_t is the compiler's (stddef.h); wchar_t (C) unsigned short.
 */
#include_next <stdint.h>

#ifndef __9FRONT_GCC_STDINT
#define __9FRONT_GCC_STDINT

#ifndef INT8_C
#define INT8_C(c)	c
#define INT16_C(c)	c
#define INT32_C(c)	c
#define INT64_C(c)	c ## LL
#define UINT8_C(c)	c
#define UINT16_C(c)	c
#define UINT32_C(c)	c ## U
#define UINT64_C(c)	c ## ULL
#define INTMAX_C(c)	c ## LL
#define UINTMAX_C(c)	c ## ULL
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
#ifdef __cplusplus
#define WCHAR_MIN	__WCHAR_MIN__
#define WCHAR_MAX	__WCHAR_MAX__
#else
#define WCHAR_MIN	0
#define WCHAR_MAX	0xffff
#endif
#endif
#ifndef WINT_MAX
#define WINT_MIN	__WINT_MIN__
#define WINT_MAX	__WINT_MAX__
#endif

#endif
