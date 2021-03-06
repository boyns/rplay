/* $Id: strdup.c,v 1.4 1999/03/10 07:57:53 boyns Exp $ */

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
#include "strdup.h"
#include <stdio.h>

#ifndef HAVE_STRDUP
#ifdef __STDC__
char *
strdup(char *str)
#else
char *
strdup(str)
    char *str;
#endif
{
    char *p;

    p = (char *) malloc(strlen(str) + 1);
    if (p == NULL)
    {
	return NULL;
    }
    else
    {
	strcpy(p, str);
	return p;
    }
}
#endif /* HAVE_STRDUP */
