/* C99 <math.h> and <fenv.h> over APE (musl's libm in libapecompat.a):
 * float and long double functions, C99's additions, the type-generic
 * macros, rounding modes. Prints each failure, "c99math: n failed".
 * Build with -frounding-math (gcc's FENV_ACCESS: else it moves rint
 * across fesetround). */
#include <fenv.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static int failed;

static void
check(const char *what, int ok)
{
	if(!ok){
		printf("c99math: FAIL %s\n", what);
		failed++;
	}
}

/* |a-b| within n ulps of b */
static int
near(long double a, long double b, long double eps)
{
	return fabsl(a - b) <= eps * fabsl(b) || a == b;
}

int
main(void)
{
	volatile float f = 2.0f;
	volatile double d = 2.0;
	volatile long double l = 2.0L;
	volatile double nan0 = NAN, inf0 = INFINITY;
	volatile double d21 = 2.1, m21 = -2.1, m29 = -2.9;
	volatile float f21 = 2.1f;
	volatile long double l29 = 2.9L;
	volatile float r;
	long q;
	int e;

	/* through a float: on the 386 gcc inlines fsqrt, and the x87's
	 * result is wider than float until stored (excess precision) */
	r = sqrtf(f);
	check("sqrtf", r == 0x1.6a09e6p+0f);
	check("sqrtl", near(sqrtl(l), 1.41421356237309504880168872420969808L, LDBL_EPSILON));
	check("sinf", near(sinf(0.5f), 0.479425538604203f, FLT_EPSILON));
	check("expl", near(expl(1.0L), 2.71828182845904523536028747135266250L, 2 * LDBL_EPSILON));
	check("logl", near(logl(10.0L), 2.30258509299404568401799145468436421L, 2 * LDBL_EPSILON));
	check("powf", powf(f, 10.0f) == 1024.0f);
	check("cbrt", cbrt(27.0) == 3.0);
	check("hypotf", hypotf(3.0f, 4.0f) == 5.0f);
	check("expm1", near(expm1(1e-10), 1.00000000005e-10, 2 * DBL_EPSILON));
	check("log1p", near(log1p(1e-10), 9.9999999995e-11, 2 * DBL_EPSILON));
	check("tgamma", near(tgamma(5.0), 24.0, 4 * DBL_EPSILON));
	check("lgamma", near(lgamma(10.0), 12.8018274800814696112, 4 * DBL_EPSILON));
	check("erff", near(erff(0.5f), 0.520499877813047f, 2 * FLT_EPSILON));
	check("atanhl", near(atanhl(0.5L), 0.549306144334054845697622618461262852L, 2 * LDBL_EPSILON));

	check("round", round(2.5) == 3.0 && round(-2.5) == -3.0);
	check("trunc", trunc(-2.7) == -2.0 && truncf(2.7f) == 2.0f && truncl(-2.7L) == -2.0L);
	check("lround", lround(2.5) == 3 && llround(-2.5) == -3);
	check("nearbyint", nearbyint(2.5) == 2.0 && rint(3.5) == 4.0);
	check("fma", fma(0x1p-60, 0x1p-60, 1.0) == 1.0 && fma(1.0 + 0x1p-30, 1.0 - 0x1p-30, -1.0) == -0x1p-60);
	check("fmax", fmax(1.0, nan0) == 1.0 && fminf(-1.0f, 2.0f) == -1.0f);
	check("copysign", copysign(3.0, -0.0) == -3.0);
	check("nextafter", nextafter(1.0, 2.0) == 1.0 + DBL_EPSILON);
	check("remquo", remquo(10.0, 3.0, &e) == 1.0 && (e & 7) == 3);
	check("frexpl", frexpl(8.0L, &e) == 0.5L && e == 4);
	check("ilogb", ilogb(1024.0) == 10 && logb(0.25) == -2.0);
	check("scalbn", scalbnl(1.0L, 16000) > DBL_MAX);
	check("modff", modff(3.25f, (float *)&f) == 0.25f && f == 3.0f);
	check("nan", isnan(nan("")) && isnan(nanf("")) && isnan(nanl("")));
	check("fdim", fdim(5.0, 3.0) == 2.0 && fdim(3.0, 5.0) == 0.0);

	check("isnan", isnan(nan0) && isnan((float)nan0) && !isnan(d));
	check("isinf", isinf(inf0) && isinf((long double)inf0) && !isinf(d));
	check("fpclassify", fpclassify(0.0) == FP_ZERO && fpclassify(DBL_MIN / 2) == FP_SUBNORMAL &&
		fpclassify(d) == FP_NORMAL && fpclassify(inf0) == FP_INFINITE);
	check("signbit", signbit(-0.0) && !signbit(0.0f));
	check("isless", isless(1.0, 2.0) && !isless(nan0, 2.0) && isunordered(nan0, 1.0));

	/* rounding modes reach the x87 and SSE (volatile: no folding at
	 * compile time, and no rint(-x) as -rint(x), in the default mode) */
	check("fegetround", fegetround() == FE_TONEAREST);
	check("FE_UPWARD", fesetround(FE_UPWARD) == 0 && rint(d21) == 3.0 && lrintf(f21) == 3);
	check("FE_DOWNWARD", fesetround(FE_DOWNWARD) == 0 && rint(m21) == -3.0 && rintl(l29) == 2.0L);
	check("FE_TOWARDZERO", fesetround(FE_TOWARDZERO) == 0 && nearbyint(m29) == -2.0);
	fesetround(FE_TONEAREST);
	feclearexcept(FE_ALL_EXCEPT);
	d = d / 0.0;
	check("FE_DIVBYZERO", fetestexcept(FE_DIVBYZERO) != 0);
	feclearexcept(FE_ALL_EXCEPT);
	check("feclearexcept", fetestexcept(FE_ALL_EXCEPT) == 0);

	q = lrint(1e6);
	check("lrint", q == 1000000);
	check("M_PI", M_PI > 3.14159 && M_PI < 3.1416);

	printf("c99math: %d failed\n", failed);
	return failed != 0;
}
