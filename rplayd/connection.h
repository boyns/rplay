/* $Id: connection.h,v 1.3 1999/03/10 07:58:03 boyns Exp $ */

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



#ifndef _connection_h
#define _connection_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include "sound.h"
#include "server.h"
#include "buffer.h"
#include "spool.h"

/*
 * event types
 */
#define EVENT_NULL			0
#define EVENT_READ_COMMAND	        (1<<0)	/* read an RPTP command (client) */
#define EVENT_CONNECT			(1<<1)	/* connect to a server (server) */
#define EVENT_READ_CONNECT_REPLY	(1<<2)	/* read a connect reply (server) */
#define EVENT_WRITE_FIND		(1<<3)	/* write a find command (server) */
#define EVENT_READ_FIND_REPLY		(1<<4)	/* read a find reply (server) */
#define EVENT_WRITE_GET			(1<<5)	/* write a get command (server) */
#define EVENT_READ_GET_REPLY		(1<<6)	/* read a get reply (server) */
#define EVENT_READ_SOUND		(1<<7)	/* read a sound (both) */
#define EVENT_WRITE			(1<<8)	/* write buffers (client) */
#define EVENT_WRITE_SOUND		(1<<9)	/* write a sound (client) */
#define EVENT_WRITE_TIMEOUT		(1<<10)	/* write connection timeout message (both) */
#define EVENT_NOTIFY			(1<<11)	/* event notification (client) */
#define EVENT_READ_FLOW			(1<<12)	/* read a flow */
#define EVENT_WAIT_FLOW			(1<<13)	/* pause the incoming flow data */
#define EVENT_PIPE_FLOW			(1<<14)	/* piped flow data */
#define EVENT_WRITE_MONITOR		(1<<15)	/* write audio monitor data */
#define EVENT_WAIT_MONITOR		(1<<16)	/* write audio monitor data */

/*
 * notify events
 */
#define NOTIFY_NONE			0
#define NOTIFY_VOLUME			(1<<0)
#define NOTIFY_PLAY			(1<<1)
#define NOTIFY_PAUSE			(1<<2)
#define NOTIFY_CONTINUE			(1<<3)
#define NOTIFY_STOP			(1<<4)
#define NOTIFY_DONE			(1<<5)
#define NOTIFY_SKIP			(1<<6)
#define NOTIFY_STATE			(1<<7)
#define NOTIFY_FLOW			(1<<8)
#define NOTIFY_MODIFY			(1<<9)
#define NOTIFY_LEVEL			(1<<10)
#define NOTIFY_POSITION			(1<<11)
#define NOTIFY_MONITOR			(1<<12)
#define NOTIFY_ANY			(0xffffffff)
#define NOTIFY_SPOOL (NOTIFY_PLAY|NOTIFY_PAUSE|NOTIFY_CONTINUE|NOTIFY_STOP|NOTIFY_DONE|NOTIFY_SKIP|NOTIFY_MODIFY|NOTIFY_FLOW)

/*
 * connection types
 */
#define CONNECTION_NULL			0
#define CONNECTION_CLIENT		1
#define CONNECTION_SERVER		2

typedef struct _event
{
    struct _event *next;
    int type;
    int nbytes;
    int byte_offset;
    int nleft;
    char *start;
    char *ptr;
    BUFFER *buffer;
    SOUND *sound;
    int fd;
    int nconnects;
    time_t time;
    int success;
    int id;
    int wait_mask;
    SINDEX *si;
}
EVENT;

#define NOTIFY_RATE_LEVEL	0
#define NOTIFY_RATE_POSITION	1
#define NOTIFY_RATE_MAX		2

typedef struct
{
    double rate;		/* event notify-rate */
    double next;		/* next notify time */
}
NOTIFY_RATE;

typedef struct _connection
{
    struct _connection *next;
    struct _connection *prev;
    struct sockaddr_in sin;
    int type;
    int fd;
    EVENT *event;
    EVENT **ep;
    SERVER *server;
    time_t time;
    char *application;
    int notify_mask;
    int notify_id;
    NOTIFY_RATE notify_rate[NOTIFY_RATE_MAX];
    int monitor;
}
CONNECTION;

extern CONNECTION *connections;
extern int nconnections;
extern int connection_want_level_notify;
extern int connection_level_notify;

#ifdef __STDC__
extern CONNECTION *connection_create (int type);
extern void connection_destroy (CONNECTION *c);
extern void connection_client_open (int fd);
extern void connection_update (fd_set * read_fds, fd_set * write_fds);
extern void connection_close (CONNECTION *c);
extern void connection_cleanup ();
extern void connection_update_fdset (CONNECTION *c);
extern void connection_reply (CONNECTION *c, char *fmt,...);
extern CONNECTION *connection_server_open (SERVER *server, SOUND *sound);
extern void connection_server_reopen (CONNECTION *c);
extern void connection_server_ping (CONNECTION *c);
extern void connection_check_timeout ();
extern void connection_timeout (CONNECTION *c);
extern void connection_server_forward_sound (CONNECTION *c, SOUND *s);
extern void connection_server_foward (CONNECTION *c);
extern BUFFER *connection_list_create ();
extern void connection_notify (CONNECTION *c, int wait_event,...);
extern void connection_flow_pause (SPOOL *sp);
extern void connection_flow_continue (SPOOL *sp);
extern int connection_idle ();
extern EVENT *event_create (int event_type,...);
extern void event_first (CONNECTION *c, EVENT *e);
extern void event_insert_before (CONNECTION *c, EVENT *e, int mask);
extern void event_insert (CONNECTION *c, EVENT *e);
extern void event_delete (CONNECTION *c, int replace);
extern void event_update (CONNECTION *c);
extern void event_destroy (EVENT *e);
extern void event_replace (CONNECTION *c, EVENT *e);
#else
extern CONNECTION *connection_create ( /* int type */ );
extern void connection_destroy ( /* CONNECTION *c */ );
extern void connection_client_open ( /* int fd */ );
extern void connection_update ( /* fd_set *read_fds, fd_set *write_fds */ );
extern void connection_close ( /* CONNECTION *c */ );
extern void connection_cleanup ();
extern void connection_update_fdset ( /* CONNECTION *c */ );
extern void connection_reply ( /* CONNECTION *c, char *fmt, ... */ );
extern CONNECTION *connection_server_open ( /* SERVER *server, SOUND *sound */ );
extern void connection_server_ping ( /* CONNECTION *c */ );
extern void connection_check_timeout ();
extern void connection_timeout ( /* CONNECTION *c */ );
extern void connection_server_forward_sound ( /* CONNECTION *c, SOUND *s */ );
extern void connection_server_foward ( /* CONNECTION *c */ );
extern BUFFER *connection_list_create ();
extern void connection_notify ( /* CONNECTION *c, int wait_event, ... */ );
extern void connection_flow_pause ( /* SPOOL *sp */ );
extern void connection_flow_continue ( /* SPOOL *sp */ );
extern int connection_idle ();
extern EVENT *event_create ( /* int event_type, ... */ );
extern void event_first ( /* CONNECTION *c, EVENT *e */ );
extern void event_insert_before ( /* CONNECTION *c, EVENT *e, int mask */ );
extern void event_insert ( /* CONNECTION *c, EVENT *e */ );
extern void event_delete ( /* CONNECTION *c, int replace */ );
extern void event_update ( /* CONNECTION *c */ );
extern void event_destroy ( /* EVENT *e */ );
extern void event_replace ( /* CONNECTION *c, EVENT *e */ );
#endif

#endif /* _connection_h */
