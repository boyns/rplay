/* $Id: buffer.h,v 1.2 1998/08/13 06:13:41 boyns Exp $ */

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



#ifndef _buffer_h
#define _buffer_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define BUFFER_SIZE	8192	/* must be >= RPTP_MAX_LINE, divisible by 2 & 4 */

/*
 * Maximum number of buffers to have on the free list.
 * The number should really depend on the value of BUFFER_SIZE.
 */
#define BUFFER_MAX_FREE	64

#define BUFFER_FREE	0	/* temporary buffer - put on free list or destroyed */
#define BUFFER_KEEP	1	/* permanent buffer - not put on free list */
#define BUFFER_REUSE	2	/* temporary buffer - always put on free list */

typedef struct _buffer
{
    struct _buffer *next;
    int id;
    int nbytes;
    char buf[BUFFER_SIZE];
    int status;
    int offset;
}
BUFFER;

#ifdef __STDC__
BUFFER *buffer_create (void);
void buffer_destroy (BUFFER *b);
BUFFER *buffer_alloc (int nbytes, int type);
void buffer_dealloc (BUFFER *b, int force);
void buffer_cleanup (void);
#else
BUFFER *buffer_create ();
void buffer_destroy ( /* BUFFER *b */ );
BUFFER *buffer_alloc ( /* int nbytes, int type */ );
void buffer_dealloc ( /* BUFFER *b, int force */ );
void buffer_cleanup ();
#endif

#endif /* _buffer_h */
