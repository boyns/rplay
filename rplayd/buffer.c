/* $Id: buffer.c,v 1.4 1999/03/10 07:58:02 boyns Exp $ */

/*
 * Copyright (C) 1993-99 Mark R. Boyns <boyns@doit.org>
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



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include "rplayd.h"
#include "buffer.h"

static BUFFER *buffers = NULL;
static int nbuffers = 0;	/* number of buffers on the free list */
static int next_id = 0;		/* next buffer id number */

#ifdef __STDC__
BUFFER *
buffer_create(void)
#else
BUFFER *
buffer_create()
#endif
{
    BUFFER *b;

    if (buffers == NULL)
    {
	b = (BUFFER *) malloc(sizeof(BUFFER));
	if (b == NULL)
	{
	    report(REPORT_ERROR, "buffer_create: out of memory\n");
	    done(1);
	}
	b->id = ++next_id;
    }
    else
    {
	nbuffers--;
	b = buffers;
	buffers = buffers->next;
    }

    b->nbytes = 0;
    b->buf[0] = '\0';
    b->status = BUFFER_FREE;
    b->offset = 0;
    b->next = NULL;

    return b;
}

#ifdef __STDC__
void
buffer_destroy(BUFFER *b)
#else
void
buffer_destroy(b)
    BUFFER *b;
#endif
{
    if (b == NULL || b->status == BUFFER_KEEP)
    {
	/* Do nothing. */
	return;
    }
    else if (b->status == BUFFER_REUSE)
    {
	/* Always put REUSE buffers on the free list.  This will
	   probably cause rplayd to grow bigger, but it prevents
	   problems with calling free() inside the timer
	   interrupt. */
	nbuffers++;
	b->next = buffers;
	buffers = b;
    }
    else
    {
	/* Free the buffer. */
	free((char *) b);
    }
}

/*
 * Allocate enough buffers to hold nbytes.
 */
#ifdef __STDC__
BUFFER *
buffer_alloc(int nbytes, int type)
#else
BUFFER *
buffer_alloc(nbytes, type)
    int nbytes;
    int type;
#endif
{
    BUFFER *head, **next = &head;

    do
    {
	*next = buffer_create();
	nbytes -= BUFFER_SIZE;
	(*next)->status = type;
	next = &(*next)->next;
    }
    while (nbytes > 0);

    *next = NULL;

    return head;
}

#ifdef __STDC__
void
buffer_dealloc(BUFFER *b, int force)
#else
void
buffer_dealloc(b, force)
    BUFFER *b;
    int force;
#endif
{
    BUFFER *next;

    while (b)
    {
	next = b->next;
	if (force)
	{
	    b->status = BUFFER_FREE;
	}
	buffer_destroy(b);
	b = next;
    }
}

#ifdef __STDC__
void
buffer_cleanup(void)
#else
void
buffer_cleanup()
#endif
{
    BUFFER *b;

    report(REPORT_DEBUG, "cleaning up buffers - %d bytes\n", nbuffers * BUFFER_SIZE);

    while (buffers)
    {
	b = buffers;
	buffers = buffers->next;
	free((char *) b);
    }

    buffers = NULL;
    nbuffers = 0;
    next_id = 0;
}

#ifdef __STDC__
int
buffer_nbytes(BUFFER *b)
#else
int
buffer_nbytes(b)
    BUFFER *b;
#endif
{
    int n = 0;
    
    while (b)
    {
	n += b->nbytes;
	b = b->next;
    }

    return n;
}
