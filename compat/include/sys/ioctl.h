/*
 * 9front-gcc: APE's <sys/ioctl.h> is a BSD extension and does not compile
 * without _BSD_EXTENSION; a program that includes it wants it.
 * _BSD_EXTENSION only for this header (bsd.h's non-POSIX prototypes stay
 * off, see gcc/config/i386/plan9.h).
 */
#ifndef __9FRONT_GCC_SYS_IOCTL_H
#define __9FRONT_GCC_SYS_IOCTL_H
#ifdef _BSD_EXTENSION
#include_next <sys/ioctl.h>
#else
#define _BSD_EXTENSION
#include_next <sys/ioctl.h>
#undef _BSD_EXTENSION
#endif
#endif
