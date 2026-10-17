/*
 * sysconf with what APE's lacks (compat <unistd.h>: link name
 * __apecompat_sysconf): the number of processors, the lines of
 * /dev/sysstat (cons(3): one a processor), and the page size; the
 * rest is APE's.
 */
#include <fcntl.h>
#include <unistd.h>

extern long __apesysconf(int) __asm__("sysconf");	/* libap's */

static long
nprocs(void)
{
	char buf[1024];
	long n;
	int fd, i, k;

	fd = open("/dev/sysstat", O_RDONLY);
	if(fd < 0)
		return 1;
	n = 0;
	while((k = read(fd, buf, sizeof buf)) > 0)
		for(i = 0; i < k; i++)
			if(buf[i] == '\n')
				n++;
	close(fd);
	return n > 0 ? n : 1;
}

long
sysconf(int name)
{
	switch(name){
	case _SC_NPROCESSORS_CONF:
	case _SC_NPROCESSORS_ONLN:
		return nprocs();
	case _SC_PAGESIZE:
		return 4096;
	}
	return __apesysconf(name);
}
