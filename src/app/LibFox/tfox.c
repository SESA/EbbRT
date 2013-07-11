/*
$Id:  $

Description: Test and benchmark the FOX library

$Log: $
*/

#include <stdio.h>
#include <stdlib.h> /* atoi, getenv */
#include <string.h> /* memset */
#ifdef USE_MPI
#include <mpi.h>
#endif

#define TIMEOFDAY
#include "ticks.h"
#include "libfox.h"

#ifndef VERSION
#define VERSION ?.?
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define dbprintf(...) //{fprintf(stderr, __VA_ARGS__);}

#define DEFAULT_NPROC 1
#define DEFAULT_PROCID 0
#define DEFAULT_LOOP 10 /* default number of loop iterations */

#define MAX_KEY 32

#define HFLAG 0x01

#define VFLAG 0x1000

enum {
	s_fox_set,
	s_fox_get,
	s_fox_sync_set,
	s_fox_sync_get,
	s_fox_broad_set,
	s_fox_broad_get,
	s_fox_reduce_set,
	s_fox_reduce_get,
	s_fox_queue_set_A,
	s_fox_queue_get_A,
	s_fox_queue_set_B,
	s_fox_queue_get_B,
	S_COUNT /* count of sections */
} SECTIONS;

typedef struct _item {
	void *obj;
	struct _item *next;
} ITEM, *ITEMPTR;

void add_item(ITEMPTR *plist, void *obj)
{
	ITEMPTR iptr = (ITEMPTR)malloc(sizeof(ITEM));
	if (iptr == NULL) return;
	iptr->obj = obj;
	iptr->next = *plist;
	*plist = iptr;
}

void accumulate(int *out, int *in)
{
	*out += *in;
}

/* Since the current implementation of synchronous semaphores does not preserve
order of access, adjacent usages of FOX BARRIER must use different keys. */

#define ROOT DEFAULT_PROCID

#ifdef USE_MPI
#define BARRIER(key) MPI_Barrier(MPI_COMM_WORLD);
#define RENDEZVOUS(key) MPI_Reduce(&procid, &ret, 1, MPI_INT, MPI_SUM, ROOT, MPI_COMM_WORLD);
#define DISPERSE(key) MPI_Bcast(&ret, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
#else
#define BARRIER(key) fox_collect(fhand, #key, strlen(#key), ROOT, C_BARRIER);
#define RENDEZVOUS(key) fox_collect(fhand, #key, strlen(#key), ROOT, C_RENDEZVOUS);
#define DISPERSE(key) fox_collect(fhand, #key, strlen(#key), ROOT, C_DISPERSE);
#endif

/* All processes must be held in wait before flush */
#define FLUSH RENDEZVOUS(_FL) if (procid == ROOT) fox_flush(fhand, 0); DISPERSE(_FL)


int
main(int argc, char *argv[])
{
	/* arg processing */
	int nok = 0;
	char *s;

	/* args */
	int flags = 0; /* argument flags */
	int procid = DEFAULT_PROCID;
	int nprocs = DEFAULT_NPROC;
	int loop = DEFAULT_LOOP;
	ITEMPTR hosts = NULL;

	/* timers */
	tick_t start[S_COUNT], finish[S_COUNT];
	char *section[S_COUNT];
	double tbuf[S_COUNT], maxbuf[S_COUNT], minbuf[S_COUNT], avgbuf[S_COUNT];

	/* application */
	int i;
	int ret;
	ITEMPTR hptr;
	fox_ptr fhand;

#ifdef USE_MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &procid);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
#else /* USE_MPI */
	if ((s = getenv("SLURM_NPROCS")) != NULL) nprocs = atoi(s);
	if ((s = getenv("SLURM_PROCID")) != NULL) procid = atoi(s);
#endif /* USE_MPI */
	if ((s = getenv("HOSTLIST")) != NULL) add_item(&hosts, s);

	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s; s++)
			switch (*s) {
			case 'h':
				flags |= HFLAG;
				break;
			case 'i':
				if (isdigit(s[1])) procid = atoi(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'l':
				if (isdigit(s[1])) loop = atoi(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'n':
				if (isdigit(s[1])) nprocs = atoi(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 's':
				add_item(&hosts, s+1);
				s += strlen(s+1);
				break;
			case 'v':
				flags |= VFLAG;
				break;
			default:
				nok = 1;
				fprintf(stderr, " -- not an option: %c\n", *s);
				break;
			}

	if (flags & VFLAG) fprintf(stderr, "Version: %s\n", TOSTRING(VERSION));
	if (nok || /*argc < 1 ||*/ (argc > 0 && *argv[0] == '?')) {
		fprintf(stderr, "Usage: tfox -vh -i<int> -n<int> -s<str> -t<int>\n");
		fprintf(stderr, "  -h  halt servers\n");
		fprintf(stderr, "  -i  process id, default: %d\n", DEFAULT_PROCID);
		fprintf(stderr, "  -l  loop count, default: %d\n", DEFAULT_LOOP);
		fprintf(stderr, "  -n  number of processes, default: %d\n", DEFAULT_NPROC);
		fprintf(stderr, "  -s  server name and port (e.g. hostname:port)\n");
		fprintf(stderr, "  -v  version\n");
		exit(EXIT_FAILURE);
	}

/* * * * * * * * * * Initialization * * * * * * * * * */

	if (hosts == NULL) add_item(&hosts, "localhost");
	if (flags & VFLAG) {
		fprintf(stderr, "np:%d pid:%d\n", nprocs, procid);
	}

	memset(start, 0, sizeof(start));
	memset(finish, 0, sizeof(finish));

	fox_new(&fhand, nprocs, procid);
	for (hptr = hosts; hptr != NULL; hptr = hptr->next) {
		if (flags & VFLAG) fprintf(stderr, "%s\n", hptr->obj);
		fox_server_add(fhand, hptr->obj);
	}

/* Use key-value server so that it is initialized (internal structures setup)
   and doesn't delay subsequent timed sections */
#define init_sz 4096
	BARRIER(A)
	dbprintf("fox_queue_set (init)\n");
	for (i = 4; i <= init_sz; i+=i) {
		size_t buf_sz = i;
		char *buf_ptr = (char *)malloc(buf_sz);
		char key[MAX_KEY];
		memset(buf_ptr, 0xFF, buf_sz);
		sprintf(key, "QI:%d", procid);
		ret = fox_queue_set(fhand, key, strlen(key), (const char *)buf_ptr, buf_sz);
		if (ret != 0) {
			fprintf(stderr, " -- error: fox_queue_set(%s) = %d\n", key, ret);
			exit(EXIT_FAILURE);
		}
		free(buf_ptr);
	}

	BARRIER(B)
	dbprintf("fox_queue_get (init)\n");
	for (i = 4; i <= init_sz; i+=i) {
		size_t buf_sz;
		char *buf_ptr;
		char key[MAX_KEY];
		sprintf(key, "QI:%d", procid);
		ret = fox_queue_get(fhand, key, strlen(key), (char **)&buf_ptr, &buf_sz);
		if (ret != 0 || buf_sz != i) {
			fprintf(stderr, " -- error: fox_queue_get(%s) = %d\n", key, ret);
			exit(EXIT_FAILURE);
		}
		free(buf_ptr);
	}
	FLUSH

/* * * * * * * * * * Timed Sections * * * * * * * * * */

/* * * * * * * * * * fundamental set & get * * * * * * * * * */

#define fund_sz 4
	BARRIER(A)
	dbprintf("fox_set\n");
	section[s_fox_set] = "fox_set";
	{
		size_t buf_sz = fund_sz;
		char *buf_ptr = (char *)malloc(buf_sz);
		memset(buf_ptr, 0xFF, buf_sz);
		tget(start[s_fox_set]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "F:%d:%d", procid, i);
			ret = fox_set(fhand, key, strlen(key), (const char *)buf_ptr, buf_sz);
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_set(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
		}
		tget(finish[s_fox_set]);
		free(buf_ptr);
	}

	BARRIER(B)
	dbprintf("fox_get\n");
	section[s_fox_get] = "fox_get";
	{
		size_t buf_sz;
		char *buf_ptr;
		tget(start[s_fox_get]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "F:%d:%d", procid, i);
			ret = fox_get(fhand, key, strlen(key), (char **)&buf_ptr, &buf_sz);
			if (ret != 0 || buf_sz != fund_sz) {
				fprintf(stderr, " -- error: fox_get(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
			free(buf_ptr);
		}
		tget(finish[s_fox_get]);
	}
	FLUSH

/* * * * * * * * * * synchronization * * * * * * * * * */

#define sync_sz 4
	BARRIER(A)
	dbprintf("fox_sync_set\n");
	section[s_fox_sync_set] = "fox_sync_set";
	{
		size_t buf_sz = sync_sz;
		char *buf_ptr = (char *)malloc(buf_sz);
		memset(buf_ptr, 0xFF, buf_sz);
		tget(start[s_fox_sync_set]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "S:%d:%d", procid, i);
			ret = fox_sync_set(fhand, 1, key, strlen(key), (const char *)buf_ptr, buf_sz);
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_sync_set(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
		}
		tget(finish[s_fox_sync_set]);
		free(buf_ptr);
	}

	BARRIER(B)
	dbprintf("fox_sync_get\n");
	section[s_fox_sync_get] = "fox_sync_get";
	{
		size_t buf_sz;
		char *buf_ptr;
		tget(start[s_fox_sync_get]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "S:%d:%d", procid, i);
			ret = fox_sync_get(fhand, 1, key, strlen(key), (char **)&buf_ptr, &buf_sz);
			if (ret != 0 || buf_sz != sync_sz) {
				fprintf(stderr, " -- error: fox_sync_get(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
			free(buf_ptr);
		}
		tget(finish[s_fox_sync_get]);
	}
	FLUSH

/* * * * * * * * * * broadcast * * * * * * * * * */

#define broad_sz 4096
	BARRIER(A)
	if (procid == ROOT) dbprintf("fox_broad_set\n");
	section[s_fox_broad_set] = "fox_broad_set";
	if (procid == ROOT) { /* one broadcaster, not enough server memory at scale */
		size_t buf_sz = broad_sz;
		char *buf_ptr = (char *)malloc(buf_sz);
		memset(buf_ptr, 0xFF, buf_sz);
		tget(start[s_fox_broad_set]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "B:%d", i);
			ret = fox_broad_set(fhand, key, strlen(key), (const char *)buf_ptr, buf_sz);
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_broad_set(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
		}
		tget(finish[s_fox_broad_set]);
		free(buf_ptr);
	}

	BARRIER(B)
	dbprintf("fox_broad_get\n");
	section[s_fox_broad_get] = "fox_broad_get";
	{
		size_t buf_sz;
		char *buf_ptr;
		tget(start[s_fox_broad_get]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "B:%d", i);
			ret = fox_broad_get(fhand, key, strlen(key), (char **)&buf_ptr, &buf_sz);
			if (ret != 0 || buf_sz != broad_sz) {
				fprintf(stderr, " -- error: fox_broad_get(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
			free(buf_ptr);
		}
		tget(finish[s_fox_broad_get]);
	}
	FLUSH

/* * * * * * * * * * reduce * * * * * * * * * */

#define reduce_sz sizeof(int)
	BARRIER(A)
	dbprintf("fox_reduce_set\n");
	section[s_fox_reduce_set] = "fox_reduce_set";
	{
		size_t buf_sz = reduce_sz;
		char *buf_ptr = (char *)malloc(buf_sz);
		memcpy(buf_ptr, &procid, sizeof(int));
		tget(start[s_fox_reduce_set]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "R:%d", i);
			ret = fox_reduce_set(fhand, key, strlen(key), (const char *)buf_ptr, buf_sz);
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_reduce_set(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
		}
		tget(finish[s_fox_reduce_set]);
		free(buf_ptr);
	}

	BARRIER(B)
	if (procid == ROOT) dbprintf("fox_reduce_get\n");
	section[s_fox_reduce_get] = "fox_reduce_get";
	if (procid == ROOT) { /* one reducer */
		size_t buf_sz = reduce_sz;
		char *buf_ptr = (char *)malloc(buf_sz);
		tget(start[s_fox_reduce_get]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "R:%d", i);
			memset(buf_ptr, 0, sizeof(int));
			ret = fox_reduce_get(fhand, key, strlen(key), buf_ptr, buf_sz,
				(void (*)(void *, void *))accumulate);
			if (ret != 0 || *(int *)buf_ptr != nprocs*(nprocs-1)/2) {
				fprintf(stderr, " -- error: fox_reduce_get(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
		}
		tget(finish[s_fox_reduce_get]);
		free(buf_ptr);
	}
	FLUSH

/* * * * * * * * * * queue * * * * * * * * * */

#define queue_sz_A 4
	BARRIER(A)
	dbprintf("fox_queue_set_A\n");
	section[s_fox_queue_set_A] = "fox_queue_set_A";
	{
		size_t buf_sz = queue_sz_A;
		char *buf_ptr = (char *)malloc(buf_sz);
		memset(buf_ptr, 0xFF, buf_sz);
		tget(start[s_fox_queue_set_A]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "QA:%d", procid);
			ret = fox_queue_set(fhand, key, strlen(key), (const char *)buf_ptr, buf_sz);
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_queue_set(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
		}
		tget(finish[s_fox_queue_set_A]);
		free(buf_ptr);
	}

	BARRIER(B)
	dbprintf("fox_queue_get_A\n");
	section[s_fox_queue_get_A] = "fox_queue_get_A";
	{
		size_t buf_sz;
		char *buf_ptr;
		tget(start[s_fox_queue_get_A]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "QA:%d", procid);
			ret = fox_queue_get(fhand, key, strlen(key), (char **)&buf_ptr, &buf_sz);
			if (ret != 0 || buf_sz != queue_sz_A) {
				fprintf(stderr, " -- error: fox_queue_get(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
			free(buf_ptr);
		}
		tget(finish[s_fox_queue_get_A]);
	}
	FLUSH

#define queue_sz_B (8*1024) /* test larger value lengths */
	BARRIER(A)
	dbprintf("fox_queue_set_B\n");
	section[s_fox_queue_set_B] = "fox_queue_set_B";
	{
		size_t buf_sz = queue_sz_B;
		char *buf_ptr = (char *)malloc(buf_sz);
		memset(buf_ptr, 0xFF, buf_sz);
		tget(start[s_fox_queue_set_B]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "QB:%d", procid);
			ret = fox_queue_set(fhand, key, strlen(key), (const char *)buf_ptr, buf_sz);
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_queue_set(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
		}
		tget(finish[s_fox_queue_set_B]);
		free(buf_ptr);
	}

	BARRIER(B)
	dbprintf("fox_queue_get_B\n");
	section[s_fox_queue_get_B] = "fox_queue_get_B";
	{
		size_t buf_sz;
		char *buf_ptr;
		tget(start[s_fox_queue_get_B]);
		for (i = 0; i < loop; i++) {
			char key[MAX_KEY];
			sprintf(key, "QB:%d", procid);
			ret = fox_queue_get(fhand, key, strlen(key), (char **)&buf_ptr, &buf_sz);
			if (ret != 0 || buf_sz != queue_sz_B) {
				fprintf(stderr, " -- error: fox_queue_get(%s) = %d\n", key, ret);
				exit(EXIT_FAILURE);
			}
			free(buf_ptr);
		}
		tget(finish[s_fox_queue_get_B]);
	}
	FLUSH

/* * * * * * * * * * Collect Times * * * * * * * * * */

	for (i = 0; i < S_COUNT; i++)
		tbuf[i] = tsec(finish[i], start[i]) / loop * 1000000;
#ifdef USE_MPI
	MPI_Reduce(tbuf, maxbuf, S_COUNT, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(tbuf, minbuf, S_COUNT, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(tbuf, avgbuf, S_COUNT, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
#else /* USE_MPI */
	memcpy(maxbuf, tbuf, sizeof(maxbuf));
	memcpy(minbuf, tbuf, sizeof(minbuf));
	memcpy(avgbuf, tbuf, sizeof(avgbuf));
#endif /* USE_MPI */
	for (i = 0; i < S_COUNT; i++)
		if (avgbuf[i] != maxbuf[i]) avgbuf[i] /= nprocs;

	if (procid == ROOT) {
		printf("section          |   max usec |   min usec |   avg usec\n");
		for (i = 0; i < S_COUNT; i++)
			printf("%-16s %12.0f %12.0f %12.0f\n", section[i], maxbuf[i], minbuf[i], avgbuf[i]);
	}

	RENDEZVOUS(TERM)
	/* terminate server */
	if (procid == ROOT) fox_flush(fhand, (flags & HFLAG)?1:0);
	fox_free(fhand);

#ifdef USE_MPI
	MPI_Finalize();
#endif /* USE_MPI */

	return 0;
}
