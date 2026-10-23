/*
 * 9front-gcc: <sys/stat.h> as POSIX has it, over APE's: fchmod (libap
 * has it, no header declares it), lstat (libbsd) and the S_IF* file
 * types, which APE has only with _BSD_EXTENSION (the same values),
 * S_ISLNK (APE: only _RESEARCH_SOURCE; 9front has no symbolic links)
 * and mkfifoat (compat/lib/at.c).
 */
#ifndef __9FRONT_GCC_SYS_STAT_H
#define __9FRONT_GCC_SYS_STAT_H
#include_next <sys/stat.h>
#ifdef __cplusplus
extern "C" {
#endif
extern int	fchmod(int, mode_t);
extern int	mkfifoat(int, const char *, mode_t);
#ifndef _BSD_EXTENSION
extern int	lstat(const char *, struct stat *);
#endif
#ifdef __cplusplus
}
#endif
#ifndef S_IFMT
#define S_IFMT	S__MASK
#define S_IFDIR	0040000
#define S_IFCHR	0020000
#define S_IFBLK	0060000
#define S_IFREG	0100000
#define S_IFIFO	0010000
#define S_IFLNK	0120000
#define S_IFSOCK S_IFIFO
#endif
#ifndef S_ISLNK
#define S_ISLNK(m)	(((m)&S__MASK) == 0120000)
#endif
#endif
