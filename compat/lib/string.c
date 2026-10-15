/*
 * POSIX.1-2008 string functions APE lacks: stpcpy, stpncpy, and
 * strndup, which APE's string.h declares but libap does not have.
 */
#include <stdlib.h>
#include <string.h>

char*
stpcpy(char *d, const char *s)
{
	while((*d = *s++) != '\0')
		d++;
	return d;
}

char*
stpncpy(char *d, const char *s, size_t n)
{
	size_t l;

	l = strnlen(s, n);
	memcpy(d, s, l);
	memset(d+l, 0, n-l);
	return d+l;
}

char*
strndup(const char *s, size_t n)
{
	size_t l;
	char *d;

	l = strnlen(s, n);
	d = malloc(l+1);
	if(d == NULL)
		return NULL;
	memcpy(d, s, l);
	d[l] = '\0';
	return d;
}
