/* $Id: connection.c,v 1.6 1999/03/10 07:58:03 boyns Exp $ */

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
#include <sys/time.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "rplayd.h"
#include "connection.h"
#include "command.h"
#include "version.h"
#include "sound.h"
#include "server.h"
#include "host.h"
#include "cache.h"
#include "rplay.h"
#include "spool.h"
#include "timer.h"
#include "buffer.h"
#include "strdup.h"

CONNECTION *connections = NULL;
int nconnections = 0;
int connection_want_level_notify = 0;
int connection_level_notify = 0;

#undef EVENT_DEBUG
#ifdef EVENT_DEBUG
static void event_print(CONNECTION *c, char *message);
#endif

/*
 * create a connection object
 * types are CONNECTION_CLIENT and CONNECTION_SERVER
 */
#ifdef __STDC__
CONNECTION *
connection_create(int type)
#else
CONNECTION *
connection_create(type)
    int type;
#endif
{
    CONNECTION *c;
    int i;

    c = (CONNECTION *) malloc(sizeof(CONNECTION));
    if (c == NULL)
    {
	report(REPORT_ERROR, "connection_create: out of memory\n");
	done(1);
    }
    c->type = type;
    c->fd = -1;
    c->event = NULL;
    c->ep = &c->event;
    c->server = NULL;
    c->time = time(0);
    c->application = NULL;
    c->notify_mask = 0;
    c->notify_id = 0;
    for (i = 0; i < NOTIFY_RATE_MAX; i++)
    {
	c->notify_rate[i].rate = 0.0;
	c->notify_rate[i].next = 0.0;
    }
    c->monitor = 0;

    /*
     * insert new connection into the connection list
     */
    c->prev = NULL;
    c->next = connections;
    if (connections)
    {
	connections->prev = c;
    }
    connections = c;

    if (type == CONNECTION_CLIENT)
    {
	nconnections++;
    }

    return c;
}

/*
 * destroy the connection and remove it from the connection list
 */
#ifdef __STDC__
void
connection_destroy(CONNECTION *c)
#else
void
connection_destroy(c)
    CONNECTION *c;
#endif
{
    if (c->type == CONNECTION_CLIENT)
    {
	nconnections--;
    }
    if (c->prev)
    {
	c->prev->next = c->next;
    }
    else
    {
	connections = c->next;
    }
    if (c->next)
    {
	c->next->prev = c->prev;
    }
    if (c->application)
    {
	free(c->application);
    }
    free((char *) c);

    connection_want_level_notify = 0;
    for (c = connections; c; c = c->next)
    {
	if (BIT(c->notify_mask, NOTIFY_LEVEL)
	    || (c->event && BIT(c->event->wait_mask, NOTIFY_LEVEL)))
	{
	    connection_want_level_notify++;
	}
    }
}

/*
 * start a server connection to try and find the given sound
 *
 * The sound can be NULL which means that events have already
 * been created and will be forwarded to this server.
 */
#ifdef __STDC__
CONNECTION *
connection_server_open(SERVER *server, SOUND *sound)
#else
CONNECTION *
connection_server_open(server, sound)
    SERVER *server;
    SOUND *sound;
#endif
{
    int n;
    CONNECTION *c;
    EVENT *e;

    /*
     * see if there is already a connection to that server
     */
    for (c = connections; c; c = c->next)
    {
	if (c->type == CONNECTION_SERVER && c->server == server)
	{
	    break;
	}
    }
    if (c == NULL)
    {
	c = connection_create(CONNECTION_SERVER);
	c->server = server;
	c->sin = server->sin;
    }
    if (c->fd == -1)
    {
	c->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (c->fd < 0)
	{
	    report(REPORT_ERROR, "connection_open_server: socket: %s\n", sys_err_str(errno));
	    done(1);
	}
	fd_nonblock(c->fd);

	/*
	 * ping the server
	 */
	connection_server_ping(c);

	/* 
	 * try to connect to the server immediately
	 */
	report(REPORT_DEBUG, "%s server connection attempt\n", inet_ntoa(c->server->sin.sin_addr));
	n = connect(c->fd, (struct sockaddr *) &c->server->sin, sizeof(c->server->sin));
	e = event_create(EVENT_CONNECT);
	e->time = time(0);
	event_insert(c, e);
    }

    /*
     * create events to try and find the sound
     */
    if (sound)
    {
	BUFFER *b;

	b = buffer_create();
	SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "find %s\r\n", sound->name);
	b->nbytes = strlen(b->buf);
	e = event_create(EVENT_WRITE_FIND, b, sound);
	event_insert(c, e);
    }

    return c;
}

#ifdef __STDC__
void
connection_server_reopen(CONNECTION *c)
#else
void
connection_server_reopen(c)
    CONNECTION *c;
#endif
{
    EVENT *e;
    CONNECTION *server;

    close(c->fd);
    c->fd = -1;
    e = c->event;
    c->event = NULL;
    c->ep = &c->event;

    server = connection_server_open(c->server, NULL);
    if (server != c)
    {
	report(REPORT_ERROR, "connection_server_reopen: server != c\n");
	done(1);
    }
    event_insert(server, e);
}

/*
 * send a ping packet to the server
 */
#ifdef __STDC__
void
connection_server_ping(CONNECTION *c)
#else
void
connection_server_ping(c)
    CONNECTION *c;
#endif
{
    struct sockaddr_in ping_addr;

    memcpy((char *) &ping_addr, (char *) &c->server->sin, sizeof(ping_addr));
    ping_addr.sin_port = htons(RPLAY_PORT);
    rplay_ping_sockaddr_in(&ping_addr);
}

/*
 * open a client connection
 */
#ifdef __STDC__
void
connection_client_open(int sock_fd)
#else
void
connection_client_open(sock_fd)
    int sock_fd;
#endif
{
    struct sockaddr_in f;
    int fd, flen = sizeof(f);
    CONNECTION *c;
    EVENT *e;
    char line[RPTP_MAX_LINE];

    fd = accept(sock_fd, (struct sockaddr *) &f, &flen);
    if (fd < 0)
    {
	report(REPORT_ERROR, "accept: %s\n", sys_err_str(errno));
	return;
    }

#ifdef DEBUG
    report(REPORT_DEBUG, "%s client connection request\n", inet_ntoa(f.sin_addr));
#endif

#ifdef AUTH
    /*
     * see if this host has any access
     */
    if (!host_access(f, HOST_READ) && !host_access(f, HOST_WRITE)
	&& !host_access(f, HOST_WRITE))
    {
	report(REPORT_NOTICE, "%s RPTP access denied\n", inet_ntoa(f.sin_addr));
	SNPRINTF(SIZE(line, sizeof(line)), "%cerror=\"connection refused\"\r\n", RPTP_ERROR);
	write(fd, line, strlen(line));
	close(fd);
	return;
    }
#endif /* AUTH */

    /*
     * see if there are too many connections
     */
    if (nconnections + 1 == RPTP_MAX_CONNECTIONS)
    {
	SNPRINTF(SIZE(line, sizeof(line)), "%cerror=\"connection refused\"\r\n", RPTP_ERROR);
	write(fd, line, strlen(line));
	close(fd);
	return;
    }

    c = connection_create(CONNECTION_CLIENT);
    c->fd = fd;
    fd_nonblock(c->fd);
    c->sin = f;

#if 1
    report(REPORT_NOTICE, "%s client connection established\n", inet_ntoa(c->sin.sin_addr));
#endif

    /* Greet the connection with the server's status. */
    e = event_create(EVENT_WRITE, rplayd_status());
    event_insert(c, e);
}

/*
 * Update connections by monitoring the read and write fd_sets.
 *
 * If the given fd_sets are NULL then select must be called to update
 * the fd_sets.  (This should not be necessary anymore)
 */
#ifdef __STDC__
void
connection_update(fd_set * read_fds, fd_set * write_fds)
#else
void
connection_update(read_fds, write_fds)
    fd_set *read_fds;
    fd_set *write_fds;
#endif
{
    int i, n, size;
    CONNECTION *c;
    struct timeval tv;
    fd_set rfds, wfds;
    time_t t;
    char buf[RPTP_MAX_LINE];
    EVENT *e;
    SOUND *s;
    SPOOL *sp;
    fd_set cmask;

    if (read_fds == NULL && write_fds == NULL)
    {
	int nfds;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	rfds = read_mask;
	wfds = write_mask;
	FD_CLR(rplay_fd, &rfds);
#ifdef __hpux
	nfds = select(FD_SETSIZE, (int *) &rfds, (int *) &wfds, 0, &tv);
#else
	nfds = select(FD_SETSIZE, &rfds, &wfds, 0, &tv);
#endif
	if (nfds == 0)
	{
	    /* nothing to do */
	    return;
	}
	else if (nfds < 0)
	{
	    report(REPORT_ERROR, "connection_update: select: %s\n",
		   sys_err_str(errno));
	    done(1);
	}
    }
    else
    {
	rfds = *read_fds;
	wfds = *write_fds;
    }

    /*
     * Check for new RPTP connections.
     */
    if (FD_ISSET(rptp_fd, &rfds))
    {
	connection_client_open(rptp_fd);
    }
#ifdef OTHER_RPLAY_PORTS
    if (FD_ISSET(other_rptp_fd, &rfds))
    {
	connection_client_open(other_rptp_fd);
    }
#endif

    t = time(0);

    /* It's not safe to simply traverse the connection list
       since connections can be closed during this loop.
       Instead a bitmask is used to keep track of the connections
       that have been updated. (efence) */
    FD_ZERO(&cmask);
    for (;;)
    {
	for (c = connections; c; c = c->next)
	{
	    if (!FD_ISSET(c->fd, &cmask))
	    {
		break;
	    }
	}
	if (c == NULL)
	{
	    break;
	}
	FD_SET(c->fd, &cmask);

	/* connection has no events */
	if (c->event == NULL)
	{
	    continue;
	}

	/*
	 * read events
	 */
	if (FD_ISSET(c->fd, &rfds))
	{
	    c->time = t;

	    switch (c->event->type)
	    {
	    case EVENT_READ_SOUND:
	      restart:
		n = read(c->fd, c->event->buffer->buf,
		       MIN(c->event->nleft, sizeof(c->event->buffer->buf)));
		if (n < 0 && (errno == EINTR || errno == EAGAIN))
		{
		    goto restart;
		}
#ifdef DEBUG
		report(REPORT_DEBUG, "- %s read %d (%d)\n",
		       inet_ntoa(c->sin.sin_addr),
		       n, c->event->nleft);
#endif

		if (n <= 0)
		{
		    event_update(c);
		}
		else
		{
		    int r;

		  restart1:
		    r = write(c->event->fd, c->event->buffer->buf, n);
		    if (r < 0 && (errno == EINTR || errno == EAGAIN))
		    {
			goto restart1;
		    }
		    c->event->nleft -= n;
		    c->event->byte_offset += n;
		    if (c->event->nleft == 0)
		    {
			c->event->success++;
			event_update(c);
		    }
		}
		break;

	    case EVENT_READ_FLOW:
		/* Find the spool entry. */
		sp = spool_find(c->event->id);
		if (!sp)
		{
		    if (c->event->nbytes == -1)		/* single-put flow */
		    {
			event_update(c);
			break;
		    }

		    /* The spool entry no longer exists.  The rest of
		       this flow must be read -- the next `put' will
		       fail. */
		    c->event->buffer = buffer_create();
		  retry:
		    n = read(c->fd, c->event->buffer->buf,
			     MIN(BUFFER_SIZE, c->event->nleft));
		    if (n < 0 && (errno == EINTR || errno == EAGAIN))
		    {
			goto retry;
		    }
#if 0
		    report(REPORT_DEBUG, "eating %d flow bytes (%d), id=#%d\n",
			   n, c->event->nleft, c->event->id);
#endif
		    buffer_destroy(c->event->buffer);	/* Eat the flow. */
		    c->event->buffer = NULL;
		    if (n <= 0)
		    {
			event_update(c);
			break;
		    }
		    c->event->nleft -= n;
		    if (c->event->nleft <= 0)
		    {
			c->event->success++;
			event_update(c);
			break;
		    }
		    break;
		}

		/* Pause the connection if the high_water_mark has been exceeded. */
		if (sp->si
		    && ((sp->si->water_mark - sp->si->offset) >= sp->si->high_water_mark))
		{
		    connection_notify(0, NOTIFY_FLOW, sp);
		    connection_flow_pause(sp);
		    break;
		}

		/* Read data from the flow.  single-put flows will always
		   try to read BUFFER_SIZE. */
		c->event->buffer = buffer_create();

		/* calculate the number of bytes to read without
		   going over the high_water_mark. */
#if 1		
		size = MIN(BUFFER_SIZE,
			   sp->si ? sp->si->high_water_mark - (sp->si->water_mark - sp->si->offset)
			   : BUFFER_SIZE);
#else
		size = MIN(BUFFER_SIZE, c->event->nbytes == -1 ? BUFFER_SIZE : c->event->nleft);
#endif		
	redo:
		n = read(c->fd, c->event->buffer->buf, size);
		if (n < 0 && (errno == EINTR || errno == EAGAIN))
		{
		    goto redo;
		}
		if (n <= 0)
		{
		    /* End of a single-put flow */
		    if (c->event->nbytes == -1)
		    {
			spool_done(sp);
			connection_close(c);
		    }
		    else
		    {
			event_update(c);
		    }
		    break;
		}

		/* Add the buffer to the sound's flow. */
		c->event->buffer->nbytes = n;
		if (spool_flow_insert(sp, c->event->buffer) < 0)
		{
		    c->event->buffer = NULL;	/* efence */
		    break;
		}
		c->event->buffer = NULL;
		c->event->byte_offset += n;
		if (c->event->nbytes != -1)
		{
		    c->event->nleft -= n;
		}

		/* single-put flows don't terminate here */
		if (c->event->nbytes != -1 && c->event->nleft <= 0)
		{
		    c->event->success++;
		    event_update(c);
		}
		break;

	    case EVENT_NOTIFY:
		/* Replace the current notify event with a read_command. */
		e = event_create(EVENT_READ_COMMAND, buffer_create());
		event_replace(c, e);
		/* fall fall fall */

	    default:
		/* Read a command. */
	      restart2:
		n = recv(c->fd, c->event->ptr, c->event->nleft, MSG_PEEK);
		if (n < 0 && (errno == EINTR || errno == EAGAIN))
		{
		    goto restart2;
		}
#if 0
		report(REPORT_DEBUG, "- %s recv %d (%d)\n",
		       inet_ntoa(c->sin.sin_addr),
		       n, c->event->nleft);
#endif
		if (n <= 0)
		{
		    event_update(c);
		}
		else
		{
		    /*
		     * search for a newline and replace \r and \n with \0
		     */
		    for (i = 0; i < n; i++)
		    {
			if (c->event->ptr[i] == '\r')
			{
			    c->event->ptr[i] = '\0';
			}
			else if (c->event->ptr[i] == '\n')
			{
			    c->event->ptr[i] = '\0';
			    c->event->success++;
			    break;
			}
		    }
		  restart3:
		    n = read(c->fd, buf, i == n ? n : i + 1);
		    if (n < 0 && (errno == EINTR || errno == EAGAIN))
		    {
			goto restart3;
		    }
		    if (n <= 0)
		    {
			c->event->success = 0;
			event_update(c);
		    }
		    else
		    {
			c->event->ptr += n;
			c->event->nleft -= n;
			if (c->event->success || c->event->nleft == 0)
			{
			    event_update(c);
			}
		    }
		}
		break;
	    }
	}
	/*
	 * write events
	 */
	else if (FD_ISSET(c->fd, &wfds))
	{
	    c->time = t;

	    switch (c->event->type)
	    {
	    case EVENT_CONNECT:
		event_update(c);
		break;

	    default:
	again:
		n = write(c->fd, c->event->ptr, c->event->nleft);
#if 0
		report(REPORT_DEBUG, "- %s write %d (%d)\n",
		       inet_ntoa(c->sin.sin_addr), n,
		       c->event->nleft);
#endif
		if (n <= 0)
		{
		    if ((errno == EINTR || errno == EAGAIN))
		    {
			goto again;
		    }
		    event_update(c);
		}
		else
		{
		    c->event->ptr += n;
		    c->event->nleft -= n;
		    c->event->byte_offset += n;
		    if (c->event->nleft == 0)
		    {
			if (c->event->type == EVENT_WRITE_SOUND)
			{
			    n = sound_fill(c->event->si, c->event->buffer, 1);
			    if (n > 0)
			    {
				c->event->nleft = c->event->buffer->nbytes;
				c->event->ptr = c->event->buffer->buf;
				c->event->start = c->event->buffer->buf;
			    }
			    else
			    {
				sound_close(c->event->si);
				c->event->si = NULL;	/* efence */
				c->event->success++;
				event_update(c);
			    }
			}
			else
			{
			    c->event->success++;
			    event_update(c);
			}
		    }
		}
		break;
	    }
	}
    }
}

/*
 * close and destroy the connection
 */
#ifdef __STDC__
void
connection_close(CONNECTION *c)
#else
void
connection_close(c)
    CONNECTION *c;
#endif
{
    EVENT *e, *next;
    SOUND *s;
    SPOOL *sp;

    switch (c->type)
    {
    case CONNECTION_CLIENT:
	report(REPORT_NOTICE, "%s client connection closed\n", inet_ntoa(c->sin.sin_addr));
	break;

    case CONNECTION_SERVER:
	report(REPORT_NOTICE, "%s server connection closed\n", inet_ntoa(c->sin.sin_addr));
	break;
    }

    FD_CLR(c->fd, &read_mask);
    FD_CLR(c->fd, &write_mask);

    if (c->monitor)
    {
	monitor_count--;
    }

    close(c->fd);

    /*
     * destroy any events that are left
     */
    for (e = c->event; e; e = next)
    {
	next = e->next;
	switch (e->type)
	{
	case EVENT_WRITE_FIND:
	case EVENT_READ_FIND_REPLY:
	case EVENT_WRITE_GET:
	case EVENT_READ_GET_REPLY:
	case EVENT_READ_SOUND:
	    spool_remove(e->sound);
	    sound_delete(e->sound, 1);
	    break;

	case EVENT_READ_FLOW:
	case EVENT_WAIT_FLOW:
	case EVENT_PIPE_FLOW:
/* Dunno if this should happen: */
#if 0
	    sp = spool_find(e->id);
	    s = sp->sound[sp->curr_sound];
	    spool_remove(s);
	    sound_delete(s, 0);
#endif
	    break;
	}
	event_destroy(e);
    }

    c->event = NULL;
    c->ep = &c->event;

    connection_destroy(c);
}

/*
 * update the fd_sets for the connect
 */
#ifdef __STDC__
void
connection_update_fdset(CONNECTION *c)
#else
void
connection_update_fdset(c)
    CONNECTION *c;
#endif
{
    FD_CLR(c->fd, &read_mask);
    FD_CLR(c->fd, &write_mask);

    if (c->event == NULL)
    {
	return;
    }

    switch (c->event->type)
    {
    case EVENT_READ_COMMAND:
    case EVENT_READ_SOUND:
    case EVENT_READ_CONNECT_REPLY:
    case EVENT_READ_FIND_REPLY:
    case EVENT_READ_GET_REPLY:
    case EVENT_NOTIFY:
    case EVENT_READ_FLOW:
    case EVENT_WAIT_MONITOR:
	FD_SET(c->fd, &read_mask);
	break;

    case EVENT_WRITE:
    case EVENT_WRITE_SOUND:
    case EVENT_WRITE_FIND:
    case EVENT_WRITE_GET:
    case EVENT_CONNECT:
    case EVENT_WRITE_TIMEOUT:
    case EVENT_WRITE_MONITOR:
	FD_SET(c->fd, &write_mask);
	break;

    case EVENT_WAIT_FLOW:
    case EVENT_PIPE_FLOW:
	/* nothing */
	break;

    default:
	report(REPORT_ERROR, "connection_update_fdset: unknown event type '%d'\n", c->event->type);
	done(1);
    }
}

/*
 * send the connection a reply message
 */
#ifdef __STDC__
void
connection_reply(CONNECTION *c, char *fmt,...)
#else
void
connection_reply(va_alist)
    va_dcl
#endif
{
    va_list args;
    EVENT *e;
    BUFFER *b;

#ifdef __STDC__
    va_start(args, fmt);
#else
    CONNECTION *c;
    char *fmt;
    va_start(args);
    c = va_arg(args, CONNECTION *);
    fmt = va_arg(args, char *);
#endif

    b = buffer_create();

    VSNPRINTF(SIZE(b->buf, BUFFER_SIZE), fmt, args);
    va_end(args);
    b->nbytes = strlen(b->buf);
    SNPRINTF(SIZE(b->buf + b->nbytes, BUFFER_SIZE - b->nbytes), "\r\n");
    b->nbytes += 2;

    e = event_create(EVENT_WRITE, b);
    event_first(c, e);
}

/*
 * close all connections
 */
void
connection_cleanup()
{
    CONNECTION *c, *c_next;

    for (c = connections; c; c = c_next)
    {
	c_next = c->next;
	connection_close(c);
    }
}

/*
 * Check all connections to see if rplayd can enter idle mode.
 */
int
connection_idle()
{
    CONNECTION *c;
    int idle = 1;

    for (c = connections; c && idle; c = c->next)
    {
	if (c->event && c->event->type != EVENT_NOTIFY
	    && !c->monitor)
	{
	    idle = 0;
	}
    }

    return idle;
}

/*
 * timeout all idle connections
 */
void
connection_check_timeout()
{
    time_t t;
    CONNECTION *c, *next;

    if (!rptp_timeout)
    {
	return;
    }

    t = time(0);
    for (c = connections; c; c = next)
    {
	next = c->next;
	if (t - c->time >= rptp_timeout)
	{
	    connection_timeout(c);
	}
    }
}

/*
 * timeout an idle connection
 */
#ifdef __STDC__
void
connection_timeout(CONNECTION *c)
#else
void
connection_timeout(c)
    CONNECTION *c;
#endif
{
    BUFFER *b;
    EVENT *e;

    /*
     * Don't timeout clients that are waiting.
     */
    if (c->event)
    {
	switch (c->event->type)
	{
	case EVENT_NOTIFY:
	case EVENT_WAIT_MONITOR:
	    return;
	}
    }

    if (c->type == CONNECTION_SERVER)
    {
	report(REPORT_DEBUG, "%s server connection timeout\n", inet_ntoa(c->sin.sin_addr));
	connection_close(c);
    }
    else
    {
	b = buffer_create();
	SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "%cmessage=\"Connection timed out after %d idle seconds.\"\r\n",
		 RPTP_TIMEOUT, rptp_timeout);
	b->nbytes += strlen(b->buf);
	e = event_create(EVENT_WRITE_TIMEOUT, b);
	event_replace(c, e);
    }
}

/*
 * forward the sound request to the next server
 */
#ifdef __STDC__
void
connection_server_forward_sound(CONNECTION *c, SOUND *s)
#else
void
connection_server_forward_sound(c, s)
    CONNECTION *c;
    SOUND *s;
#endif
{
    CONNECTION *server;

    if (c->server->next)
    {
	server = connection_server_open(c->server->next, s);
    }
    else
    {
	spool_remove(s);
	sound_delete(s, 1);
    }
}

/*
 * forward all events to the next server
 */
#ifdef __STDC__
void
connection_server_forward(CONNECTION *c)
#else
void
connection_server_forward(c)
    CONNECTION *c;
#endif
{
    EVENT *e, *next;
    CONNECTION *server;

    if (c->server->next)
    {
	server = connection_server_open(c->server->next, NULL);
	event_insert(server, c->event);
    }
    else
    {
	for (e = c->event; e; e = next)
	{
	    next = e->next;
	    switch (e->type)
	    {
	    case EVENT_WRITE_FIND:
	    case EVENT_READ_FIND_REPLY:
	    case EVENT_WRITE_GET:
	    case EVENT_READ_GET_REPLY:
	    case EVENT_READ_SOUND:
		spool_remove(e->sound);
		sound_delete(e->sound, 1);
		break;
	    }
	    event_destroy(e);
	}
    }
    c->event = NULL;
    c->ep = &c->event;
}

BUFFER *
connection_list_create()
{
    CONNECTION *c;
    BUFFER *b, *connection_list;
    char line[RPTP_MAX_LINE];
    char buf[RPTP_MAX_LINE];
    int n, length;
    time_t t;

    b = buffer_create();
    connection_list = b;
    SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "+message=\"connections\"\r\n");
    b->nbytes += strlen(b->buf);

    t = time(0);

    for (c = connections; c; c = c->next)
    {
	length = 0;
	line[0] = '\0';

	SNPRINTF(SIZE(buf, sizeof(buf)), "host=%s", inet_ntoa(c->sin.sin_addr));
	strncat(line + length, buf, sizeof(line) - length);
	length += strlen(buf);

	SNPRINTF(SIZE(buf, sizeof(buf)), " type=");
	strncat(line + length, buf, sizeof(line) - length);
	length += strlen(buf);

	buf[0] = '\0';
	switch (c->type)
	{
	case CONNECTION_SERVER:
	    SNPRINTF(SIZE(buf, sizeof(buf)), "server");
	    break;

	case CONNECTION_CLIENT:
	    SNPRINTF(SIZE(buf, sizeof(buf)), "client");
	    break;
	}
	strncat(line + length, buf, sizeof(line) - length);
	length += strlen(buf);

	if (c->event != NULL)
	{
	    SNPRINTF(SIZE(buf, sizeof(buf)), " what=");
	    strncat(line + length, buf, sizeof(line) - length);
	    length += strlen(buf);

	    buf[0] = '\0';
	    switch (c->event->type)
	    {
	    case EVENT_READ_COMMAND:
		SNPRINTF(SIZE(buf, sizeof(buf)), "idle");
		break;

	    case EVENT_CONNECT:
	    case EVENT_READ_CONNECT_REPLY:
		SNPRINTF(SIZE(buf, sizeof(buf)), "connect");
		break;

	    case EVENT_WRITE_FIND:
	    case EVENT_READ_FIND_REPLY:
		SNPRINTF(SIZE(buf, sizeof(buf)), "\"find %s\"", c->event->sound->name);
		break;

	    case EVENT_WRITE_GET:
	    case EVENT_READ_GET_REPLY:
		SNPRINTF(SIZE(buf, sizeof(buf)), "\"get %s\"", c->event->sound->name);
		break;

	    case EVENT_READ_SOUND:
		if (c->type == CONNECTION_SERVER)
		    SNPRINTF(SIZE(buf, sizeof(buf)), "\"get %s (%d/%d)\"",
			     c->event->sound->name,
			     c->event->byte_offset,
			     c->event->nbytes);
		else
		    SNPRINTF(SIZE(buf, sizeof(buf)), "\"put %s (%d/%d)\"",
			     c->event->sound->name,
			     c->event->byte_offset,
			     c->event->nbytes);
		break;

	    case EVENT_WRITE:
		SNPRINTF(SIZE(buf, sizeof(buf)), "write");
		break;

	    case EVENT_WRITE_SOUND:
		SNPRINTF(SIZE(buf, sizeof(buf)), "\"get %s (%d/%d)\"",
			 c->event->sound->name,
			 c->event->byte_offset,
			 c->event->nbytes);
		break;

	    case EVENT_NOTIFY:
		if (c->notify_mask)
		    SNPRINTF(SIZE(buf, sizeof(buf)), "\"notify %08x\"",
			     c->notify_mask);
		else
		    SNPRINTF(SIZE(buf, sizeof(buf)), "\"wait %08x\"",
			     c->event->wait_mask);
		break;

	    case EVENT_READ_FLOW:
	    case EVENT_WAIT_FLOW:
		SNPRINTF(SIZE(buf, sizeof(buf)), "\"put #%d (%d/%d)\"",
			 c->event->id,
			 c->event->byte_offset,
			 c->event->nbytes);
		break;

	    case EVENT_PIPE_FLOW:
		SNPRINTF(SIZE(buf, sizeof(buf)), "\"pipe #%d\"",
			 c->event->id);
		break;
	    }
	    strncat(line + length, buf, sizeof(line) - length);
	    length += strlen(buf);
	}

	if (t - c->time)
	{
	    SNPRINTF(SIZE(buf, sizeof(buf)), " idle=%s", time2string(t - c->time));
	    strncat(line + length, buf, sizeof(line) - length);
	    length += strlen(buf);
	}

	if (c->application)
	{
	    SNPRINTF(SIZE(buf, sizeof(buf)), " application=\"%s\"", c->application);
	    strncat(line + length, buf, sizeof(line) - length);
	    length += strlen(buf);
	}

	SNPRINTF(SIZE(buf, sizeof(buf)), "\r\n");
	strncat(line + length, buf, sizeof(line) - length);
	length += strlen(buf);

	if (b->nbytes + length > BUFFER_SIZE)
	{
	    b->next = buffer_create();
	    b = b->next;
	}

	strncat(b->buf + b->nbytes, line, BUFFER_SIZE - b->nbytes);
	b->nbytes += length;
    }

    if (b->nbytes + 3 > BUFFER_SIZE)
    {
	b->next = buffer_create();
	b = b->next;
    }

    strncat(b->buf + b->nbytes, ".\r\n", BUFFER_SIZE - b->nbytes);
    b->nbytes += 3;

    return connection_list;
}

#ifdef __STDC__
void
connection_notify(CONNECTION *notify_connection, int notify_event,...)
#else
void
connection_notify(va_alist)
    va_dcl
#endif
{
    static int prev_play = -1, prev_pause = -1, prev_volume = -1;
    CONNECTION *c;
    EVENT *e;
    BUFFER *b;
    char buf[RPTP_MAX_LINE];
    char head[3], *p;
    int new_volume, n, length;
    int force_notify = 0;
    time_t t;
    SPOOL *sp;
    va_list args;

#ifdef __STDC__
    va_start(args, notify_event);
#else
    CONNECTION *notify_connection;
    int notify_event;
    va_start(args);
    notify_connection = va_arg(args, CONNECTION *);
    notify_event = va_arg(args, int);
#endif

    /* Extract event parameters.  */
    switch (notify_event)
    {
    case NOTIFY_VOLUME:	/* (int) */
	new_volume = va_arg(args, int);
	break;

    case NOTIFY_PLAY:		/* (SPOOL *) */
    case NOTIFY_PAUSE:
    case NOTIFY_CONTINUE:
    case NOTIFY_STOP:
    case NOTIFY_SKIP:
    case NOTIFY_DONE:
    case NOTIFY_FLOW:
    case NOTIFY_MODIFY:
    case NOTIFY_POSITION:
	sp = va_arg(args, SPOOL *);
	break;

    case NOTIFY_STATE:		/* none */
	break;

    case NOTIFY_LEVEL:		/* int */
	force_notify = va_arg(args, int);
	break;
    }

    va_end(args);

    t = time(0);

    c = notify_connection ? notify_connection : connections;
    for (; c; c = c->next)
    {
	if (notify_connection && c != notify_connection)
	{
	    break;
	}

	if (BIT(c->notify_mask, notify_event)
	    || (c->event && BIT(c->event->wait_mask, notify_event)))
	{
	    strcpy(buf, "?event=");
	    length = strlen(buf);

	    switch (notify_event)
	    {
	    case NOTIFY_DONE:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    n = sp->curr_sound;
		    if (n >= sp->rp->nsounds)
		    {
			n--;
		    }
		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length),
			     "done id=#%d sound=\"%s\" client-data=\"%s\"",
		    sp->id, sp->sound[n]->name, sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
		break;

	    case NOTIFY_VOLUME:
		SNPRINTF(SIZE(buf + length, sizeof(buf) - length), "volume volume=%d",
			 new_volume);
		break;

	    case NOTIFY_PLAY:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    SOUND *s = sp->sound[sp->curr_sound];

		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length), "play\
 id=#%d sound=\"%s\" host=%s volume=%d priority=%d count=%d seconds=%.2f size=%d\
 sample-rate=%d channels=%d bits=%g input=%s client-data=\"%s\"",
			     sp->id,
			     sp->curr_attrs->sound,
			     inet_ntoa(sp->sin.sin_addr),
			     sp->curr_attrs->volume,
			     sp->rp->priority,
			     sp->curr_attrs->count,
			     (float) s->samples / sp->sample_rate,
			     s->size,
			     sp->sample_rate,
			     s->channels,
			     s->input_precision,
			     input_to_string(s->type),
			     sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
		break;

	    case NOTIFY_STOP:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length),
			     "stop id=#%d sound=\"%s\" client-data=\"%s\"",
			     sp->id, sp->curr_attrs->sound, sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
		break;

	    case NOTIFY_PAUSE:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length),
			     "pause id=#%d sound=\"%s\" client-data=\"%s\"",
			     sp->id, sp->curr_attrs->sound, sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
		break;

	    case NOTIFY_CONTINUE:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length),
			  "continue id=#%d sound=\"%s\" client-data=\"%s\"",
			     sp->id, sp->curr_attrs->sound, sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
		break;

	    case NOTIFY_SKIP:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length),
			     "skip id=#%d sound=\"%s\" client-data=\"%s\"",
			     sp->id, sp->curr_attrs->sound, sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
		break;

	    case NOTIFY_STATE:
		SNPRINTF(SIZE(buf + length, sizeof(buf) - length),
			 "state play=%d pause=%d volume=%d priority-threshold=%d audio-port=%s",
			 spool_nplaying, spool_npaused,
			 rplay_audio_volume, rplay_priority_threshold,
			 audio_port_to_string(rplay_audio_port));
		break;

	    case NOTIFY_FLOW:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length),
			     "flow id=#%d sound=\"%s\" mark=%d low=%d high=%d client-data=\"%s\"",
			     sp->id,
			     sp->curr_attrs->sound,
			     sp->si->water_mark - sp->si->offset,
			     sp->si->low_water_mark,
			     sp->si->high_water_mark,
			     sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
		break;

	    case NOTIFY_MODIFY:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length), "modify \
id=#%d count=%d list-count=%d priority=%d sample-rate=%d volume=%d client-data=\"%s\"",
			     sp->id,
			     sp->curr_count,
			     sp->list_count,
			     sp->rp->priority,
			     sp->sample_rate,
			     sp->curr_attrs->volume,
			     sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
		break;

	    case NOTIFY_LEVEL:
		if (c->notify_rate[NOTIFY_RATE_LEVEL].rate)
		{
		    if (!force_notify
		    && c->notify_rate[NOTIFY_RATE_LEVEL].next > timer_count)
		    {
			continue;
		    }

		    c->notify_rate[NOTIFY_RATE_LEVEL].next =
			timer_count + c->notify_rate[NOTIFY_RATE_LEVEL].rate;
		}

		SNPRINTF(SIZE(buf + length, sizeof(buf) - length),
			 "level volume=%d left=%d right=%d",
			 rplay_audio_volume,
			 rplay_audio_left_level,
			 rplay_audio_right_level);
		break;

	    case NOTIFY_POSITION:
		if ((!c->notify_id || c->notify_id == sp->id)
		    || (c->event && c->event->id == sp->id))
		{
		    SOUND *s = sp->sound[sp->curr_sound];

		    if (c->notify_rate[NOTIFY_RATE_POSITION].rate)
		    {
			if (c->notify_rate[NOTIFY_RATE_POSITION].next > timer_count)
			{
			    continue;
			}

			c->notify_rate[NOTIFY_RATE_POSITION].next =
			    timer_count + c->notify_rate[NOTIFY_RATE_POSITION].rate;
		    }

		    SNPRINTF(SIZE(buf + length, sizeof(buf) - length), "position \
id=#%d position=%.2f remain=%.2f seconds=%.2f sample=%d samples=%d client-data=\"%s\"",
			     sp->id,
			     sp->sample_rate && s->samples ? sp->sample_index / sp->sample_rate : 0,
			     sp->sample_rate && s->samples ? ((double) s->samples - sp->sample_index) / sp->sample_rate : 0,
			     sp->sample_rate && s->samples ? (double) s->samples / sp->sample_rate : 0,
			     s->samples ? (int) sp->sample_index : 0,
			     s->samples,
			     sp->curr_attrs->client_data);
		}
		else
		{
		    continue;
		}
	    }

	    length = strlen(buf);
	    SNPRINTF(SIZE(buf + length, sizeof(buf) - length), "\r\n");
	    length = strlen(buf);

	    /* Build the head array with the necessary RPTP headers.  */
	    n = 0;
	    if (c->event && BIT(c->event->wait_mask, notify_event))
	    {
		head[n++] = RPTP_OK;
	    }
	    if (BIT(c->notify_mask, notify_event))
	    {
		head[n++] = RPTP_NOTIFY;
	    }
	    head[n] = '\0';

	    /* Setup write events to send `buf' using each RPTP header.  */
	    for (p = head; *p; p++)
	    {
		buf[0] = *p;

		/* Try to append `buf' to the last write event.
		   This is an attempt use reuse existing write buffers
		   to improve performance.  */
		for (e = c->event; e && e->next; e = e->next) ;
		if (e && e->type == EVENT_WRITE
		    && e->buffer->nbytes + length <= BUFFER_SIZE)
		{
		    strcpy(e->buffer->buf + e->buffer->nbytes, buf);
		    e->buffer->nbytes += length;
		    e->nbytes += length;
		    e->nleft += length;
		}
		else
		{
		    b = buffer_create();
		    strcpy(b->buf + b->nbytes, buf);
		    b->nbytes += length;
		    e = event_create(EVENT_WRITE, b);
		    if (c->event && c->event->type == EVENT_NOTIFY)
		    {
			event_replace(c, e);
		    }
		    else
		    {
			event_insert_before(c, e,
					    EVENT_READ_COMMAND
					    | EVENT_READ_SOUND
					    | EVENT_READ_CONNECT_REPLY
					    | EVENT_READ_FIND_REPLY
					    | EVENT_READ_GET_REPLY
					    | EVENT_READ_FLOW
					    | EVENT_WAIT_FLOW
					    | EVENT_PIPE_FLOW);
		    }
		}
	    }

	    c->time = t;
	}
    }

    /* Notify state changes.  */
    switch (notify_event)
    {
    default:
	/* Only notify is the state has _really_ changed. */
	if (prev_play != spool_nplaying
	    || prev_pause != spool_npaused
	    || prev_volume != rplay_audio_volume
	    || notify_connection)
	{
	    prev_play = spool_nplaying;
	    prev_pause = spool_npaused;
	    prev_volume = rplay_audio_volume;
	    /* Recursion is your friend. */
	    connection_notify(notify_connection, NOTIFY_STATE);
	}
	break;

    case NOTIFY_STATE:
	break;
    }
}

#ifdef __STDC__
void
connection_flow_pause(SPOOL *sp)
#else
void
connection_flow_pause(sp)
    SPOOL *sp;
#endif
{
    CONNECTION *c;
    EVENT *e;

    for (c = connections; c; c = c->next)
    {
	for (e = c->event; e; e = e->next)
	{
	    if (e->type == EVENT_READ_FLOW && e->id == sp->id)
	    {
		e->type = EVENT_WAIT_FLOW;
		if (c->event == e)
		{
		    connection_update_fdset(c);
		}
		break;
	    }
	}
    }
}

#ifdef __STDC__
void
connection_flow_continue(SPOOL *sp)
#else
void
connection_flow_continue(sp)
    SPOOL *sp;
#endif
{
    CONNECTION *c;
    EVENT *e;

    for (c = connections; c; c = c->next)
    {
	for (e = c->event; e; e = e->next)
	{
	    if (e->type == EVENT_WAIT_FLOW && e->id == sp->id)
	    {
		e->type = EVENT_READ_FLOW;
		if (c->event == e)
		{
		    connection_update_fdset(c);
		}
		break;
	    }
	}
    }
}

#ifdef __STDC__
void
connection_monitor_pause(CONNECTION *c)
#else
void
connection_monitor_pause(c)
    CONNECTION *c;
#endif
{
    if (c->event && c->event->type == EVENT_WRITE_MONITOR)
    {
	c->event->type = EVENT_WAIT_MONITOR;
	connection_update_fdset(c);
    }
}

void
connection_monitor_continue()
{
    CONNECTION *c;
    EVENT *e;

    for (c = connections; c; c = c->next)
    {
	for (e = c->event; e; e = e->next)
	{
	    if (e->type == EVENT_WAIT_MONITOR)
	    {
		e->type = EVENT_WRITE_MONITOR;
		if (c->event == e)
		{
		    connection_update_fdset(c);
		}
		break;
	    }
	}
    }
}

#ifdef __STDC__
void
event_update(CONNECTION *c)
#else
void
event_update(c)
    CONNECTION *c;
#endif
{
    int n, size;
    int fd;
    EVENT *e;
    time_t t;
    char *p;
    BUFFER *b;

    if (c->event == NULL)
    {
	return;
    }

    switch (c->event->type)
    {
    case EVENT_READ_COMMAND:
	if (c->event->success)
	{
	    if (c->event->start[0] != '\0'
		&& isascii(c->event->start[0]))
	    {
		b = c->event->buffer;	/* save the event's buffer */
		c->event->buffer = NULL;
		event_delete(c, 0);

		if (command(c, b->buf) != 0)
		{
		    connection_close(c);
		}
		buffer_destroy(b);
	    }
	    else
	    {
		event_delete(c, 1);
	    }
	}
	else
	{
	    connection_close(c);
	}
	break;

    case EVENT_CONNECT:
	/*
	 * check the time to delay between connect attempts
	 */
	t = time(0);
	if (t - c->event->time < RPTP_PING_DELAY)
	{
	    break;
	}
	c->event->time = t;

	/*
	 * check the socket to see if it is connected
	 */
	n = connect(c->fd, (struct sockaddr *) &c->server->sin, sizeof(c->server->sin));
	if (n == 0 || errno == EISCONN)
	{
	    /*
	     * got a connection
	     */
	    report(REPORT_NOTICE, "%s server connection established\n", inet_ntoa(c->server->sin.sin_addr));
	    /*
	     * prepare to read the connect reply, this must replace the
	     * connect event
	     */
	    e = event_create(EVENT_READ_CONNECT_REPLY, buffer_create());
	    event_replace(c, e);
	}
	else
	{
	    /*
	     * socket is not connected
	     */
	    c->event->nconnects++;
	    report(REPORT_DEBUG, "%s server connection failed attempt #%d\n",
		   inet_ntoa(c->server->sin.sin_addr),
		   c->event->nconnects);
	    if (c->event->nconnects < RPTP_CONNECT_ATTEMPTS)
	    {
		switch (errno)
		{
		case ECONNREFUSED:
		case EINTR:
		case EAGAIN:
		case EINVAL:
		case EPIPE:
		    /*
		     * ping the server again
		     */
		    connection_server_ping(c);

		    FD_CLR(c->fd, &read_mask);
		    FD_CLR(c->fd, &write_mask);
		    close(c->fd);
		    c->fd = socket(AF_INET, SOCK_STREAM, 0);
		    fd_nonblock(c->fd);

		    /*
		     * try and connect again
		     */
		    connect(c->fd, (struct sockaddr *) &c->server->sin, sizeof(c->server->sin));
		    connection_update_fdset(c);
		    return;

		default:
		    break;
		}
	    }

	    /*
	     * connection failed, remove connect event and forward all other
	     * events to the next server
	     */
	    report(REPORT_NOTICE, "%s server connection failed\n", inet_ntoa(c->server->sin.sin_addr));
	    event_delete(c, 1);
	    connection_server_forward(c);
	    connection_close(c);
	}
	break;

    case EVENT_READ_CONNECT_REPLY:
	if (c->event->success)
	{
	    if (c->event->start[0] == RPTP_OK)
	    {
		event_delete(c, 1);
	    }
	    else
	    {
		event_delete(c, 1);
		connection_server_forward(c);
		connection_close(c);
	    }
	}
	else
	{
	    report(REPORT_NOTICE, "cannot read connect reply from %s\n",
		   inet_ntoa(c->server->sin.sin_addr));
	    event_delete(c, 1);
	    connection_server_forward(c);
	    connection_close(c);
	}
	break;

    case EVENT_WRITE_FIND:
	if (c->event->success)
	{
	    e = event_create(EVENT_READ_FIND_REPLY, buffer_create(), c->event->sound);
	    event_replace(c, e);
	}
	else
	{
	    connection_server_forward(c);
	    connection_close(c);
	}
	break;

    case EVENT_READ_FIND_REPLY:
	if (c->event->success)
	{
	    switch (c->event->start[0])
	    {
	    case RPTP_OK:
		if (strchr(c->event->start, '='))
		{
		    size = atoi(rptp_parse(c->event->start, "size"));
		}
		else
		    /* old-style */
		{
		    p = strtok(c->event->start, " ");
		    size = atoi(strtok(NULL, "\r\n"));
		}
		/*
		 * see if the sound will fit in the cache
		 */
		if (cache_free(size) < 0)
		{
		    /*
		     * the sound is too big for the cache
		     */
		    spool_remove(c->event->sound);
		    sound_delete(c->event->sound, 1);
		    event_delete(c, 1);
		    break;
		}
		b = buffer_create();
		SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "get %s\r\n", c->event->sound->name);	/* old-style */
		b->nbytes = strlen(b->buf);
		e = event_create(EVENT_WRITE_GET, b, c->event->sound);
		event_replace(c, e);
		break;

	    case RPTP_ERROR:
		report(REPORT_NOTICE, "%s server does not have %s\n",
		       inet_ntoa(c->sin.sin_addr), c->event->sound->name);
		connection_server_forward_sound(c, c->event->sound);
		event_delete(c, 1);
		break;

	    case RPTP_TIMEOUT:
		report(REPORT_NOTICE, "%s server connection timed out\n",
		       inet_ntoa(c->sin.sin_addr));
		b = buffer_create();
		SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "find %s\r\n", c->event->sound->name);	/* old-style */
		b->nbytes = strlen(b->buf);
		e = event_create(EVENT_WRITE_FIND, b, c->event->sound);
		event_replace(c, e);
		connection_server_reopen(c);
		break;

	    default:
		report(REPORT_ERROR, "event_update: unknown find reply '%c'\n", c->event->start[0]);
		done(1);
	    }
	}
	else
	{
	    report(REPORT_NOTICE, "cannot read find reply from %s\n", inet_ntoa(c->sin.sin_addr));
	    b = buffer_create();
	    SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "find %s\r\n", c->event->sound->name);	/* old-style */
	    b->nbytes = strlen(b->buf);
	    e = event_create(EVENT_WRITE_FIND, b, c->event->sound);
	    event_replace(c, e);
	    connection_server_forward(c);
	    connection_close(c);
	}
	break;

    case EVENT_WRITE_GET:
	if (c->event->success)
	{
	    e = event_create(EVENT_READ_GET_REPLY, buffer_create(), c->event->sound);
	    event_replace(c, e);
	}
	else
	{
	    b = buffer_create();
	    SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "find %s\r\n", c->event->sound->name);	/* old-style */
	    b->nbytes = strlen(b->buf);
	    e = event_create(EVENT_WRITE_FIND, b, c->event->sound);
	    event_replace(c, e);
	    connection_server_forward(c);
	    connection_close(c);
	}
	break;

    case EVENT_READ_GET_REPLY:
	if (c->event->success)
	{
	    char *sound_name;

	    switch (c->event->start[0])
	    {
	    case RPTP_OK:
		if (strchr(c->event->start, '='))
		{
		    sound_name = rptp_parse(c->event->start, "sound");
		    size = atoi(rptp_parse(0, "size"));
		}
		else
		    /* old-style */
		{
		    p = strtok(c->event->start, " ");
		    p++;
		    sound_name = p;
		    size = atoi(strtok(NULL, "\r\n"));
		}

		free((char *) c->event->sound->path);
		c->event->sound->path = strdup(cache_name(sound_name));
		c->event->sound->name = c->event->sound->path[0] == '/' ?
		    strrchr(c->event->sound->path, '/') + 1 :
		    c->event->sound->path;
		fd = cache_create(c->event->sound->path, size);
		if (fd < 0)
		{
		    spool_remove(c->event->sound);
		    sound_delete(c->event->sound, 1);
		    event_delete(c, 1);
		}
		else
		{
		    e = event_create(EVENT_READ_SOUND, fd, buffer_create(),
				     size, c->event->sound);
		    c->event->sound->status = SOUND_NOT_READY;
		    event_replace(c, e);
		}
		break;

	    case RPTP_ERROR:
		report(REPORT_NOTICE, "%s server does not have %s\n",
		       inet_ntoa(c->sin.sin_addr), c->event->sound->name);
		connection_server_forward_sound(c, c->event->sound);
		event_delete(c, 1);
		break;

	    case RPTP_TIMEOUT:
		report(REPORT_NOTICE, "%s server connection timed out\n",
		       inet_ntoa(c->sin.sin_addr));
		b = buffer_create();
		SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "get %s\r\n", c->event->sound->name);	/* old-style */
		b->nbytes = strlen(b->buf);
		e = event_create(EVENT_WRITE_GET, b, c->event->sound);
		event_replace(c, e);
		connection_server_reopen(c);
		break;

	    default:
		report(REPORT_ERROR, "event_update: unknown get reply '%c'\n",
		       c->event->start[0]);
		done(1);
	    }
	}
	else
	{
	    report(REPORT_NOTICE, "cannot read get reply from %s\n",
		   inet_ntoa(c->server->sin.sin_addr));
	    b = buffer_create();
	    SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "find %s\r\n", c->event->sound->name);	/* old-style */
	    b->nbytes = strlen(b->buf);
	    e = event_create(EVENT_WRITE_FIND, b, c->event->sound);
	    event_replace(c, e);
	    connection_server_forward(c);
	    connection_close(c);
	}
	break;

    case EVENT_READ_SOUND:
	if (c->event->success)
	{
	    close(c->event->fd);
	    sound_map(c->event->sound);
	    spool_ready(c->event->sound);
	    c->event->sound->status = SOUND_READY;
	    report(REPORT_DEBUG, "sound %s is ready\n", c->event->sound->name);
	    event_delete(c, 1);
	}
	else
	{
	    report(REPORT_NOTICE, "cannot read sound %s from %s\n", c->event->sound->name,
		   inet_ntoa(c->sin.sin_addr));
	    if (c->type == CONNECTION_SERVER)
	    {
		b = buffer_create();
		SNPRINTF(SIZE(b->buf, BUFFER_SIZE), "find %s\r\n", c->event->sound->name);	/* old-style */
		b->nbytes = strlen(b->buf);
		e = event_create(EVENT_WRITE_FIND, b, c->event->sound);
		event_replace(c, e);
		connection_server_forward(c);
	    }
	    connection_close(c);
	}
	break;

    case EVENT_WRITE_SOUND:
	if (c->event->success)
	{
	    event_delete(c, 1);
	}
	else
	{
	    connection_close(c);
	}
	break;

    case EVENT_WRITE:
	if (c->event->success)
	{
	    b = c->event->buffer;
	    c->event->buffer = c->event->buffer->next;
	    if (b->status == BUFFER_FREE)
	    {
		buffer_destroy(b);
	    }
	    if (c->event->buffer)
	    {
		c->event->start = c->event->buffer->buf;
		c->event->ptr = c->event->start;
		c->event->nleft = c->event->buffer->nbytes;
		c->event->nbytes = c->event->nleft;
		c->event->success = 0;
	    }
	    else
	    {
		event_delete(c, 1);
	    }
	}
	else
	{
	    connection_close(c);
	}
	break;

    case EVENT_WRITE_TIMEOUT:
	report(REPORT_DEBUG, "%s client connection timeout\n", inet_ntoa(c->sin.sin_addr));
	connection_close(c);
	break;

    case EVENT_READ_FLOW:
    case EVENT_WAIT_FLOW:
    case EVENT_PIPE_FLOW:
	if (c->event->success)
	{
	    event_delete(c, 1);
	}
	else
	{
	    connection_close(c);
	}
	break;

    case EVENT_NOTIFY:
	report(REPORT_DEBUG, "event_update: `%d' update?\n", c->event->type);
	break;

    case EVENT_WRITE_MONITOR:
	if (c->event->success)
	{
	    /* move to the next monitor buffer */
	    c->event->buffer = c->event->buffer->next;
	    c->event->start = c->event->buffer->buf;
	    c->event->ptr = c->event->start;
	    c->event->nleft = c->event->buffer->nbytes;
	    c->event->nbytes = c->event->nleft;
	    c->event->success = 0;

	    /* need to wait for the buffer to be complete */
	    if (c->event->buffer == monitor_buffers)
	    {
		connection_monitor_pause(c);
	    }
	}
	else
	{
	    connection_close(c);
	}
	break;

    case EVENT_WAIT_MONITOR:
	connection_close(c);
	break;

    default:
	report(REPORT_ERROR, "event_update: unknown event `%d'\n", c->event->type);
	done(1);
    }
}

#ifdef __STDC__
void
event_first(CONNECTION *c, EVENT *e)
#else
void
event_first(c, e)
    CONNECTION *c;
    EVENT *e;
#endif
{
    EVENT *tail;

    for (tail = e; tail->next; tail = tail->next) ;

    tail->next = c->event;
    c->event = e;

    if (!c->event->next)
    {
	c->ep = &c->event->next;
    }

    connection_update_fdset(c);

#ifdef EVENT_DEBUG
    event_print(c, "first");
#endif
}

#ifdef __STDC__
void
event_insert_before(CONNECTION *c, EVENT *e, int mask)
#else
void
event_insert_before(c, e, mask)
    CONNECTION *c;
    EVENT *e;
    int mask;
#endif
{
    if (!c->event || (c->event->type & mask))
    {
	event_first(c, e);
    }
    else
    {
	EVENT *tail, *event;

	for (tail = e; tail->next; tail = tail->next) ;

	for (event = c->event; event->next; event = event->next)
	{
	    if (event->next->type & mask)
	    {
		break;
	    }
	}

	tail->next = event->next;
	event->next = e;

	if (!tail->next)
	{
	    c->ep = &tail->next;
	}

	if (c->event == e)
	{
	    connection_update_fdset(c);
	}

#ifdef EVENT_DEBUG
	event_print(c, "before");
#endif
    }
}

#ifdef __STDC__
void
event_insert(CONNECTION *c, EVENT *e)
#else
void
event_insert(c, e)
    CONNECTION *c;
    EVENT *e;
#endif
{
    EVENT *tail;

    for (tail = e; tail->next; tail = tail->next) ;

    *c->ep = e;
    c->ep = &tail->next;

    if (c->event == e)
    {
	connection_update_fdset(c);
    }

#ifdef EVENT_DEBUG
    event_print(c, "insert");
#endif
}

#ifdef __STDC__
void
event_replace(CONNECTION *c, EVENT *e)
#else
void
event_replace(c, e)
    CONNECTION *c;
    EVENT *e;
#endif
{
    if (c->event)
    {
	e->next = c->event->next;
	event_destroy(c->event);
    }
    c->event = e;
    if (c->event->next == NULL)
    {
	c->ep = &c->event->next;
    }
    connection_update_fdset(c);

#ifdef EVENT_DEBUG
    event_print(c, "replace");
#endif
}

#ifdef __STDC__
void
event_delete(CONNECTION *c, int replace)
#else
void
event_delete(c, replace)
    CONNECTION *c;
    int replace;
#endif
{
    EVENT *e;

    if (c->event == NULL)
    {
	connection_update_fdset(c);
	return;
    }

    e = c->event;
    c->event = c->event->next;

    event_destroy(e);

    if (c->event == NULL)
    {
	c->ep = &c->event;

	/* Replace the empty event with notify or read_command. */
	if (replace && c->type == CONNECTION_CLIENT)
	{
	    if (c->notify_mask)
	    {
		e = event_create(EVENT_NOTIFY);
	    }
	    else
	    {
		e = event_create(EVENT_READ_COMMAND, buffer_create());
	    }
	    event_insert(c, e);
	}
    }

    connection_update_fdset(c);

#ifdef EVENT_DEBUG
    event_print(c, "delete");
#endif
}

/*
 * EVENT_READ_COMMAND, BUFFER *b
 * EVENT_CONNECT
 * EVENT_READ_CONNECT_REPLY, BUFFER *b
 * EVENT_WRITE_FIND, BUFFER *b, SOUND *sound
 * EVENT_READ_FIND_REPLY, BUFFER *b, SOUND *sound
 * EVENT_WRITE_GET, BUFFER *b, SOUND *sound
 * EVENT_READ_GET_REPLY, BUFFER *b, SOUND *sound
 * EVENT_READ_SOUND, int fd, BUFFER *b, int nbytes, SOUND *sound
 * EVENT_WRITE, BUFFER *buffer
 * EVENT_WRITE_SOUND, SOUND *sound
 * EVENT_NOTIFY
 * EVENT_READ_FLOW, int spool_id, int nbytes
 */
#ifdef __STDC__
EVENT *
event_create(int type,...)
#else
EVENT *
event_create(va_alist)
    va_dcl
#endif
{
    EVENT *e;
    va_list args;

#ifdef __STDC__
    va_start(args, type);
#else
    int type;
    va_start(args);
    type = va_arg(args, int);
#endif

    e = (EVENT *) malloc(sizeof(EVENT));
    if (e == NULL)
    {
	report(REPORT_ERROR, "event_create: out of memory\n");
	done(1);
    }

    /*
     * defaults
     */
    e->next = NULL;
    e->type = type;
    e->nbytes = 0;
    e->byte_offset = 0;
    e->nleft = 0;
    e->start = NULL;
    e->ptr = NULL;
    e->sound = NULL;
    e->fd = -1;
    e->nconnects = 0;
    e->time = 0;
    e->success = 0;
    e->buffer = NULL;
    e->id = 0;
    e->wait_mask = NOTIFY_NONE;
    e->si = NULL;

    switch (e->type)
    {
    case EVENT_READ_COMMAND:
    case EVENT_READ_CONNECT_REPLY:
	e->buffer = va_arg(args, BUFFER *);
	e->nleft = RPTP_MAX_LINE;
	e->nbytes = e->nleft;
	e->ptr = e->buffer->buf;
	e->start = e->ptr;
	break;

    case EVENT_CONNECT:
	break;

    case EVENT_WRITE_FIND:
    case EVENT_WRITE_GET:
	e->buffer = va_arg(args, BUFFER *);
	e->nleft = e->buffer->nbytes;
	e->nbytes = e->nleft;
	e->ptr = e->buffer->buf;
	e->start = e->ptr;
	e->sound = va_arg(args, SOUND *);
	break;

    case EVENT_READ_FIND_REPLY:
    case EVENT_READ_GET_REPLY:
	e->buffer = va_arg(args, BUFFER *);
	e->nleft = RPTP_MAX_LINE;
	e->nbytes = e->nleft;
	e->ptr = e->buffer->buf;
	e->start = e->ptr;
	e->sound = va_arg(args, SOUND *);
	break;

    case EVENT_READ_SOUND:
	e->fd = va_arg(args, int);
	e->buffer = va_arg(args, BUFFER *);
	e->nbytes = va_arg(args, int);
	e->nleft = e->nbytes;
	e->sound = va_arg(args, SOUND *);
	break;

    case EVENT_WRITE_SOUND:
	e->sound = va_arg(args, SOUND *);
	e->buffer = buffer_create();
	e->si = sound_open(e->sound, 0);
	if (e->si == NULL)
	{
	    event_destroy(e);
	    return NULL;
	}
	if (sound_fill(e->si, e->buffer, 1) <= 0)
	{
	    event_destroy(e);
	    return NULL;
	}
	e->nbytes = e->sound->size;	/* total number of bytes */
	e->nleft = e->buffer->nbytes;	/* # bytes in the buffer */
	e->ptr = e->buffer->buf;
	e->start = e->buffer->buf;
	break;

    case EVENT_WRITE:
    case EVENT_WRITE_TIMEOUT:
	e->buffer = va_arg(args, BUFFER *);
	e->ptr = e->buffer->buf;
	e->start = e->ptr;
	e->nleft = e->buffer->nbytes;
	e->nbytes = e->nleft;
	break;

    case EVENT_NOTIFY:
	break;

    case EVENT_READ_FLOW:
	e->id = va_arg(args, int);
	e->nbytes = va_arg(args, int);
	if (e->nbytes == 0)
	{
	    e->nbytes = -1;
	}
	e->nleft = e->nbytes;
	break;

    case EVENT_WAIT_FLOW:
	/* XXX */
	break;

    case EVENT_PIPE_FLOW:
	e->id = va_arg(args, int);
	e->sound = va_arg(args, SOUND *);
	break;

    case EVENT_WAIT_MONITOR:
	e->buffer = monitor_buffers;
	e->ptr = e->buffer->buf;
	e->start = e->ptr;
	e->nleft = e->buffer->nbytes;
	e->nbytes = e->nleft;
	break;
    }

    va_end(args);

    return e;
}

#ifdef __STDC__
void
event_destroy(EVENT *e)
#else
void
event_destroy(e)
    EVENT *e;
#endif
{
    if (e->buffer && e->buffer->status == BUFFER_FREE)
    {
	buffer_destroy(e->buffer);
    }
    if (e->si)
    {
	sound_close(e->si);
    }
    free((char *) e);
}

#ifdef EVENT_DEBUG
static void
event_print(CONNECTION *c, char *message)
{
    EVENT *e;

    printf("%8s %s (%s) -> ", message, c->application, inet_ntoa(c->sin.sin_addr));
    e = c->event;
    for (; e; e = e->next)
    {
	switch (e->type)
	{
	case EVENT_READ_COMMAND:
	    printf("read_command ");
	    break;

	case EVENT_CONNECT:
	    printf("connect ");
	    break;

	case EVENT_READ_CONNECT_REPLY:
	    printf("read_connect_reply ");
	    break;

	case EVENT_WRITE_FIND:
	    printf("write_find ");
	    break;

	case EVENT_READ_FIND_REPLY:
	    printf("read_find_reply ");
	    break;

	case EVENT_WRITE_GET:
	    printf("write_get ");
	    break;

	case EVENT_READ_GET_REPLY:
	    printf("read_get_reply ");
	    break;

	case EVENT_READ_SOUND:
	    printf("read_sound ");
	    break;

	case EVENT_WRITE:
	    printf("write ");
	    break;

	case EVENT_WRITE_SOUND:
	    printf("write_sound ");
	    break;

	case EVENT_WRITE_TIMEOUT:
	    printf("write_timeout ");
	    break;

	case EVENT_NOTIFY:
	    printf("notify ");
	    break;

	case EVENT_READ_FLOW:
	    printf("read_flow ");
	    break;

	case EVENT_WAIT_FLOW:
	    printf("wait_flow ");
	    break;

	case EVENT_PIPE_FLOW:
	    printf("pipe_flow ");
	    break;

	default:
	    printf("`%d' ", e->type);
	    break;
	}
    }

    if (FD_ISSET(c->fd, &write_mask))
    {
	printf("[write] ");
    }
    if (FD_ISSET(c->fd, &read_mask))
    {
	printf("[read] ");
    }

    printf("\n");
}
#endif
