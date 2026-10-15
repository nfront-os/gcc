/*
 * 9front-gcc: C99's floating-point classification, which APE's math.h
 * lacks (libstdc++'s <charconv>, ports), from gcc's builtins.
 */
#include_next <math.h>

#ifndef __9FRONT_GCC_MATH
#define __9FRONT_GCC_MATH

#ifndef FP_NAN
#define FP_NAN		0
#define FP_INFINITE	1
#define FP_ZERO		2
#define FP_SUBNORMAL	3
#define FP_NORMAL	4
#define fpclassify(x) \
	__builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x)
#endif
#ifndef isfinite
#define isfinite(x)	__builtin_isfinite(x)
#endif
#ifndef isnormal
#define isnormal(x)	__builtin_isnormal(x)
#endif
#ifndef signbit
#define signbit(x)	__builtin_signbit(x)
#endif

#endif
