/* $Id: native.h,v 1.2 1998/08/13 06:14:00 boyns Exp $ */

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



#ifndef _native_h
#define _native_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "spool.h"

#ifdef __STDC__
typedef int (*NATIVE_FUNC) (char *, int, int);
#else
typedef int (*NATIVE_FUNC) ();
#endif
    
typedef struct
{
    NATIVE_FUNC to_native[2];
    NATIVE_FUNC from_native[2];
}
NATIVE_TABLE;
extern NATIVE_TABLE native_table[];

extern void zero_native ( /* char *native_buf, int nsamples, int nchannels */ );
extern void level ( /* char *native_buf, int nsamples, int nchannels */ );

#ifdef FAKE_VOLUME
extern void fake_volume ( /* char *native_buf, int nsamples, int nchannels */ );
#endif /* FAKE_VOLUME */

#endif /* _native_h */
