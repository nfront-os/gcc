/*
 * 9front-gcc: APE's <sys/uio.h> is a BSD extension and does not compile
 * without _BSD_EXTENSION; a program that includes it wants it.
 * _BSD_EXTENSION only for this header (bsd.h's non-POSIX prototypes stay
 * off, see gcc/config/i386/plan9.h).
 */
#ifndef __9FRONT_GCC_SYS_UIO_H
#define __9FRONT_GCC_SYS_UIO_H
#ifdef _BSD_EXTENSION
#include_next <sys/uio.h>
#else
#define _BSD_EXTENSION
#include_next <sys/uio.h>
#undef _BSD_EXTENSION
#endif
#endif
