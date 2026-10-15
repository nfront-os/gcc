/*
 * 9front-gcc: APE's <sys/socket.h> is a BSD extension and does not compile
 * without _BSD_EXTENSION; a program that includes it wants it.
 * _BSD_EXTENSION only for this header (bsd.h's non-POSIX prototypes stay
 * off, see gcc/config/i386/plan9.h). APE's socket functions take int
 * lengths and return int, not POSIX's socklen_t and ssize_t; the
 * declarations here are APE's, which callers then use consistently.
 */
#ifndef __9FRONT_GCC_SYS_SOCKET_H
#define __9FRONT_GCC_SYS_SOCKET_H
#ifdef _BSD_EXTENSION
#include_next <sys/socket.h>
#else
#define _BSD_EXTENSION
#include_next <sys/socket.h>
#undef _BSD_EXTENSION
#endif
#endif
