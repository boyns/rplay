/* $Id: xmalloc.c,v 1.2 1998/08/13 06:13:34 boyns Exp $ */

/* 
 * Copyright (C) 1994 Mark Boyns
 *
 * This file is part of rplay
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ansidecl.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef __STDC__
#include <stddef.h>
#else
#define size_t unsigned long
#endif

#ifdef __STDC__
PTR
xmalloc (size_t size)
#else
PTR
xmalloc (size)
    size_t size;
#endif
{
    PTR p;

    p = (PTR) malloc (size);
    if (p == 0)
    {
	fprintf (stderr, "xmalloc: Virtual memory exhausted.\n");
	exit (1);
    }
    return p;
}

#ifdef __STDC__
PTR
xrealloc (PTR oldmem, size_t size)
#else
PTR
xrealloc (oldmem, size)
    PTR oldmem;
    size_t size;
#endif
{
    PTR newmem;

    if (size == 0)
    {
	size = 1;
    }

    newmem = (oldmem) ? (PTR) realloc (oldmem, size) : (PTR) malloc (size);
    
    if (!newmem)
    {
	fprintf (stderr, "xrealloc: Virtual memory exhausted.\n");
	exit (1);
    }

    return newmem;
}

