/*
 * 9front-gcc: isascii and toascii (XSI), which APE defines only with
 * _BSD_EXTENSION; as APE's macros.
 */
#include_next <ctype.h>

#ifndef __9FRONT_GCC_CTYPE
#define __9FRONT_GCC_CTYPE
#ifndef isascii
#define isascii(c)	(((unsigned int)(c))<0x80)
#endif
#ifndef toascii
#define toascii(c)	((unsigned)(c) & 0x007f)
#endif
#endif
