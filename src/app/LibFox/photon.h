/* file photon.h */

#ifndef _PHOTON_H
#define _PHOTON_H

#include <stdio.h>

#ifndef PHOTONFLOATTYPE
#define PHOTONFLOATTYPE float
#endif
typedef PHOTONFLOATTYPE PHOTONFLOAT;

/*
#define ntasks0 1
#define ntasksm ntasks0-1
#define NTASKS  ntasks0
*/

/*#define npp0  5000*/
#define nppm  (npp0-1) /* not used */
#define NPP   npp0

#define ns0   37
#define nsm   (ns0-1)
#define NSM   ns0

#define nseq0 1
#define nseqm (nseq0-1) /* not used */

#define tau0  0.100
#define rhod0 0.300
#define rhos0 0.500

#define modlus31 2147483647
#define epsdet0  1.e-10
#define pi0      3.1415926536

#define lensin0 90
#define lensinm (lensin0-1) /* not used */
#define LENSIN  lensin0

#define lencos0 (4*lensin0)
#define lencosm (lencos0-1) /* not used */
#define LENCOS  lencos0

struct readonly {
	PHOTONFLOAT
		x1    [NSM],    x2    [NSM],
		y1    [NSM],    y2    [NSM],
		delx  [NSM],    dely  [NSM],
		enx   [NSM],    eny   [NSM],
		sqln  [NSM],    rhs   [NSM],
		spec1 [NSM],    spec2 [NSM],
		sintab[LENSIN], costab[LENCOS];
};

/* ALLOCATE ONE FOR EACH TASK */
struct writeable {
	long nevents; /* number of photons emitted */
	double seconds; /* seconds of compute time */
	int icounte[NSM][NSM]; /* absorbed count [from surface][to surface] */
	int icountt[NSM][NSM]; /* transmit count [from surface][to surface] */
};

typedef struct readonly READONLY;
typedef struct writeable WRITEABLE;

void taskcode(int itsk, int npp0, READONLY *readonly, WRITEABLE *writeable);
void init_readonly(READONLY *readonly);
void init_writeable(WRITEABLE *writeable);
void collect_writeable(WRITEABLE *out, WRITEABLE *in);
void print_writeable(FILE *fp, char *header, WRITEABLE *writeable);
void print_initial(FILE *fp, char *header);
void get_ngon(PHOTONFLOAT x1[], PHOTONFLOAT y1[], PHOTONFLOAT x2[], PHOTONFLOAT y2[]);
int  iran31(int oldseed);

#endif /* _PHOTON_H */
