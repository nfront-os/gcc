/* POSIX clocks over /dev/bintime, timespec_get, nanosleep, usleep,
 * sched_yield, pread and pwrite (APE: ap/gnu). Prints each failure
 * and "c99time: n failed". */
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int failed;

static void
check(const char *what, int ok)
{
	if(!ok){
		printf("c99time: FAIL %s\n", what);
		failed++;
	}
}

static int64_t
ns(struct timespec *ts)
{
	return (int64_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}

int
main(void)
{
	struct timespec a, b, r, req;
	char buf[16];
	int64_t d;
	volatile double x;
	time_t t;
	int i, fd;

	t = time(NULL);
	check("CLOCK_REALTIME", clock_gettime(CLOCK_REALTIME, &a) == 0 &&
		a.tv_sec >= t && a.tv_sec <= t + 1 && a.tv_nsec >= 0 && a.tv_nsec < 1000000000);
	check("timespec_get", timespec_get(&b, TIME_UTC) == TIME_UTC && ns(&b) >= ns(&a));
	check("clock_getres", clock_getres(CLOCK_MONOTONIC, &r) == 0 && r.tv_sec == 0 && r.tv_nsec > 0);
	check("bad clock", clock_gettime(99, &a) == -1);

	/* monotonic: goes on while asleep, never back */
	clock_gettime(CLOCK_MONOTONIC, &a);
	req.tv_sec = 0;
	req.tv_nsec = 50000000;
	check("nanosleep", nanosleep(&req, NULL) == 0);
	clock_gettime(CLOCK_MONOTONIC, &b);
	d = ns(&b) - ns(&a);
	check("monotonic over 50ms", d >= 49000000 && d < 2000000000);
	if(d < 49000000 || d >= 2000000000)
		printf("c99time: slept %lld ns\n", (long long)d);
	a = b;
	check("usleep", usleep(20000) == 0);
	clock_gettime(CLOCK_MONOTONIC, &b);
	check("monotonic over 20ms", ns(&b) - ns(&a) >= 19000000);
	for(i = 0; i < 1000; i++){
		a = b;
		clock_gettime(CLOCK_MONOTONIC, &b);
		if(ns(&b) < ns(&a))
			break;
	}
	check("monotonic never back", i == 1000);
	check("sched_yield", sched_yield() == 0);

	/* CPU time grows with work */
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &a);
	x = 0;
	do{
		for(i = 0; i < 1000000; i++)
			x += i;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &b);
	}while(ns(&b) == ns(&a) && x < 1e15);
	check("CLOCK_PROCESS_CPUTIME_ID", ns(&b) > ns(&a));

	/* pread, pwrite: at an offset, the file offset unchanged */
	fd = open("/tmp/c99time.pread", O_RDWR|O_CREAT|O_TRUNC, 0666);
	check("open", fd >= 0);
	if(fd >= 0){
		check("write", write(fd, "0123456789", 10) == 10);
		check("pwrite", pwrite(fd, "ab", 2, 3) == 2);
		check("pread", pread(fd, buf, 4, 2) == 4 && memcmp(buf, "2ab5", 4) == 0);
		check("offset kept", lseek(fd, 0, SEEK_CUR) == 10);
		close(fd);
		unlink("/tmp/c99time.pread");
	}
	check("pread EBADF", pread(99, buf, 1, 0) == -1);

	printf("c99time: %d failed\n", failed);
	return failed != 0;
}
