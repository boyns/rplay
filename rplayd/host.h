/* $Id: host.h,v 1.3 1999/03/10 07:58:03 boyns Exp $ */

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



#ifndef _host_h
#define _host_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "buffer.h"

#ifdef AUTH

#define HOST_EXPR_SIZE		1024
#define HOST_READ		'r'
#define HOST_WRITE		'w'
#define HOST_EXECUTE		'x'
#define HOST_MONITOR		'm'
#define HOST_DEFAULT_ACCESS	"rx"

extern BUFFER *host_list;

#ifdef __STDC__
extern void host_read (char *filename);
extern void host_reread (char *filename);
extern void host_stat (char *filename);
extern char *host_ip_to_regex (char *ip_addr);
extern int host_access (struct sockaddr_in sin, char access_mode);
extern void host_insert (char *expr_read, char *expr_write, char *expr_execute,
			 char *expr_monitor, char *name, char *perms);
#else
extern void host_read ( /* char *filename */ );
extern void host_reread ( /* char *filename */ );
extern void host_stat ( /* char *filename */ );
extern char *host_ip_to_regex ( /* char *ip_addr */ );
extern int host_access ( /* struct sockaddr_in sin, char access_mode */ );
extern void host_insert ();
#endif

#endif /* AUTH */

#endif /* _host_h */
