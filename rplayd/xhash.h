/* $Id: xhash.h,v 1.3 1999/03/10 07:58:05 boyns Exp $ */

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



#ifndef _xhash_h
#define _xhash_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __STDC__
extern void xhash_init (int hash_table_size);
extern char *xhash_get (char *hash_key);
extern void xhash_put (char *hash_key, char *data);
extern void xhash_replace (char *hash_key, char *data);
extern void xhash_delete (char *hash_key);
extern char *xhash_name (char *pathname);
extern void xhash_apply (char *(*func) ());
extern void xhash_die ();
#else
extern void xhash_init ( /* int hash_table_size */ );
extern char *xhash_get ( /* char *hash_key */ );
extern void xhash_put ( /* char *hash_key, char *data */ );
extern void xhash_replace ( /* char *hash_key, char *data */ );
extern void xhash_delete ( /* char *hash_key */ );
extern char *xhash_name ( /* char *pathname */ );
extern void xhash_apply ( /* char *(*func) () */ );
extern void xhash_die ();
#endif

#endif /* _xhash_h */
