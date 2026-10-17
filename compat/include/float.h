/*
 * 9front-gcc: <float.h> with the compiler's values. APE's (/$objtype/
 * include/ape/float.h) describes kencc's types: its long double is a
 * double, under gcc it is the x87's 80-bit type, and LDBL_MAX was a
 * double constant (printf's %Lf read garbage). C99's and C11's macros
 * APE lacks besides. APE's header stays for the rest (FPdbleword, the
 * FP* bits of _PLAN9_SOURCE); its limits are replaced, as gcc's own
 * float.h defines them.
 */
#include_next <float.h>

#ifndef __9FRONT_GCC_FLOAT
#define __9FRONT_GCC_FLOAT

#undef FLT_RADIX
#undef FLT_ROUNDS
#undef FLT_DIG
#undef FLT_EPSILON
#undef FLT_MANT_DIG
#undef FLT_MAX
#undef FLT_MAX_10_EXP
#undef FLT_MAX_EXP
#undef FLT_MIN
#undef FLT_MIN_10_EXP
#undef FLT_MIN_EXP
#undef DBL_DIG
#undef DBL_EPSILON
#undef DBL_MANT_DIG
#undef DBL_MAX
#undef DBL_MAX_10_EXP
#undef DBL_MAX_EXP
#undef DBL_MIN
#undef DBL_MIN_10_EXP
#undef DBL_MIN_EXP
#undef LDBL_DIG
#undef LDBL_EPSILON
#undef LDBL_MANT_DIG
#undef LDBL_MAX
#undef LDBL_MAX_10_EXP
#undef LDBL_MAX_EXP
#undef LDBL_MIN
#undef LDBL_MIN_10_EXP
#undef LDBL_MIN_EXP

#define FLT_RADIX	__FLT_RADIX__
/* the current rounding mode (fesetround) */
#define FLT_ROUNDS	(__builtin_flt_rounds())
#define FLT_EVAL_METHOD	__FLT_EVAL_METHOD__
#define DECIMAL_DIG	__DECIMAL_DIG__

#define FLT_MANT_DIG	__FLT_MANT_DIG__
#define FLT_DIG		__FLT_DIG__
#define FLT_EPSILON	__FLT_EPSILON__
#define FLT_MIN		__FLT_MIN__
#define FLT_MAX		__FLT_MAX__
#define FLT_MIN_EXP	__FLT_MIN_EXP__
#define FLT_MAX_EXP	__FLT_MAX_EXP__
#define FLT_MIN_10_EXP	__FLT_MIN_10_EXP__
#define FLT_MAX_10_EXP	__FLT_MAX_10_EXP__

#define DBL_MANT_DIG	__DBL_MANT_DIG__
#define DBL_DIG		__DBL_DIG__
#define DBL_EPSILON	__DBL_EPSILON__
#define DBL_MIN		__DBL_MIN__
#define DBL_MAX		__DBL_MAX__
#define DBL_MIN_EXP	__DBL_MIN_EXP__
#define DBL_MAX_EXP	__DBL_MAX_EXP__
#define DBL_MIN_10_EXP	__DBL_MIN_10_EXP__
#define DBL_MAX_10_EXP	__DBL_MAX_10_EXP__

#define LDBL_MANT_DIG	__LDBL_MANT_DIG__
#define LDBL_DIG	__LDBL_DIG__
#define LDBL_EPSILON	__LDBL_EPSILON__
#define LDBL_MIN	__LDBL_MIN__
#define LDBL_MAX	__LDBL_MAX__
#define LDBL_MIN_EXP	__LDBL_MIN_EXP__
#define LDBL_MAX_EXP	__LDBL_MAX_EXP__
#define LDBL_MIN_10_EXP	__LDBL_MIN_10_EXP__
#define LDBL_MAX_10_EXP	__LDBL_MAX_10_EXP__

/* C11 */
#if (defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L) \
	|| (defined __cplusplus && __cplusplus >= 201103L)
#define FLT_DECIMAL_DIG		__FLT_DECIMAL_DIG__
#define DBL_DECIMAL_DIG		__DBL_DECIMAL_DIG__
#define LDBL_DECIMAL_DIG	__LDBL_DECIMAL_DIG__
#define FLT_HAS_SUBNORM		__FLT_HAS_DENORM__
#define DBL_HAS_SUBNORM		__DBL_HAS_DENORM__
#define LDBL_HAS_SUBNORM	__LDBL_HAS_DENORM__
#define FLT_TRUE_MIN		__FLT_DENORM_MIN__
#define DBL_TRUE_MIN		__DBL_DENORM_MIN__
#define LDBL_TRUE_MIN		__LDBL_DENORM_MIN__
#endif

#endif
