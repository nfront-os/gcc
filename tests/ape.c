/* APE: stdio, stdlib, unistd with the driver's defaults (-D_POSIX_SOURCE ...) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	char buf[64];
	long long v;

	v = strtoll("123456789012", NULL, 10);
	snprintf(buf, sizeof buf, "%lld %d", v * 2, argc);
	printf("ape: %s %s pid>0=%d\n", buf, argv[0], getpid() > 0);
	return strcmp(buf, "246913578024 1") == 0 && getpid() > 0 ? 0 : 1;
}
