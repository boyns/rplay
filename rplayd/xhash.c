/* $Id: xhash.c,v 1.4 1999/03/10 07:58:05 boyns Exp $ */

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
#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "xhash.h"
#include "hash.h"
#include "rplayd.h"

static struct hash_control *htable;

#ifdef DEBUG
static void
xhash_stats()
{
    int statbuf[HASH_STATLENGTH];
    hash_say(htable, statbuf, HASH_STATLENGTH);
    printf(" %d size, %d read, %d write, %d collisions, %d used\n",
	   statbuf[1], statbuf[2], statbuf[3], statbuf[4], statbuf[5]);
}
#endif

#ifdef __STDC__
void
xhash_init(int hash_table_size)
#else
void
xhash_init(hash_table_size)
    int hash_table_size;
#endif
{
    htable = hash_new();
}

#ifdef __STDC__
char *
xhash_get(char *hash_key)
#else
char *
xhash_get(hash_key)
    char *hash_key;
#endif
{
    return (char *) hash_find(htable, hash_key);
}

#ifdef __STDC__
void
xhash_put(char *hash_key, char *data)
#else
void
xhash_put(hash_key, data)
    char *hash_key;
    char *data;
#endif
{
    hash_insert(htable, hash_key, data);
}

#ifdef __STDC__
void
xhash_replace(char *hash_key, char *data)
#else
void
xhash_replace(hash_key, data)
    char *hash_key;
    char *data;
#endif
{
    hash_replace(htable, hash_key, data);
}

#ifdef __STDC__
void
xhash_delete(char *hash_key)
#else
void
xhash_delete(hash_key)
    char *hash_key;
#endif
{
    hash_delete(htable, hash_key);
}

/*
 * create a hash key name for the given sound
 */
#ifdef __STDC__
char *
xhash_name(char *pathname)
#else
char *
xhash_name(pathname)
    char *pathname;
#endif
{
    static char name[MAXPATHLEN];
    char *extension;

    if (pathname[0] == '/')
    {
	pathname = strrchr(pathname, '/') + 1;
    }

    strncpy(name, pathname, sizeof(name));
    extension = strrchr(name, '.');
    if (extension)
    {
	*extension = '\0';
    }

    return name;
}

#ifdef __STDC__
void
xhash_apply(char *(*func) ())
#else
void
xhash_apply(func)
    char *(*func) ();
#endif
{
    hash_apply(htable, func);
}

void
xhash_die()
{
    hash_die(htable);
}
