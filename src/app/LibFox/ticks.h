/*
$Id: ticks.h,v 1.2 2010/05/12 23:25:24 scott Stab $

Description: Macros for timing
Usage: Define the timer call desired (e.g. #define FTIME, TIMEOFDAY, GETTIME)
before including "ticks.h"

Example:
#define TIMEOFDAY 1
#include "ticks.h"

void benchmark(void)
{
	tick_t start, finish;

	tget(start);
	timed_section();
	tget(finish);
	printf("time in seconds:%f\n", tsec(finish, start));
}

$Log: ticks.h,v $
Revision 1.2  2010/05/12 23:25:24  scott
Update comments for version control

*/

#ifndef _TICKS_H
#define _TICKS_H

/*
Casting the ticks to a signed value allows a more efficient
conversion to a float on x86 CPUs, but it limits the count to one
less high-order bit. Uncomment "unsigned" to use a full tick count.
*/
#define us /*unsigned*/

#if defined(FTIME)

/* deprecated */
#include <sys/timeb.h>
typedef struct timeb tick_t;
#define TICKS_SEC 1000
#define tget(t) ftime(&t)
#define tdiff(f,s) (((unsigned long)f.time*TICKS_SEC+f.millitm)-((unsigned long)s.time*TICKS_SEC+s.millitm))
#define tsec(f,s) ((us long)tdiff(f,s)/(double)TICKS_SEC)

#elif defined(TIMEOFDAY)

#include <sys/time.h>
typedef struct timeval tick_t;
#define TICKS_SEC 1000000UL
#define tget(t) gettimeofday(&t,NULL)
#define tdiff(f,s) (((unsigned long)f.tv_sec*TICKS_SEC+f.tv_usec)-((unsigned long)s.tv_sec*TICKS_SEC+s.tv_usec))
#define tsec(f,s) ((us long)tdiff(f,s)/(double)TICKS_SEC)

#elif defined(GETTIME)

/* must link with lib -lrt */
#include <time.h>
typedef struct timespec tick_t;
#define TICKS_SEC 1000000000UL
/* clock types: CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID */
#define tget(t) clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t)
#define tdiff(f,s) (((unsigned long long)f.tv_sec*TICKS_SEC+f.tv_nsec)-((unsigned long long)s.tv_sec*TICKS_SEC+s.tv_nsec))
#define tsec(f,s) ((us long long)tdiff(f,s)/(double)TICKS_SEC)

#elif defined(GETRUSAGE)

#include <sys/resource.h>
typedef struct rusage tick_t;
#define TICKS_SEC 1000000UL
/* who types: RUSAGE_SELF, RUSAGE_CHILDREN */
#define tget(t) getrusage(RUSAGE_SELF, &(t))
#define tdiff(f,s) (((unsigned long)(f).ru_utime.tv_sec*TICKS_SEC+(f).ru_utime.tv_usec)-((unsigned long)(s).ru_utime.tv_sec*TICKS_SEC+(s).ru_utime.tv_usec))
#define tsec(f,s) ((us long)tdiff(f,s)/(double)TICKS_SEC)
//#define tdiff(f,s,r) timersub(&(f).ru_utime, &(s).ru_utime, &(r).ru_utime)
//#define tsec(t) ((double)(t).ru_utime.tv_sec+(double)(t).ru_utime.tv_usec/(double)TICKS_SEC)

#elif defined(RDTSC)

typedef union {
	unsigned long long ui64;
	struct {unsigned int lo, hi;} ui32;
} tick_t;
//#define TICKS_SEC 1662000000UL
//#define TICKS_SEC 2497000000UL
#ifndef TICKS_SEC
#error TICKS_SEC must be defined to match the CPU clock frequency
#endif
#define tget(t) __asm__ __volatile__ ("lfence\n\trdtsc" : "=a" ((t).ui32.lo), "=d"((t).ui32.hi))
#define tdiff(f,s) ((f).ui64-(s).ui64)
#define tsec(f,s) ((us long long)tdiff(f,s)/(double)TICKS_SEC)

#else

#include <time.h>
typedef clock_t tick_t;
#define TICKS_SEC CLOCKS_PER_SEC
#define tget(t) t = clock()
#define tdiff(f,s) (f-s)
#define tsec(f,s) ((us long)tdiff(f,s)/(double)TICKS_SEC)

#endif

#endif /* _TICKS_H */
