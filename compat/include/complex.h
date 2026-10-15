/*
 * 9front-gcc: C99's <complex.h>, which APE lacks (mpc), as far as gcc's
 * builtins carry it: the type macros and the component functions. The
 * library functions (csin, cexp, ...) are not here; libap has none.
 */
#ifndef __9FRONT_GCC_COMPLEX
#define __9FRONT_GCC_COMPLEX

#define complex		_Complex
#define _Complex_I	(__extension__ 1.0iF)
#define I		_Complex_I

#define creal(z)	__builtin_creal(z)
#define crealf(z)	__builtin_crealf(z)
#define creall(z)	__builtin_creall(z)
#define cimag(z)	__builtin_cimag(z)
#define cimagf(z)	__builtin_cimagf(z)
#define cimagl(z)	__builtin_cimagl(z)
#define conj(z)		__builtin_conj(z)
#define conjf(z)	__builtin_conjf(z)
#define conjl(z)	__builtin_conjl(z)

#define CMPLX(x, y)	__builtin_complex((double)(x), (double)(y))
#define CMPLXF(x, y)	__builtin_complex((float)(x), (float)(y))
#define CMPLXL(x, y)	__builtin_complex((long double)(x), (long double)(y))

#endif
