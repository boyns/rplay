/* server.c - RPTP server maintenance.  */

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



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "rplay.h"
#include <sys/param.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "rplayd.h"
#include "server.h"
#include "connection.h"
#include "buffer.h"

BUFFER *server_list = NULL;
SERVER *servers = NULL;

static time_t server_read_time;

#ifdef __STDC__
void
server_read (char *filename)
#else
void
server_read (filename)
    char *filename;
#endif
{
    FILE *fp;
    char buf[BUFSIZ], *p;
    char line[RPTP_MAX_LINE];
    struct hostent *hp, *local_hp;
    unsigned long addr, local_addr;
    struct sockaddr_in sin;
    int port, n;
    SERVER **s = &servers;
    BUFFER *b;

    server_read_time = time (0);

    fp = fopen (filename, "r");
    if (fp == NULL)
    {
	/*
	 * I guess it's ok to not have any servers.
	 */
	report (REPORT_NOTICE, "warning: cannot open %s\n", filename);
	return;
    }

    local_hp = gethostbyname (hostname);
    if (local_hp == NULL)
    {
	report (REPORT_ERROR, "server_read: %s unknown host?!\n", hostname);
	done (1);

    }
    memcpy ((char *) &local_addr, (char *) local_hp->h_addr, local_hp->h_length);

    b = buffer_create ();
    strcpy (b->buf, "+message=\"servers\"\r\n");
    b->nbytes += strlen (b->buf);
    b->status = BUFFER_KEEP;
    server_list = b;

    while (fgets (buf, sizeof (buf), fp))
    {
	switch (buf[0])
	{
	case '#':
	case ' ':
	case '\t':
	case '\n':
	    continue;
	}

	p = strchr (buf, '\n');
	if (p)
	{
	    *p = '\0';
	}

	p = strchr (buf, ':');
	if (p)
	{
	    *p = '\0';
	    port = atoi (p + 1);
	}
	else
	{
	    port = RPTP_PORT;
	}
	addr = inet_addr (buf);
	memset ((char *) &sin, 0, sizeof (sin));
	if (addr == 0xffffffff)
	{
	    hp = gethostbyname (buf);
	    if (hp == NULL)
	    {
		report (REPORT_NOTICE, "warning: %s unknown host in %s\n", buf, filename);
		continue;
	    }
	    memcpy ((char *) &sin.sin_addr.s_addr, (char *) hp->h_addr, hp->h_length);
	}
	else
	{
	    memcpy ((char *) &sin.sin_addr.s_addr, (char *) &addr, sizeof (addr));
	}

	if (memcmp ((char *) &sin.sin_addr.s_addr, (char *) &local_addr, sizeof (local_addr)) == 0)
	{
	    /*
	     * ignore the server if it's the local host
	     */
	    continue;
	}

	*s = (SERVER *) malloc (sizeof (SERVER));
	if (*s == NULL)
	{
	    report (REPORT_ERROR, "server_read: out of memory\n");
	    done (1);
	}
	sin.sin_family = AF_INET;
	sin.sin_port = htons (port);

	SNPRINTF (SIZE(line,sizeof(line)), "host=%s port=%d\r\n", buf, port);
	n = strlen (line);

	if (b->nbytes + n > BUFFER_SIZE)
	{
	    b->next = buffer_create ();
	    b = b->next;
	    b->status = BUFFER_KEEP;
	}
	SNPRINTF (SIZE(b->buf+strlen(b->buf), BUFFER_SIZE), line);
	b->nbytes += n;

	(*s)->sin = sin;
	s = &(*s)->next;

	report (REPORT_DEBUG, "server %s port %d\n", inet_ntoa (sin.sin_addr), port);
    }

    *s = NULL;
    fclose (fp);

    if (b->nbytes + 3 > BUFFER_SIZE)
    {
	b->next = buffer_create ();
	b = b->next;
	b->status = BUFFER_KEEP;
    }
    SNPRINTF (SIZE(b->buf+strlen(b->buf), BUFFER_SIZE), ".\r\n");
    b->nbytes += 3;
}

#ifdef __STDC__
void
server_reread (char *filename)
#else
void
server_reread (filename)
    char *filename;
#endif
{
    BUFFER *b, *bb;
    SERVER *s, *ss;

    report (REPORT_DEBUG, "re-reading servers\n");

    /*
     * Free the server_list buffers.
     */
    for (b = server_list; b; bb = b, b = b->next, bb->status = BUFFER_FREE, buffer_destroy (bb)) ;

    /*
     * Free the servers list.
     */
    for (s = servers; s; ss = s, s = s->next, free ((char *) ss)) ;

    server_list = NULL;
    servers = NULL;
    server_read (filename);
}

#ifdef __STDC__
void
server_stat (char *filename)
#else
void
server_stat (filename)
    char *filename;
#endif
{
    if (modified (filename, server_read_time))
    {
	server_reread (filename);
    }
}
