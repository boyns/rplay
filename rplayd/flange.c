/* flange.c - */

/*
 * Copyright (C) 1993-98 Mark R. Boyns <boyns@doit.org>
 *
 * This file is part of rplay.
 *
 * rplay is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * rplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rplay; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */



#include "rplayd.h"

#ifdef TEST_FLANGE

typedef struct
{
    float rate;			/* rate of delay change (-1.0 - 1.0) */
    float sweep;		/* sweep range in milliseconds */
    float delay;		/* fixed additional delay in milliseconds */
    float dry_mix;		/* mix of unaffected signal (-1.0 - 1.0) */
    float wet_mix;		/* mix of affected signal (-1.0 - 1.0) */
    float feedback;		/* amount of recirculation (-1.0 - 1.0) */
    float depth;
}
parameters;

parameters p =
{2.0, 20.0, 0.0, 0.5, 0.5, 0.0, 10};

#define	SPMS	8
#define	SPS		8000
#define	BFSZ	1024

float Buf[BFSZ];

typedef union
{
    unsigned char b[2];
    unsigned short w;
}
bw;

typedef union
{
    unsigned short w[2];
    long l;
}
wl;

void
flange (char *native_buf, int nsamples, int nchannels)
{
    short *sample = (short *) native_buf;
    short *flanged = (short *) native_buf;
    int size = nsamples * nchannels;
    static int first = 1;

    static int i, fp, ep1, ep2;
    static int step, depth, delay, min_sweep, max_sweep;
    static float inval, outval, ifac = 65536.0;
    static wl sweep;

    if (first)
    {
	first = 0;

	step = (int) (p.rate * 65.536);
	depth = (int) (p.depth * (float) SPMS);
	delay = (int) (p.delay * (float) SPMS);
	max_sweep = BFSZ - 2 - delay;
	min_sweep = max_sweep - depth;
	if (min_sweep < 0)
	{
	    exit (1);
	}

	sweep.w[0] = (min_sweep + max_sweep) / 2;
	sweep.w[1] = 0;
	fp = ep1 = ep2 = 0;
    }

#if 0
    printf ("step = %d, depth = %d, delay = %d\n", step, depth, delay);
    printf ("max_sweep = %d, min_sweep = %d\n", max_sweep, min_sweep);
#endif

    for (i = 0; i < size; i++)
    {
	outval = (Buf[ep1] * sweep.w[1] + Buf[ep2] * (ifac - sweep.w[1])) / ifac;
	Buf[fp] = (inval = (float) sample[i]) + outval * p.feedback;
	outval = outval * p.wet_mix + inval * p.dry_mix;
	if (outval > 32000)
	    flanged[i] = 32000;
	else if (outval < -32000)
	    flanged[i] = -32000;
	else
	    flanged[i] = (int) outval;

	fp = (fp + 1) & (BFSZ - 1);
	sweep.l += step;
	ep1 = (fp + sweep.w[0]) & (BFSZ - 1);
	ep2 = (ep1 - 1) & (BFSZ - 1);
	if (sweep.w[0] > max_sweep)
	    step = -step;
	else if (sweep.w[0] < min_sweep)
	    step = -step;
    }
}

#endif /* TEST_FLANGE */
