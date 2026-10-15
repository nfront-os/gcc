/*
 * 9front-gcc: APE's <sys/select.h> is a BSD extension and does not compile
 * without _BSD_EXTENSION; a program that includes it wants it.
 * _BSD_EXTENSION only for this header (bsd.h's non-POSIX prototypes stay
 * off, see gcc/config/i386/plan9.h).
 */
#ifndef __9FRONT_GCC_SYS_SELECT_H
#define __9FRONT_GCC_SYS_SELECT_H
#ifdef _BSD_EXTENSION
#include_next <sys/select.h>
#else
#define _BSD_EXTENSION
#include_next <sys/select.h>
#undef _BSD_EXTENSION
#endif
#endif
