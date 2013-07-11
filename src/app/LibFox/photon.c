/*                  ********************                  */
/*                  ********************                  */
/*                  **                **                  */
/*                  **  MAIN PROGRAM  **                  */
/*                  **                **                  */
/*                  ********************                  */
/*                  ********************                  */

#define DO_TIME

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef DO_TIME
#define GETRUSAGE
#include "ticks.h"
#endif
#include "photon.h"

#define ifix(f) (int)(f)


#ifdef MAIN
#define NTASKS 10 /* number of tasks to run */
#define NPHOTONS 5000 /* number of photons to emit per surface */

int
main(int argc, char *argv[])
{
	static READONLY readonly;
	static WRITEABLE wglobal;
	static WRITEABLE wtask;
	int itsk;
	char str[80];

	fprintf(stdout, "size of READONLY:%d\n", sizeof(READONLY));
	fprintf(stdout, "size of WRITEABLE:%d\n", sizeof(WRITEABLE));

	/*print_initial(stdout, "\n *** initial data ***");*/

	/* initialize read variables */
	init_readonly(&readonly);

	/* initialize writeable variables, execute tasks, and collect */
	init_writeable(&wglobal);
	for (itsk = 0; itsk < NTASKS; itsk++) {
		init_writeable(&wtask);
		taskcode(itsk, NPHOTONS, &readonly, &wtask);
		collect_writeable(&wglobal, &wtask);
		/*sprintf(str, "\n *** task %d results ***", itsk);
		print_writeable(stdout, str, &wtask);*/
	}

	print_writeable(stdout, "\n *** final results ***", &wglobal);
}
#endif

/*            ********************************            */
/*            ********************************            */
/*            **                            **            */
/*            **  CODE FOR A SINGLE THREAD  **            */
/*            **                            **            */
/*            ********************************            */
/*            ********************************            */

void
taskcode(int itsk, int npp0, READONLY *readonly, WRITEABLE *writeable)
{
	PHOTONFLOAT scldenom, scalephi, scaletha, fnplfinv;
#ifdef ntasks0
	PHOTONFLOAT fitsk, fntasks, toffset;
#endif
	int iseedphi, iseedth, iseedrn;
	int le0;

#ifdef DO_TIME
	tick_t start, finish;
	tget(start);
#endif

	/*printf("\nin taskcode - itsk = %d\n", itsk);*/
	/*printf("readonly addr %p, writeable addr %p\n", readonly, writeable);*/

	iseedphi = 3*(itsk) + 1;
	iseedth  = 3*(itsk) + 2;
	iseedrn  = 3*(itsk) + 3;

	scldenom = 1.0/((PHOTONFLOAT)(modlus31)-2.0);
	scalephi = (PHOTONFLOAT)(lencos0)*scldenom;
	scaletha = (PHOTONFLOAT)(lensin0)*scldenom;

#ifdef ntasks0
	fitsk    = (PHOTONFLOAT)(itsk);
	fntasks  = (PHOTONFLOAT)(ntasks0);
	toffset = (fitsk + .5)/fntasks;
#endif
	fnplfinv = 1./(PHOTONFLOAT)(npp0);

	for (le0 = 0; le0 < NSM; le0++) {
		PHOTONFLOAT dxcrnt, dycrnt, xe0, ye0;
		int i;

		/*printf("le0=%d\n", le0);*/
		dxcrnt  = readonly->delx[le0]*fnplfinv;
		dycrnt  = readonly->dely[le0]*fnplfinv;
#ifdef ntasks0
		xe0     = readonly->x1[le0] + (toffset - 1.0)*dxcrnt;
		ye0     = readonly->y1[le0] + (toffset - 1.0)*dycrnt;
#else
		xe0 = readonly->x1[le0];
		ye0 = readonly->y1[le0];
#endif

		for (i = 0; i < NPP; i++) {
			PHOTONFLOAT xe, ye, exloc, eyloc, ex, ey;
			int le, ixphi, ixth, iphi, ith, idead;

			xe0 += dxcrnt;
			ye0 += dycrnt;
			xe  = xe0;
			ye  = ye0;
			le  = le0;

			/*                         EMIT                         */

			iseedphi= iran31(iseedphi);
			iseedth = iran31(iseedth);
			ixphi = iseedphi - 1;
			ixth  = iseedth  - 1;
			iphi  = ifix((PHOTONFLOAT)(ixphi)*scalephi);
			ith   = ifix((PHOTONFLOAT)(ixth )*scaletha);
			exloc = readonly->sintab[ith]*readonly->costab[iphi];
			eyloc = readonly->costab[ith];
			ex    = exloc*readonly->eny[le] + eyloc*readonly->enx[le];
			ey    = eyloc*readonly->eny[le] - exloc*readonly->enx[le];
			idead = 0;

			/*                        INTSEC                        */
			/*               main loop over surfaces                */
			/*     for now this only works for convex surfaces      */

			while (! idead) {
				PHOTONFLOAT rhse;
				PHOTONFLOAT xi = 0.0, yi = 0.0; /* intersection point */
				int li, l;

				writeable->nevents += 1;
				rhse = ex*ye - ey*xe;
				li = le;
				for (l = 0; l < NSM; l++) {
					/*printf("le0=%d, i=%d, l=%d, li=%d, le=%d\n", le0, i, l, li, le);*/
					/* skip the emitting surface */
					if (l != le)  {
						PHOTONFLOAT delxl, delyl, rhsl, det, absdt, dtinv;
						PHOTONFLOAT x1l, y1l, x2l, y2l, sqlnl, ssq;

						delxl = readonly->delx[l];
						delyl = readonly->dely[l];
						rhsl  = readonly->rhs[l];

						/* compute intersection points */

						det   = ex*delyl - ey*delxl;
						absdt = fabs(det);
						if (absdt <= epsdet0)
							det = epsdet0;
						dtinv = 1.0/det;
						xi = dtinv * (delxl*rhse - ex*rhsl);
						yi = dtinv * (delyl*rhse - ey*rhsl);

						/* test for intersection between surface endpoints */

						x1l   = readonly->x1[l];
						y1l   = readonly->y1[l];
						x2l   = readonly->x2[l];
						y2l   = readonly->y2[l];
						sqlnl = readonly->sqln[l];
						/*printf("xi %f, x1l %f, x2l %f, yi %f, y1l %f, y2l %f ,sqlnl %f\n", xi, x1l, x2l, yi, y1l, y2l, sqlnl);*/
						ssq  = (xi - x1l)*(xi - x1l) + (xi - x2l)*(xi - x2l)
							+ (yi - y1l)*(yi - y1l) + (yi - y2l)*(yi - y2l);
						/*printf("ssq %f, sqlnl %f l %d, li %d\n", ssq, sqlnl, l, li);
						printf("delxl %f, delyl %f, ex %f, ey %f, rhse %f, l %d, le %d\n", delxl, delyl, ex, ey, rhse,l, le );*/
						if (ssq <= sqlnl)  {
							char c;
							li = l;
							/*printf("ssq %f, sqlnl %f l %d, li %d\n", ssq, sqlnl, l, li);
							printf("delxl %f, delyl %f, ex %f, ey %f, rhse %f, l %d, le %d\n", delxl, delyl, ex, ey, rhse, l, le );
							printf("Break condition\n");
							c=getc(stdin);
							if (c=='q') exit(0);*/
							break;
						}
					}
				}
				/* end of loop over surfaces */

/*                           ABSREF                            */
/***************************************************************/
/*                                                             */
/*                 absref disposition:                         */
/*                                                             */
/*            1.0 -                                            */
/*                | ===> absorption in this interval (e++)     */
/*           rhos -                                            */
/*                | ===> specular reflection in this interval  */
/*  rani     rhod -                                            */
/*                | ===> diffuse reflection in this interval   */
/*            tau -                                            */
/*                | ===> transmission in this interval (t++)   */
/*            0.0 -                                            */
/*                                                             */
/***************************************************************/

				if (li != le) {
					PHOTONFLOAT rani;
					int ix;

					/* intersection found */
					iseedrn = iran31(iseedrn);
					ix   = iseedrn - 1;
					rani = (PHOTONFLOAT)(ix)*scldenom;

					/* test for absorbed photon */

					if (rani >= rhos0) {
						writeable->icounte[le0][li] += 1;
						idead = 1;
					}

					/* test for specularly reflected photon, and reemit */

					else if (rani >= rhod0) {
						PHOTONFLOAT exold, eyold, spec1c2, spec2c2;

						exold  = ex;
						eyold  = ey;
						xe     = xi;
						ye     = yi;
						spec1c2= readonly->spec1[li];
						spec2c2= readonly->spec2[li];
						ex     = spec1c2*exold + spec2c2*eyold;
						ey     = spec2c2*exold - spec1c2*eyold;
						le     = li;
					}

					/* test for diffusely reflected photon */

					else if (rani >= tau0) {
						xe = xi;
						ye = yi;
						le = li;
						iseedphi = iran31(iseedphi);
						iseedth = iran31(iseedth);
						ixphi = iseedphi - 1;
						ixth = iseedth - 1;
						iphi = ifix((PHOTONFLOAT)(ixphi)*scalephi);
						ith = ifix((PHOTONFLOAT)(ixth )*scaletha);
						exloc = readonly->sintab[ith] * readonly->costab[iphi];
						eyloc = readonly->costab[ith];
						ex = exloc*readonly->eny[le] + eyloc*readonly->enx[le];
						ey = eyloc*readonly->eny[le] - exloc*readonly->enx[le];
					}

					/* transmitted photon identified */

					else {
						writeable->icountt[le0][li] += 1;
						idead = 1;
					}
				}
				else {
					/* no intersection found --- photon is lost */
					idead = 1;
				}
			} /* while (! idead) */
		} /* for (i = 0; i < NPP; i++) */
	} /* for (le0 = 0; le0 < NSM; le0++) */

#ifdef DO_TIME
	tget(finish);
	writeable->seconds += tsec(finish, start);
#endif
}


void
init_readonly(READONLY *readonly)
{
	int i, l;

	/*printf("init_readonly addr %p\n", readonly);*/

	get_ngon(readonly->x1,readonly->y1,readonly->x2,readonly->y2);

	for (l = 0; l < NSM; l++) {
		PHOTONFLOAT dxl, dyl, scale;
		dxl = readonly->x2[l] - readonly->x1[l];
		dyl = readonly->y2[l] - readonly->y1[l];
		readonly->delx[l] = dxl;
		readonly->dely[l] = dyl;
		readonly->sqln[l] = dxl*dxl + dyl*dyl;
		scale = 1.0/sqrt(readonly->sqln[l]);
		readonly->enx[l] =  dyl*scale;
		readonly->eny[l] = -dxl*scale;
		readonly->rhs[l] = dxl*readonly->y1[l] - dyl*readonly->x1[l];
		readonly->spec1[l] = readonly->eny[l]*readonly->eny[l] -
			readonly->enx[l]*readonly->enx[l];
		readonly->spec2[l] = -2.0*readonly->eny[l]*readonly->enx[l];
	}

	for (i = 0; i < LENSIN; i++)
		readonly->sintab[i] = sin(((PHOTONFLOAT)(i) + .5)*pi0/(2.0*(PHOTONFLOAT)(lensin0)));

	for (i = 0; i < LENCOS; i++)
		readonly->costab[i] = cos(((PHOTONFLOAT)(i) + .5)*pi0*2.0/(PHOTONFLOAT)(lencos0));
}


void
init_writeable(WRITEABLE *writeable)
{
	register WRITEABLE *lw = writeable;
	int le, li;

	/*printf("init_writeable addr %p\n", lw);*/

	lw->nevents = 0;
	lw->seconds = 0.0;
	for (le = 0; le < NSM; le++)
		for (li = 0; li < NSM; li++) {
			lw->icounte[le][li] = 0;
			lw->icountt[le][li] = 0;
		}
}


void
collect_writeable(WRITEABLE *out, WRITEABLE *in)
{
	int le, li;

	out->nevents += in->nevents;
	out->seconds += in->seconds;
	for (le = 0; le < NSM; le++)
		for (li = 0; li < NSM; li++) {
			out->icounte[le][li] += in->icounte[le][li];
			out->icountt[le][li] += in->icountt[le][li];
		}
}


void
print_writeable(FILE *fp, char *header, WRITEABLE *writeable)
{
	register WRITEABLE *lw;
	double evperpht;
	double perphtn;
	int le, li;
	long iesum, itsum, itotal;
	long irowsume[NSM], irowsumt[NSM];

	if (writeable == NULL) return;
	lw = writeable;
	iesum = 0;
	itsum = 0;
	for (le = 0; le < NSM; le++) {
		irowsume[le]= 0;
		irowsumt[le]= 0;
		for (li = 0; li < NSM; li++) {
			irowsume[le] += lw->icounte[le][li];
			irowsumt[le] += lw->icountt[le][li];
			iesum += lw->icounte[le][li];
			itsum += lw->icountt[le][li];
		}
	}
	itotal   = iesum + itsum;
	evperpht = (double)(lw->nevents)/(double)itotal;
	perphtn  = lw->seconds/(double)itotal;

	fprintf(fp, "%s\n", header);

	if (nsm < 10) {
		fprintf(fp, " irowsume =");
		for (le = 0; le < NSM; le++)
			fprintf(fp, " %d", irowsume[le]);
		fprintf(fp, "\n");
		if (itsum > 0) {
			fprintf(fp, " irowsumt =");
			for (le = 0; le < NSM; le++)
				fprintf(fp, " %d", irowsumt[le]);
			fprintf(fp, "\n");
		}

		fprintf(fp, " icounte =");
		for (le = 0; le < NSM; le++)
			for (li = 0; li < NSM; li++)
				fprintf(fp, " %d", lw->icounte[le][li]);
		fprintf(fp, "\n");
		if (itsum > 0) {
			fprintf(fp, " icountt =");
			for (le = 0; le < NSM; le++)
				for (li = 0; li < NSM; li++)
					fprintf(fp, " %d", lw->icountt[le][li]);
			fprintf(fp, "\n");
		}
	}

	fprintf(fp, " number of events                = %ld\n", lw->nevents);
	fprintf(fp, " number of events/photon         = %f\n", evperpht);
	fprintf(fp, " number of photons absorbed      = %ld\n", iesum);
	fprintf(fp, " number of photons transmitted   = %ld\n", itsum);
	fprintf(fp, " total number of photons counted = %ld\n", itotal);
	fprintf(fp, " CPU time (sec)                  = %f\n", lw->seconds);
	fprintf(fp, " CPU time/photon (sec)           = %e\n", perphtn);
	fflush(fp);
}


void
print_initial(FILE *fp, char *header)
{
	fprintf(fp, "%s\n", header);
	fprintf(fp, " nseq0   = %d\n", nseq0);
#ifdef ntasks0
	fprintf(fp, " ntasks0 = %d\n", ntasks0);
#endif
	fprintf(fp, " ns0     = %d\n", ns0);
#ifdef npp0
	fprintf(fp, " npp0    = %d\n", npp0);
#endif
	fprintf(fp, " epsdet0 = %f\n", epsdet0);
	fprintf(fp, " tau0    = %f\n", tau0);
	fprintf(fp, " rhod0   = %f\n", rhod0);
	fprintf(fp, " rhos0   = %f\n", rhos0);
	fprintf(fp, " lensin0 = %d\n", lensin0);
	fprintf(fp, " lencos0 = %d\n", lencos0);
	fflush(fp);
}


void
get_ngon(PHOTONFLOAT x1[], PHOTONFLOAT y1[], PHOTONFLOAT x2[], PHOTONFLOAT y2[])
{
	int ns, l;
	PHOTONFLOAT scale, dr, thl;

	/* construct outer polygon (counter clockwise) */

	ns    = nsm+1;
	scale = 1.;
	dr    = -2.*3.14159265358/(PHOTONFLOAT)(ns);

	x1[0] = scale*1.;
	y1[0] = scale*0.;
	for (l = 1; l < NSM; l++) {
		thl = (PHOTONFLOAT)(l)*dr;
		x1[l] = scale*cos(thl);
		y1[l] = scale*sin(thl);
		x2[l-1] = x1[l];
		y2[l-1] = y1[l];
	}
	x2[nsm] = x1[0];
	y2[nsm] = y1[0];
}


/* returns a random integer in {1,2,...,2147483646 = 2^31 - 2} */
/* -- see Park & Miller, Communications, ACM Nov, 1988 */

#define a_right 16807
#define q_31    127773
#define r_31    2836
#define m_31    2147483647

int
iran31(int oldseed)
{
	int hi, lo, seed;

	hi = oldseed/q_31;
	lo = oldseed - (hi*q_31);
	seed = a_right*lo - r_31*hi;
	if (seed < 0)
		seed = seed + m_31;
	return seed;
}
