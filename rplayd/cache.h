/* cache.h - Disk cache definitions.  */

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



#ifndef _cache_h
#define _cache_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern int cache_max_size;
extern int cache_remove;

#ifdef __STDC__
extern void cache_init (char *dir_name);
extern char *cache_first ();
extern char *cache_next ();
extern int cache_size ();
extern void cache_read ();
extern char *cache_name (char *sound);
extern int cache_free (int size);
extern int cache_create (char *name, int size);
extern void cache_cleanup ();
#else
extern void cache_init ( /* char *dir_name */ );
extern char *cache_first ();
extern char *cache_next ();
extern int cache_size ();
extern void cache_read ();
extern char *cache_name ( /* char *sound */ );
extern int cache_free ( /* int size */ );
extern int cache_create ( /* char *name, int size */ );
extern void cache_cleanup ();
#endif

#endif /* _cache_h */
