/* C99's printf and scanf (musl's engine over APE's FILE), strtof and
 * strtold, llabs, lldiv, the multibyte functions with gcc's 32-bit
 * wchar_t, errno in APE's numbers. Prints each failure and
 * "c99stdio: n failed"; "malloc: align" the smallest alignment of
 * malloc's blocks seen. */
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static int failed;

static void
check(const char *what, int ok)
{
	if(!ok){
		printf("c99stdio: FAIL %s\n", what);
		failed++;
	}
}

static int
fmt(const char *want, const char *f, ...)
{
	char buf[256];
	va_list arg;
	int n;

	va_start(arg, f);
	n = vsnprintf(buf, sizeof buf, f, arg);
	va_end(arg);
	if(n != (int)strlen(want) || strcmp(buf, want) != 0){
		printf("c99stdio: \"%s\" gave \"%s\" (%d), want \"%s\"\n", f, buf, n, want);
		return 0;
	}
	return 1;
}

int
main(void)
{
	char buf[64], *end;
	long double ld;
	float fl;
	signed char hh;
	intmax_t jm;
	size_t zu;
	ptrdiff_t td;
	wchar_t wc, ws[8];
	mbstate_t st;
	FILE *f;
	unsigned long align;
	int i, n, c;
	void *p[64];

#ifdef __linux__
	/* checking the test on the host: glibc's "C" is not UTF-8 */
	setlocale(LC_CTYPE, "C.UTF-8");
#endif
	/* printf: C99's conversions and length modifiers */
	check("%Lf", fmt("3.141592653589793239", "%.18Lf", 3.141592653589793238462643383279L));
	check("%Le", fmt("1.000000e+4000", "%Le", 1e4000L));
	check("%a", fmt("0x1.8p+1", "%a", 3.0));
	check("%F", fmt("INF NAN", "%F %F", (double)INFINITY, (double)NAN));
	check("%hhd", fmt("-1", "%hhd", 255));
	check("%jd", fmt("-9223372036854775807", "%jd", (intmax_t)-INT64_MAX));
	check("%zu", fmt("4294967295", "%zu", (size_t)UINT32_MAX));
	check("%td", fmt("-5", "%td", (ptrdiff_t)-5));
	check("%" PRIu64, fmt("18446744073709551615", "%" PRIu64, UINT64_MAX));
	check("%g", fmt("1e+100 0.0001", "%g %g", 1e100, 1e-4));
	check("%.0f", fmt("2 4", "%.0f %.0f", 2.5, 3.5));
	check("%5.2s|%-4d|", fmt("   ab|7   |", "%5.2s|%-4d|", "abc", 7));
	check("%n$", fmt("b a", "%2$s %1$s", "a", "b"));
	check("%ls", fmt("h\xc3\xa9\xf0\x9f\x98\x80", "%ls", L"hé\U0001F600"));
	check("%lc", fmt("\xe2\x82\xac", "%lc", (wint_t)0x20AC));
	n = snprintf(buf, 4, "%d", 123456);
	check("snprintf C99 result", n == 6 && strcmp(buf, "123") == 0);
	check("snprintf(NULL, 0)", snprintf(NULL, 0, "%s-%d", "xyz", 42) == 6);

	/* scanf */
	n = sscanf("3.25e1 -128 77 0x1fP-2 abc", "%Lf %hhd %jd %f %3c", &ld, &hh, &jm, &fl, buf);
	check("sscanf", n == 5 && ld == 32.5L && hh == -128 && jm == 77 && fl == 7.75f &&
		memcmp(buf, "abc", 3) == 0);
	n = sscanf("12 34", "%zu %td", &zu, &td);
	check("sscanf %zu %td", n == 2 && zu == 12 && td == 34);
	n = sscanf("x", "%d", &i);
	check("sscanf no match", n == 0);
	check("sscanf EOF", sscanf("", "%d", &i) == EOF);

	/* fscanf on an APE FILE: what it does not consume is still there */
	f = tmpfile();
	check("tmpfile", f != NULL);
	if(f != NULL){
		fprintf(f, "%d %s;rest\n", 42, "word");
		rewind(f);
		n = fscanf(f, "%d %4s", &i, buf);
		c = getc(f);
		check("fscanf", n == 2 && i == 42 && strcmp(buf, "word") == 0 && c == ';');
		fgets(buf, sizeof buf, f);
		check("fscanf leaves the rest", strcmp(buf, "rest\n") == 0);
		rewind(f);
		n = fscanf(f, "%d", &i);
		c = getc(f);
		check("fscanf pushback", n == 1 && c == ' ');
		fclose(f);
	}

	/* strtof, strtold, errno in APE's numbers */
	fl = strtof("0.1", &end);
	check("strtof", fl == 0.1f && *end == 0);
	ld = strtold("0x1.8p1L", &end);
	check("strtold hex", ld == 3.0L && strcmp(end, "L") == 0);
	ld = strtold("1.18973149535723176502e+4932", NULL);
	check("strtold LDBL_MAX", ld == LDBL_MAX);
	errno = 0;
	ld = strtold("1e99999", NULL);
	check("strtold ERANGE", ld == HUGE_VALL && errno == ERANGE);

	check("llabs", llabs(-5000000000LL) == 5000000000LL);
	check("lldiv", lldiv(-7, 2).quot == -3 && lldiv(-7, 2).rem == -1);

	/* multibyte: UTF-8, 32-bit wchar_t */
	check("sizeof(wchar_t)", sizeof(wchar_t) == 4 && sizeof L"x"[0] == 4);
	memset(&st, 0, sizeof st);
	check("mbrtowc 4 bytes", mbrtowc(&wc, "\xf0\x9f\x98\x80", 4, &st) == 4 && wc == 0x1F600);
	check("mbrtowc partial", mbrtowc(&wc, "\xe2\x82", 2, &st) == (size_t)-2 &&
		!mbsinit(&st) && mbrtowc(&wc, "\xac", 1, &st) == 1 && wc == 0x20AC && mbsinit(&st));
	errno = 0;
	check("mbrtowc overlong", mbrtowc(&wc, "\xc0\x80", 2, &st) == (size_t)-1 && errno == EILSEQ);
	check("mbrtowc surrogate", mbrtowc(&wc, "\xed\xa0\x80", 3, &st) == (size_t)-1);
	check("wcrtomb", wcrtomb(buf, 0x10FFFF, &st) == 4 && memcmp(buf, "\xf4\x8f\xbf\xbf", 4) == 0);
	check("wcrtomb > 0x10FFFF", wcrtomb(buf, 0x110000, &st) == (size_t)-1);
	check("mbstowcs", mbstowcs(ws, "a\xc3\xa9z", 8) == 3 && ws[1] == 0xE9 && ws[3] == 0);
	check("wcstombs", wcstombs(buf, L"é!", sizeof buf) == 3 && strcmp(buf, "\xc3\xa9!") == 0);
	check("mbtowc", mbtowc(&wc, "\xe2\x82\xac", 3) == 3 && wc == 0x20AC);
	check("wctomb", wctomb(buf, 0x1F600) == 4);

	/* malloc's alignment, for C++'s new (16 on amd64) */
	align = 4096;
	for(i = 0; i < 64; i++){
		p[i] = malloc(1 + i * 7);
		while((uintptr_t)p[i] & (align - 1))
			align >>= 1;
	}
	for(i = 0; i < 64; i++)
		free(p[i]);
	printf("malloc: align %lu\n", align);

	printf("c99stdio: %d failed\n", failed);
	return failed != 0;
}
