/* $Id: server.h,v 1.3 1999/03/10 07:58:04 boyns Exp $ */

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



#ifndef _server_h
#define _server_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "buffer.h"

typedef struct _server
{
    struct _server *next;
    struct sockaddr_in sin;
}
SERVER;

extern BUFFER *server_list;
extern SERVER *servers;

#ifdef __STDC__
extern void server_read (char *filename);
extern void server_reread (char *filename);
extern void server_stat (char *filename);
#else
extern void server_read ( /* char *filename */ );
extern void server_reread ( /* char *filename */ );
extern void server_stat ( /* char *filename */ );
#endif

#endif /* _server_h */
