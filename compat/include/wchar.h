/*
 * 9front-gcc: APE has no <wchar.h>. Only wint_t and its limits (mpfr's
 * printf needs them); none of the functions, and no mbstate_t, by which
 * configure scripts judge the multibyte functions (mbrtowc, ...) there.
 */
#ifndef __9FRONT_GCC_WCHAR
#define __9FRONT_GCC_WCHAR

#include <stddef.h>
#include <stdio.h>

#ifndef __WINT_TYPE__
#define __WINT_TYPE__ int
#endif
typedef __WINT_TYPE__ wint_t;

#ifndef WINT_MIN
#define WINT_MIN	__WINT_MIN__
#define WINT_MAX	__WINT_MAX__
#endif
#ifndef WEOF
#define WEOF		((wint_t)-1)
#endif

#endif
