/* POSIX threads over 9front's procs (APE's, sys/src/ape/lib/ap/gnu/
 * pthread.c): create, join, detach, mutexes, conditions, rwlocks, once,
 * keys, per-thread errno, main's stack shared. Prints each failure and
 * "pthread: n failed". */
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum { N = 8, ROUNDS = 20000 };

static int failed;

static void
check(const char *what, int ok)
{
	if(!ok){
		printf("pthread: FAIL %s\n", what);
		failed++;
	}
}

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static long counter;

static void *
adder(void *v)
{
	int i;

	for(i = 0; i < ROUNDS; i++){
		pthread_mutex_lock(&m);
		counter++;
		pthread_mutex_unlock(&m);
	}
	return v;
}

/* main's locals, written by another thread */
static void *
poke(void *v)
{
	*(int *)v = 4711;
	return NULL;
}

static void *
errnothread(void *v)
{
	errno = 99;
	usleep(50000);
	return (void *)(long)(errno == 99);
}

static pthread_cond_t c = PTHREAD_COND_INITIALIZER;
static int ready, consumed;

static void *
consumer(void *v)
{
	pthread_mutex_lock(&m);
	while(!ready)
		pthread_cond_wait(&c, &m);
	consumed++;
	pthread_mutex_unlock(&m);
	return NULL;
}

static pthread_key_t key;
static int dtors;

static void
keydtor(void *v)
{
	__atomic_add_fetch(&dtors, 1, __ATOMIC_SEQ_CST);
	free(v);
}

static void *
keyuser(void *v)
{
	pthread_setspecific(key, malloc(16));
	return (void *)(long)(pthread_getspecific(key) != NULL);
}

static int oncecount;

static void
oncefn(void)
{
	oncecount++;
}

static void *
oncer(void *v)
{
	static pthread_once_t once = PTHREAD_ONCE_INIT;

	pthread_once(&once, oncefn);
	return NULL;
}

static int detacheddone;

static void *
detached(void *v)
{
	__atomic_store_n(&detacheddone, 1, __ATOMIC_SEQ_CST);
	return NULL;
}

static void *
exiter(void *v)
{
	pthread_exit((void *)42);
	return NULL;
}

static pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;
static int shared;

static void *
reader(void *v)
{
	int i, x;

	x = 0;
	for(i = 0; i < 1000; i++){
		pthread_rwlock_rdlock(&rw);
		x += shared;
		pthread_rwlock_unlock(&rw);
	}
	return (void *)(long)x;
}

int
main(void)
{
	pthread_t t[N];
	pthread_attr_t a;
	pthread_mutex_t rm, em;
	pthread_mutexattr_t ma;
	struct timespec ts;
	void *ret;
	int i, local, n;

	/* many threads, one mutex */
	for(i = 0; i < N; i++)
		check("create", pthread_create(&t[i], NULL, adder, (void *)(long)i) == 0);
	for(i = 0; i < N; i++){
		check("join", pthread_join(t[i], &ret) == 0 && ret == (void *)(long)i);
	}
	check("mutex counts", counter == (long)N * ROUNDS);
	if(counter != (long)N * ROUNDS)
		printf("pthread: counter %ld\n", counter);

	/* main's stack is shared memory */
	local = 0;
	pthread_create(&t[0], NULL, poke, &local);
	pthread_join(t[0], NULL);
	check("main's local", local == 4711);

	/* errno is per thread */
	errno = 7;
	pthread_create(&t[0], NULL, errnothread, NULL);
	usleep(10000);
	errno = 8;
	pthread_join(t[0], &ret);
	check("errno per thread", ret == (void *)1 && errno == 8);

	/* conditions */
	for(i = 0; i < 4; i++)
		pthread_create(&t[i], NULL, consumer, NULL);
	usleep(50000);
	pthread_mutex_lock(&m);
	ready = 1;
	pthread_cond_broadcast(&c);
	pthread_mutex_unlock(&m);
	for(i = 0; i < 4; i++)
		pthread_join(t[i], NULL);
	check("cond broadcast", consumed == 4);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += 100000000;
	if(ts.tv_nsec >= 1000000000){
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}
	pthread_mutex_lock(&m);
	n = pthread_cond_timedwait(&c, &m, &ts);
	pthread_mutex_unlock(&m);
	check("cond timedwait", n == ETIMEDOUT);

	/* mutex kinds */
	pthread_mutexattr_init(&ma);
	pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&rm, &ma);
	check("recursive", pthread_mutex_lock(&rm) == 0 && pthread_mutex_lock(&rm) == 0 &&
		pthread_mutex_unlock(&rm) == 0 && pthread_mutex_unlock(&rm) == 0 &&
		pthread_mutex_unlock(&rm) == EPERM);
	pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&em, &ma);
	check("errorcheck", pthread_mutex_lock(&em) == 0 && pthread_mutex_lock(&em) == EDEADLK &&
		pthread_mutex_trylock(&em) == EBUSY && pthread_mutex_unlock(&em) == 0);
	pthread_mutex_lock(&m);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += 50000000;
	if(ts.tv_nsec >= 1000000000){
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}
	check("trylock busy", pthread_mutex_trylock(&m) == EBUSY);
	check("timedlock", pthread_mutex_timedlock(&m, &ts) == ETIMEDOUT);
	pthread_mutex_unlock(&m);

	/* keys */
	check("key_create", pthread_key_create(&key, keydtor) == 0);
	for(i = 0; i < 4; i++)
		pthread_create(&t[i], NULL, keyuser, NULL);
	for(i = 0; i < 4; i++){
		pthread_join(t[i], &ret);
		check("setspecific", ret == (void *)1);
	}
	check("key destructors", dtors == 4);
	check("main's key value", pthread_getspecific(key) == NULL);

	/* once */
	for(i = 0; i < N; i++)
		pthread_create(&t[i], NULL, oncer, NULL);
	for(i = 0; i < N; i++)
		pthread_join(t[i], NULL);
	check("once", oncecount == 1);

	/* detached, pthread_exit */
	pthread_attr_init(&a);
	pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&a, 65536);
	check("detached", pthread_create(&t[0], &a, detached, NULL) == 0);
	for(i = 0; i < 100 && !__atomic_load_n(&detacheddone, __ATOMIC_SEQ_CST); i++)
		usleep(10000);
	check("detached ran", detacheddone);
	pthread_create(&t[0], NULL, exiter, NULL);
	pthread_join(t[0], &ret);
	check("pthread_exit", ret == (void *)42);

	/* rwlock */
	shared = 1;
	for(i = 0; i < 4; i++)
		pthread_create(&t[i], NULL, reader, NULL);
	pthread_rwlock_wrlock(&rw);
	shared = 1;
	pthread_rwlock_unlock(&rw);
	for(i = 0; i < 4; i++){
		pthread_join(t[i], &ret);
		check("rwlock readers", ret == (void *)1000);
	}
	check("self", pthread_equal(pthread_self(), pthread_self()));
	check("nprocessors", sysconf(_SC_NPROCESSORS_ONLN) >= 1);

	printf("pthread: %d failed\n", failed);
	return failed != 0;
}
