/*
$Id:  $

Description: Use FOX library for work distribution

$Log: $
*/

#include <stdio.h> /* printf */
#include <stdlib.h> /* exit, atoi, malloc, realloc, free, rand, getenv */
#include <string.h> /* strchr, strlen, strcmp, memcpy */
#include <ctype.h> /* isspace */
#include <errno.h> /* errno */
#ifdef USE_MPI
#include <mpi.h>
#endif
//#define NDEBUG 1
//#include <assert.h> /* assert */

#define TIMEOFDAY
#include "ticks.h"
#include "photon.h"
#include "libfox.h"

#ifndef VERSION
#define VERSION ?.?
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define dbprintf(...) //{fprintf(stdout, __VA_ARGS__); fflush(stdout);}

#define DEFAULT_NPROC 1
#define DEFAULT_PROCID 0
#define DEFAULT_NTASKS 1
#define DEFAULT_NPHOTONS 5000

#define HFLAG 0x01

#define VFLAG 0x1000

/* keys */
#define KEY_READDATA "RD"
#define KEY_WRITEDATA "WR"
#define KEY_TASK "TK"
#define KEY_TASK_SYNC "TKS"

enum {
	s_new,
	s_broad_set,
	s_broad_get,
	s_dist_queue_set,
	s_broad_queue_set,
	s_dist_queue_get,
	s_reduce_set,
	s_reduce_get,
	s_flush,
	s_free,
	S_COUNT /* count of sections */
} SECTIONS;

enum {FALSE, TRUE};

int flags; /* argument flags */
int ntasks = DEFAULT_NTASKS; /* number of tasks to run */
int nphotons = DEFAULT_NPHOTONS; /* number of photons to emit per surface */

int nprocs = DEFAULT_NPROC;
int procid = DEFAULT_PROCID;
int nnodes = 1;
int nodeid = 0;

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

ITEMPTR hosts;

int main(int argc, char *argv[])
{
	int nok = 0;
	char *s;
	tick_t start[S_COUNT], finish[S_COUNT];
	char *section[S_COUNT];

#ifdef USE_MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &procid);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
#else /* USE_MPI */
	if ((s = getenv("SLURM_NPROCS")) != NULL) nprocs = atoi(s);
	if ((s = getenv("SLURM_PROCID")) != NULL) procid = atoi(s);
	if ((s = getenv("SLURM_NNODES")) != NULL) nnodes = atoi(s);
	if ((s = getenv("SLURM_NODEID")) != NULL) nodeid = atoi(s);
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
			case 'n':
				if (isdigit(s[1])) nprocs = atoi(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'p':
				if (isdigit(s[1])) nphotons = atoi(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 's':
				add_item(&hosts, s+1);
				s += strlen(s+1);
				break;
			case 't':
				if (isdigit(s[1])) ntasks = atoi(s+1);
				else nok = 1;
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
		fprintf(stderr, "Usage: mcphoton -vh -i<int> -n<int> -p<int> -s<str> -t<int>\n");
		fprintf(stderr, "  -h  halt servers\n");
		fprintf(stderr, "  -i  process id, default: %d\n", DEFAULT_PROCID);
		fprintf(stderr, "  -n  number of processes, default: %d\n", DEFAULT_NPROC);
		fprintf(stderr, "  -p  photons per surface, default: %d\n", DEFAULT_NPHOTONS);
		fprintf(stderr, "  -s  server name and port (e.g. hostname:port)\n");
		fprintf(stderr, "  -t  tasks, default: %d\n", DEFAULT_NTASKS);
		fprintf(stderr, "  -v  version\n");
		exit(EXIT_FAILURE);
	}

	if (hosts == NULL) add_item(&hosts, "localhost");
	if (flags & VFLAG) {
		fprintf(stderr, "np:%d pid:%d nn:%d nid:%d\n", nprocs, procid, nnodes, nodeid);
	}

	memset(start, 0, sizeof(start));
	memset(finish, 0, sizeof(finish));
	{
		int ret;
		ITEMPTR hptr;
		fox_ptr fhand;
		READONLY *ro_ptr;
		size_t ro_sz;

		section[s_new] = "new";
		tget(start[s_new]);
		fox_new(&fhand, nprocs, procid);
		for (hptr = hosts; hptr != NULL; hptr = hptr->next) {
			if (flags & VFLAG) fprintf(stderr, "%s\n", hptr->obj);
			fox_server_add(fhand, hptr->obj);
		}
		tget(finish[s_new]);

		section[s_broad_set] = "broad_set";
		section[s_broad_get] = "broad_get";
		if (procid == 0) {
			/* initialize and broadcast read-only data */
			tget(start[s_broad_set]);
			ro_sz = sizeof(READONLY);
			ro_ptr = (READONLY *)malloc(ro_sz);
			init_readonly(ro_ptr);
			ret = fox_broad_set(fhand, KEY_READDATA, strlen(KEY_READDATA), (const char *)ro_ptr, ro_sz);
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_broad_set(%s) = %d\n", KEY_READDATA, ret);
				exit(EXIT_FAILURE);
			}
			tget(finish[s_broad_set]);
		} else {
			/* get read data */
			tget(start[s_broad_get]);
			ret = fox_broad_get(fhand, KEY_READDATA, strlen(KEY_READDATA), (char **)&ro_ptr, &ro_sz);
			if (ret != 0 || ro_sz != sizeof(READONLY)) {
				fprintf(stderr, " -- error: fox_broad_get(%s) = %d\n", KEY_READDATA, ret);
				exit(EXIT_FAILURE);
			}
			tget(finish[s_broad_get]);
		}

		{
#if 1
			/* distributed work generation */
			int d = ntasks / nprocs;
			int r = ntasks % nprocs;
			int itsk = d * procid + ((procid < r) ? procid : r);
			int etsk = itsk + d + ((procid < r) ? 1 : 0);
#else
			/* root process work generation */
			int itsk = 0;
			int etsk = (procid == 0) ? ntasks : 0;
#endif
			/* post task id to distributed work queue */
			section[s_dist_queue_set] = "dist_queue_set";
			tget(start[s_dist_queue_set]);
			for (; itsk < etsk; itsk++) {
				ret = fox_dist_queue_set(fhand, KEY_TASK, strlen(KEY_TASK), (const char *)&itsk, sizeof(itsk));
				if (ret != 0) {
					fprintf(stderr, " -- error: fox_dist_queue_set(%s) = %d\n", KEY_TASK, ret);
					exit(EXIT_FAILURE);
				}
			}
			tget(finish[s_dist_queue_set]);
		}

		if (procid == 0) {
			char *ptr;
			size_t sz;
			int itsk;

			if (nprocs > 1) {
				/* wait for end of work generation */
				ret = fox_sync_get(fhand, nprocs-1, KEY_TASK_SYNC, strlen(KEY_TASK_SYNC), &ptr, &sz);
				if (ret != 0) {
					fprintf(stderr, " -- error: fox_sync_get(%s) = %d\n", KEY_TASK_SYNC, ret);
					exit(EXIT_FAILURE);
				}
				free(ptr);
			}

			/* broadcast end-of-tasks flag to distributed work queue */
			section[s_broad_queue_set] = "broad_queue_set";
			tget(start[s_broad_queue_set]);
			itsk = -1;
			ret = fox_broad_queue_set(fhand, KEY_TASK, strlen(KEY_TASK), (const char *)&itsk, sizeof(itsk));
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_broad_queue_set(%s) = %d\n", KEY_TASK, ret);
				exit(EXIT_FAILURE);
			}
			tget(finish[s_broad_queue_set]);
		} else {
			char dummy[4] = {0};
			/* signal end of work generation */
			ret = fox_sync_set(fhand, 1, KEY_TASK_SYNC, strlen(KEY_TASK_SYNC), dummy, sizeof(dummy));
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_sync_set(%s) = %d\n", KEY_TASK_SYNC, ret);
				exit(EXIT_FAILURE);
			}
		}

		{
			int *itsk_ptr;
			size_t itsk_sz;
			WRITEABLE wr;

			/* get task id from distributed work queue and do work */
			section[s_dist_queue_get] = "dist_queue_get";
			tget(start[s_dist_queue_get]);
			init_writeable(&wr);
			for (;;) {
				ret = fox_dist_queue_get(fhand, KEY_TASK, strlen(KEY_TASK), (char **)&itsk_ptr, &itsk_sz);
				if (ret != 0 || itsk_sz != sizeof(int)) {
					fprintf(stderr, " -- error: fox_dist_queue_get(%s) = %d\n", KEY_TASK, ret);
					exit(EXIT_FAILURE);
				}
				if (*itsk_ptr < 0) {free(itsk_ptr); break;}
				taskcode(*itsk_ptr, nphotons, ro_ptr, &wr); /* add results to wr */
				free(itsk_ptr);
			}
			tget(finish[s_dist_queue_get]);

			/* post results */
			//print_writeable(stdout, " *** process results ***", &wr);
			section[s_reduce_set] = "reduce_set";
			tget(start[s_reduce_set]);
			ret = fox_reduce_set(fhand, KEY_WRITEDATA, strlen(KEY_WRITEDATA), (const char *)&wr, sizeof(wr));
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_reduce_set(%s) = %d\n", KEY_WRITEDATA, ret);
				exit(EXIT_FAILURE);
			}
			tget(finish[s_reduce_set]);

			free(ro_ptr);
		}

		if (procid == 0) {
			WRITEABLE results;

			/* get results data */
			section[s_reduce_get] = "reduce_get";
			tget(start[s_reduce_get]);
			init_writeable(&results);
			ret = fox_reduce_get(fhand,
				KEY_WRITEDATA, strlen(KEY_WRITEDATA),
				(char *)&results, sizeof(results),
				(void (*)(void *, void *))collect_writeable);
			if (ret != 0) {
				fprintf(stderr, " -- error: fox_reduce_get(%s) = %d\n", KEY_WRITEDATA, ret);
				exit(EXIT_FAILURE);
			}
			tget(finish[s_reduce_get]);

			print_writeable(stdout, " *** final results ***", &results);
			fprintf(stdout, " Avg time/process (sec)          = %f\n", results.seconds/nprocs);
			fprintf(stdout, " Wall time (sec)                 = %f\n", tsec(finish[s_reduce_get], start[s_new]));
			fflush(stdout);

			/* flush data on servers */
			section[s_flush] = "flush";
			tget(start[s_flush]);
			fox_flush(fhand, (flags & HFLAG)?1:0);
			tget(finish[s_flush]);
		}

		section[s_free] = "free";
		tget(start[s_free]);
		fox_free(fhand);
		tget(finish[s_free]);
	}
#ifdef USE_MPI
	{
		int i;
		double tbuf[S_COUNT], maxbuf[S_COUNT], minbuf[S_COUNT], avgbuf[S_COUNT];

		for (i = 0; i < S_COUNT; i++)
			tbuf[i] = tsec(finish[i], start[i]);
		MPI_Reduce(tbuf, maxbuf, S_COUNT, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
		MPI_Reduce(tbuf, minbuf, S_COUNT, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
		MPI_Reduce(tbuf, avgbuf, S_COUNT, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
		for (i = 0; i < S_COUNT; i++)
			if (avgbuf[i] != maxbuf[i]) avgbuf[i] /= nprocs;

		if (procid == 0) {
			printf(" section          |    max sec |    min sec |    avg sec\n");
			for (i = 0; i < S_COUNT; i++)
				printf(" %-16s %12.6f %12.6f %12.6f\n", section[i], maxbuf[i], minbuf[i], avgbuf[i]);
		}

		MPI_Finalize();
	}
#endif /* USE_MPI */
	return(EXIT_SUCCESS); /* or EXIT_FAILURE */
}

/* TODO:
 */
