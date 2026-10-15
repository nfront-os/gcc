/* native Plan 9 C: -nostdinc -I/amd64/include -I/sys/include, libc */
#include <u.h>
#include <libc.h>

void
main(int argc, char **argv)
{
	USED(argc);
	print("plan9: %s %d\n", argv[0], 6*7);
	exits(nil);
}
