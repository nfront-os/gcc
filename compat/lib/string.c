/*
 * POSIX string functions APE lacks: stpcpy, stpncpy, strndup, which
 * APE's string.h declares but libap does not have, and memmem.
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

void*
memmem(const void *h, size_t hn, const void *n, size_t nn)
{
	const unsigned char *p, *e;

	if(nn == 0)
		return (void*)h;
	if(hn < nn)
		return NULL;
	e = (const unsigned char*)h + (hn - nn);
	for(p = h; p <= e; p++)
		if(*p == *(const unsigned char*)n && memcmp(p, n, nn) == 0)
			return (void*)p;
	return NULL;
}
