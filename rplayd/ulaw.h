/* $Id: ulaw.h,v 1.2 1998/08/13 06:14:12 boyns Exp $ */

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



#ifndef _ulaw_h
#define _ulaw_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * ulaw formats
 */
#define ULAW_MULAW_8		1	/* 8-bit ISDN u-law (CCITT G.711) */
#define ULAW_LINEAR_8		2	/* 8-bit linear PCM */
#define ULAW_LINEAR_16		3	/* 16-bit linear PCM */
#define ULAW_LINEAR_24		4	/* 24-bit linear PCM */
#define ULAW_LINEAR_32		5	/* 32-bit linear PCM */
#define ULAW_LINEAR_FLOAT	6	/* 32-bit IEEE floating point */
#define ULAW_LINEAR_DOUBLE	7	/* 64-bit IEEE floating point */
#define ULAW_G721		23	/* CCITT G.721 4-bits ADPCM */
#define ULAW_G723_3		25	/* CCITT G.723 3-bits ADPCM */
#define ULAW_G723_5		26	/* CCITT G.723 5-bits ADPCM */
#define ULAW_MAGIC		0x2e736e64	/* ".snd" ulaw file header */
#define ULAW_HDRSIZE		24	/* minimum ulaw header size */

extern short _ulaw2linear[];
extern unsigned char *_linear2ulaw;

#define ulaw_to_linear(ulaw8) (_ulaw2linear[(unsigned char)(ulaw8)])
#define linear_to_ulaw(lin16) (_linear2ulaw[(short)(lin16) >> 3])

#if 0
#ifdef __STDC__
extern unsigned char st_linear_to_ulaw (int sample);
extern int st_ulaw_to_linear (unsigned char ulawbyte);
#else
extern unsigned char st_linear_to_ulaw ( /* int sample */ );
extern int st_ulaw_to_linear ( /* unsigned char ulawbyte */ );
#endif
#endif

#endif /* _ulaw_h */
