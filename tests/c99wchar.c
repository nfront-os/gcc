/* C99's <wchar.h> and <wctype.h> over APE (ap/gnu): wide I/O
 * on APE's FILE in UTF-8, the wide printf and scanf, wcsto*, the
 * string functions, Unicode classification and case mapping. Prints
 * each failure and "c99wchar: n failed". */
#ifdef __linux__
#define _XOPEN_SOURCE 700	/* the host: wcwidth */
#endif
#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

static int failed;

static void
check(const char *what, int ok)
{
	if(!ok){
		printf("c99wchar: FAIL %s\n", what);
		failed++;
	}
}

int
main(void)
{
	wchar_t ws[64], *end;
	char buf[64];
	FILE *f;
	struct tm tm;
	long double ld;
	double d;
	int i, n;
	wint_t c;

#ifdef __linux__
	/* checking the test on the host: glibc's "C" is not UTF-8 */
	setlocale(LC_CTYPE, "C.UTF-8");
#endif
	/* string functions */
	check("wcslen", wcslen(L"héllo") == 5);
	check("wcscmp", wcscmp(L"abc", L"abd") < 0 && wcsncmp(L"abc", L"abd", 2) == 0);
	check("wcschr", wcschr(L"a€b", 0x20AC) != NULL && wcsrchr(L"abca", L'a') - L"abca" == 3);
	check("wcsstr", wcsstr(L"Grüße Welt", L"Welt") != NULL);
	wcscpy(ws, L"ab");
	wcscat(ws, L"çd");
	check("wcscpy wcscat", wcscmp(ws, L"abçd") == 0);
	check("wmemset wmemcmp", wmemcmp(wmemset(ws, L'x', 3), L"xxx", 3) == 0);
	check("wcscoll", wcscoll(L"a", L"b") < 0);
	check("wcsxfrm", wcsxfrm(ws, L"abc", 64) == 3 && wcscmp(ws, L"abc") == 0);

	/* <wctype.h>: Unicode, not only ASCII */
	check("iswalpha", iswalpha(0xE9) && iswalpha(0x3B1) && !iswalpha(L'1'));
	check("iswdigit", iswdigit(L'7') && !iswdigit(0x0663));
	check("iswspace", iswspace(L' ') && iswspace(0x2003) && !iswspace(L'x'));
	check("towupper", towupper(0xE9) == 0xC9 && towupper(0x3B1) == 0x391 && towlower(L'Q') == L'q');
	check("wctype", iswctype(0xE9, wctype("alpha")) && !iswctype(L'a', wctype("digit")));
	check("wcwidth", wcwidth(L'a') == 1 && wcwidth(0x4E2D) == 2 && wcwidth(0x301) == 0);

	/* wcsto* */
	check("wcstol", wcstol(L"  -1234xyz", &end, 10) == -1234 && *end == L'x');
	check("wcstoull", wcstoull(L"0xffffffffffffffff", NULL, 16) == 0xffffffffffffffffULL);
	d = wcstod(L"2.5e3", &end);
	check("wcstod", d == 2500.0 && *end == 0);
	ld = wcstold(L"0x1p-16000", NULL);
	check("wcstold", ld > 0 && ld < 1e-4000L);
	check("wcstoimax", wcstoimax(L"-42", NULL, 10) == -42);

	/* swprintf, swscanf */
	n = swprintf(ws, 64, L"%ls|%d|%.2f|%lc|%s", L"éè", 42, 3.14159, (wint_t)0x20AC, "\xc3\xa0");
	check("swprintf", n == 14 && wcscmp(ws, L"éè|42|3.14|€|à") == 0);
	check("swprintf overflow", swprintf(ws, 4, L"%d", 123456) < 0);
	n = swscanf(L"17 2.5 héllo w", L"%d %lf %ls %lc", &i, &d, ws, (wchar_t *)buf);
	check("swscanf", n == 4 && i == 17 && d == 2.5 && wcscmp(ws, L"héllo") == 0 &&
		*(wchar_t *)buf == L'w');

	/* wide I/O on an APE FILE: UTF-8 on the bytes */
	f = tmpfile();
	check("tmpfile", f != NULL);
	if(f != NULL){
		check("fwide", fwide(f, 1) > 0);
		n = fwprintf(f, L"%ls %d\n", L"grüß", 7);
		check("fwprintf", n == 7);
		check("fputwc", fputwc(0x1F600, f) == 0x1F600);
		check("fputws", fputws(L"αβ\n", f) >= 0);
		rewind(f);
		check("fgetwc", fgetwc(f) == L'g' && fgetwc(f) == L'r' && fgetwc(f) == 0xFC);
		check("ungetwc", ungetwc(0xFC, f) == 0xFC && fgetwc(f) == 0xFC);
		n = fwscanf(f, L"%lc %d", (wchar_t *)buf, &i);
		check("fwscanf", n == 2 && *(wchar_t *)buf == 0xDF && i == 7);
		check("after fwscanf", fgetwc(f) == L'\n' && fgetwc(f) == 0x1F600);
		check("fgetws", fgetws(ws, 64, f) != NULL && wcscmp(ws, L"αβ\n") == 0);
		check("WEOF", fgetwc(f) == WEOF && feof(f));
		rewind(f);
		check("bytes are UTF-8", fread(buf, 1, 9, f) == 9 && memcmp(buf, "gr\xc3\xbc\xc3\x9f 7\n", 9) == 0);
		fclose(f);
	}

	/* a malformed byte sequence */
	f = tmpfile();
	if(f != NULL){
		fputs("a\xff", f);
		rewind(f);
		errno = 0;
		c = fgetwc(f);
		check("EILSEQ", c == L'a' && fgetwc(f) == WEOF && errno == EILSEQ);
		fclose(f);
	}

	/* wcsftime */
	memset(&tm, 0, sizeof tm);
	tm.tm_year = 126;
	tm.tm_mon = 9;
	tm.tm_mday = 17;
	n = wcsftime(ws, 64, L"%Y-%m-%d é", &tm);
	check("wcsftime", n == 12 && wcscmp(ws, L"2026-10-17 é") == 0);

	printf("c99wchar: %d failed\n", failed);
	return failed != 0;
}
