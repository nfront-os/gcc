/*
 * 9front-gcc: <sys/sysmacros.h> as on Linux: major, minor and makedev
 * of a dev_t (APE's stat: the device's number; there are no device
 * numbers of this kind on 9front).
 */
#ifndef __9FRONT_GCC_SYS_SYSMACROS_H
#define __9FRONT_GCC_SYS_SYSMACROS_H
#define major(d)	((unsigned)(d) >> 8 & 0xFFF)
#define minor(d)	((unsigned)(d) & 0xFF)
#define makedev(x, y)	((dev_t)(((x) & 0xFFF) << 8 | ((y) & 0xFF)))
#endif
