/*
 * 9front-gcc: C99's printf and scanf families over APE's FILE
 * (libapecompat.a, compat/lib/stdio.c: musl's engine): APE's read every
 * floating argument as a double (%Lf) and lack %a, %F, hh, j, t, z and
 * C99's snprintf result. APE's declarations are renamed while its header
 * is read; ours carry the link names __apecompat_*, so libap keeps its
 * own functions and both use the same FILEs.
 *
 * POSIX.1-2024's asprintf and vasprintf, which APE lacks; popen and
 * pclose, which APE declares only with _BSD_EXTENSION (libbsd; the driver
 * links both).
 */
#define fprintf		__ape_fprintf
#define fscanf		__ape_fscanf
#define printf		__ape_printf
#define scanf		__ape_scanf
#define sprintf		__ape_sprintf
#define snprintf	__ape_snprintf
#define vsnprintf	__ape_vsnprintf
#define sscanf		__ape_sscanf
#define vfprintf	__ape_vfprintf
#define vprintf		__ape_vprintf
#define vsprintf	__ape_vsprintf
#define vfscanf		__ape_vfscanf
#include_next <stdio.h>
#undef fprintf
#undef fscanf
#undef printf
#undef scanf
#undef sprintf
#undef snprintf
#undef vsnprintf
#undef sscanf
#undef vfprintf
#undef vprintf
#undef vsprintf
#undef vfscanf

#ifndef __9FRONT_GCC_STDIO
#define __9FRONT_GCC_STDIO
#ifdef __cplusplus
extern "C" {
#endif
extern int	fprintf(FILE *, const char *, ...)
		__asm__("__apecompat_fprintf");
extern int	fscanf(FILE *, const char *, ...)
		__asm__("__apecompat_fscanf");
extern int	printf(const char *, ...)
		__asm__("__apecompat_printf");
extern int	scanf(const char *, ...)
		__asm__("__apecompat_scanf");
extern int	sprintf(char *, const char *, ...)
		__asm__("__apecompat_sprintf");
extern int	snprintf(char *, size_t, const char *, ...)
		__asm__("__apecompat_snprintf");
extern int	sscanf(const char *, const char *, ...)
		__asm__("__apecompat_sscanf");
extern int	vfprintf(FILE *, const char *, va_list)
		__asm__("__apecompat_vfprintf");
extern int	vfscanf(FILE *, const char *, va_list)
		__asm__("__apecompat_vfscanf");
extern int	vprintf(const char *, va_list)
		__asm__("__apecompat_vprintf");
extern int	vscanf(const char *, va_list)
		__asm__("__apecompat_vscanf");
extern int	vsnprintf(char *, size_t, const char *, va_list)
		__asm__("__apecompat_vsnprintf");
extern int	vsprintf(char *, const char *, va_list)
		__asm__("__apecompat_vsprintf");
extern int	vsscanf(const char *, const char *, va_list)
		__asm__("__apecompat_vsscanf");

extern int	asprintf(char **, const char *, ...)
		__attribute__((__format__(__printf__, 2, 3)));
extern int	vasprintf(char **, const char *, va_list)
		__attribute__((__format__(__printf__, 2, 0)));
#ifndef _BSD_EXTENSION
extern FILE	*popen(const char *, const char *);
extern int	pclose(FILE *);
#endif
#ifdef __cplusplus
}
#endif
#endif
