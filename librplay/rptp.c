/* $Id: rptp.c,v 1.2 1998/08/13 06:13:38 boyns Exp $ */

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
#include "version.h"
#include "rplay.h"
#include <sys/errno.h>
#ifdef ultrix
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <netdb.h>
#include <unistd.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <errno.h>

int rptp_errno = RPTP_ERROR_NONE;

char *rptp_errlist[] =
{
    "no error",			/* RPTP_ERROR_NONE */
    "out of memory",		/* RPTP_ERROR_MEMORY */
    "host not found",		/* RPTP_ERROR_HOST */
    "connection failed",	/* RPTP_ERROR_CONNECT */
    "cannot create socket",	/* RPTP_ERROR_SOCKET */
    "cannot open socket",	/* RPTP_ERROR_OPEN */
    "cannot read from socket",	/* RPTP_ERROR_READ */
    "cannot write to socket",	/* RPTP_ERROR_WRITE */
    "cannot ping rplay server",	/* RPTP_ERROR_PING */
    "connection timeout",	/* RPTP_ERROR_TIMEOUT */
    "RPTP protocol error",	/* RPTP_ERROR_PROTOCOL */
};

#ifdef __STDC__
unsigned long inet_addr (char *rp);
#else
unsigned long inet_addr ();
#endif

#ifndef HAVE_STRDUP
#ifdef __STDC__
static char *
strdup (char *str)
#else
static char *
strdup (str)
    char *str;
#endif
{
    char *p;

    p = (char *) malloc (strlen (str) + 1);
    if (p == NULL)
    {
	return NULL;
    }
    else
    {
	strcpy (p, str);
	return p;
    }
}
#endif /* HAVE_STRDUP */

/*
 * open an RPTP connection to host using port
 * the connection response will be stored in the response buffer
 */
#ifdef __STDC__
int
rptp_open (char *host, int port, char *response, int response_size)
#else
int
rptp_open (host, port, response, response_size)
    char *host;
    int port;
    char *response;
    int response_size;
#endif
{
    u_long addr;
    struct sockaddr_in s;
    struct hostent *hp;
    int rptp_fd;
    int i, n;

    rptp_errno = RPTP_ERROR_NONE;

    memset ((char *) &s, 0, sizeof (s));

    addr = inet_addr (host);
    if (addr == 0xffffffff)
    {
	hp = gethostbyname (host);
	if (hp == NULL)
	{
	    rptp_errno = RPTP_ERROR_HOST;
	    return -1;
	}
	memcpy ((char *) &s.sin_addr.s_addr, (char *) hp->h_addr, hp->h_length);
    }
    else
    {
	memcpy ((char *) &s.sin_addr.s_addr, (char *) &addr, sizeof (addr));
    }

    s.sin_port = htons (port);
    s.sin_family = AF_INET;

    for (i = 0; i < RPTP_CONNECT_ATTEMPTS; i++)
    {
	if (rplay_ping (host) < 0)
	{
	    rptp_errno = RPTP_ERROR_PING;
	    return -1;
	}

	rptp_fd = socket (AF_INET, SOCK_STREAM, 0);
	if (rptp_fd < 0)
	{
	    rptp_errno = RPTP_ERROR_SOCKET;
	    return -1;
	}

	n = connect (rptp_fd, (struct sockaddr *) &s, sizeof (s));
	if (n == 0)
	{
	    /*
	     * successful connection
	     */
	    rptp_getline (rptp_fd, response, response_size);
	    if (response[0] == RPTP_ERROR)
	    {
		rptp_errno = RPTP_ERROR_OPEN;
		return -1;
	    }
	    return rptp_fd;
	}

	switch (errno)
	{
	case ECONNREFUSED:
	case EINTR:
	    close (rptp_fd);
	    if (i + 1 != RPTP_CONNECT_ATTEMPTS)
	    {
		sleep (RPTP_PING_DELAY);
	    }
	    break;

	default:
	    rptp_errno = RPTP_ERROR_CONNECT;
	    return -1;
	}
    }

    rptp_errno = RPTP_ERROR_CONNECT;

    return -1;
}

/*
 * read nbytes from the RPTP connection
 *
 * return the number of bytes actually read
 *
 * The routine was adapted from readn() in "Unix Network Programming"
 * by W. Richard Stevens, page 279.
 */
#ifdef __STDC__
int
rptp_read (int rptp_fd, char *ptr, int nbytes)
#else
int
rptp_read (rptp_fd, ptr, nbytes)
    int rptp_fd;
    char *ptr;
    int nbytes;
#endif
{
    int nleft, nread;

    rptp_errno = RPTP_ERROR_NONE;

    nleft = nbytes;

    while (nleft > 0)
    {
	nread = read (rptp_fd, ptr, nleft);
	if (nread < 0)
	{
	    if (errno == EINTR)
	    {
		continue;
	    }
	    rptp_errno = RPTP_ERROR_READ;
	    return -1;
	}
	if (nread == 0)
	{
	    break;
	}
	nleft -= nread;
	ptr += nread;
    }

    return nbytes - nleft;
}

/*
 * write nbytes to the RPTP connection
 *
 * return the number of bytes actually written
 *
 * The routine was adapted from writen() in "Unix Network Programming"
 * by W. Richard Stevens, page 279-80.
 */
#ifdef __STDC__
int
rptp_write (int rptp_fd, char *ptr, int nbytes)
#else
int
rptp_write (rptp_fd, ptr, nbytes)
    int rptp_fd;
    char *ptr;
    int nbytes;
#endif
{
    int nleft, nwritten;

    rptp_errno = RPTP_ERROR_NONE;

    nleft = nbytes;

    while (nleft > 0)
    {
	nwritten = write (rptp_fd, ptr, nleft);
	if (nwritten < 0)
	{
	    if (errno == EINTR)
	    {
		continue;
	    }
	    rptp_errno = RPTP_ERROR_WRITE;
	    return -1;
	}
	else if (nwritten == 0)
	{
	    rptp_errno = RPTP_ERROR_WRITE;
	    return -1;
	}
	nleft -= nwritten;
	ptr += nwritten;
    }

    return nbytes - nleft;
}

/*
 * write a line to the RPTP connection
 *
 * "\r\n" is appened to the string
 */
#ifdef __STDC__
int
rptp_putline (int rptp_fd, char *fmt,...)
#else
int
rptp_putline (va_alist)
    va_dcl
#endif
{
    va_list args;
    char buf[RPTP_MAX_LINE];

#ifdef __STDC__
    va_start (args, fmt);
#else
    int rptp_fd;
    char *fmt;
    va_start (args);
    rptp_fd = va_arg (args, int);
    fmt = va_arg (args, char *);
#endif

    rptp_errno = RPTP_ERROR_NONE;

    vsprintf (buf, fmt, args);
    va_end (args);
    strcat (buf, "\r\n");

    return rptp_write (rptp_fd, buf, strlen (buf)) != strlen (buf) ? -1 : 0;
}

/*
 * read a line from the RPTP connection
 *
 * "\r\n" is removed from the line
 */
#ifdef __STDC__
int
rptp_getline (int rptp_fd, char *buf, int nbytes)
#else
int
rptp_getline (rptp_fd, buf, nbytes)
    int rptp_fd;
    char *buf;
    int nbytes;
#endif
{
    int i, n, nleft, x;
    char *ptr;
    char tmp_buf[RPTP_MAX_LINE];

    rptp_errno = RPTP_ERROR_NONE;

    nleft = nbytes;
    ptr = buf;

    while (nleft > 0)
    {
	/*
	 * peek at the message so only the necessary data
	 * is actually read
	 */
	n = recv (rptp_fd, ptr, nleft, MSG_PEEK);
	if (n < 0)
	{
	    if (errno == EINTR)
	    {
		continue;
	    }
	    rptp_errno = RPTP_ERROR_READ;
	    return -1;
	}
	else if (n == 0)
	{
	    rptp_errno = RPTP_ERROR_READ;
	    return -1;
	}
	nleft -= n;
	for (i = 0; i < n; i++)
	{
	    if (ptr[i] == '\r')
	    {
		ptr[i] = '\0';
	    }
	    else if (ptr[i] == '\n')
	    {
		ptr[i] = '\0';
		break;
	    }
	}
      again:
	x = read (rptp_fd, tmp_buf, i == n ? n : i + 1);
	if (x < 0)
	{
	    if (errno == EINTR)
	    {
		goto again;
	    }
	    rptp_errno = RPTP_ERROR_READ;
	    return -1;
	}
	else if (x == 0)
	{
	    rptp_errno = RPTP_ERROR_READ;
	    return -1;
	}
	else if (i < n)
	{
	    return 0;
	}
	else
	{
	    ptr += n;
	}
    }

    rptp_errno = RPTP_ERROR_READ;

    return -1;
}

/*
 * send the RPTP command to the RPTP connection storing the response in the
 * response buffer
 */
#ifdef __STDC__
int
rptp_command (int rptp_fd, char *command, char *response, int response_size)
#else
int
rptp_command (rptp_fd, command, response, response_size)
    int rptp_fd;
    char *command;
    char *response;
    int response_size;
#endif
{
    rptp_errno = RPTP_ERROR_NONE;

    if (rptp_putline (rptp_fd, command) < 0)
    {
	return -1;
    }
    if (rptp_getline (rptp_fd, response, response_size) < 0)
    {
	return -1;
    }
    switch (response[0])
    {
    case RPTP_TIMEOUT:
	rptp_errno = RPTP_ERROR_TIMEOUT;
	return -1;

    case RPTP_ERROR:
	return 1;

    case RPTP_OK:
    case RPTP_NOTIFY:
	return 0;

    default:
	rptp_errno = RPTP_ERROR_PROTOCOL;
	return -1;
    }
}

/*
 * close the RPTP connection
 */
#ifdef __STDC__
int
rptp_close (int rptp_fd)
#else
int
rptp_close (rptp_fd)
    int rptp_fd;
#endif
{
    rptp_errno = RPTP_ERROR_NONE;

    close (rptp_fd);

    return 0;
}

/*
 * report RPTP errors
 */
#ifdef __STDC__
void
rptp_perror (char *message)
#else
void
rptp_perror (message)
    char *message;
#endif
{
    fprintf (stderr, "%s: %s\n", message, rptp_errlist[rptp_errno]);
}

/*
 * List definitions used in rptp_parse.
 */
typedef struct _list
{
    struct _list *next;
    char *name;
    char *value;
}
LIST;

static LIST *list = NULL, **list_next = &list;

#define list_alloc()				\
	*list_next = (LIST *) malloc (sizeof(LIST)); \
	if (!*list_next)			\
	{					\
		return NULL;			\
	}

#define list_add(xname, xvalue)			\
	list_alloc();				\
	(*list_next)->name = xname;		\
	(*list_next)->value = xvalue;		\
	list_next = &(*list_next)->next;	\
	*list_next = NULL;

#define list_free()				\
{						\
	LIST *l;				\
	for (; list; l = list, list = list->next, free ((char *)l)); \
	list = NULL;				\
	list_next = &list;			\
}

#ifdef __STDC__
char *
rptp_parse (char *response, char *name)
#else
char *
rptp_parse (response, name)
    char *response;
    char *name;
#endif
{
    static char *buf;
    static LIST *list_pos, *cache_pos;

    /* No more name-value pairs. */
    if (!response && !name && !list_pos)
    {
	return NULL;
    }
    /* Load the new `name=value' list.  */
    else if (response)
    {
	char *p;
	char *response_name = "", *response_value = "";

	list_free ();
	if (buf)
	{
	    free ((char *) buf);
	}
	buf = strdup (response);

	p = buf;

	/* Skip any initial RPTP characters. */
	switch (*p)
	{
	case RPTP_ERROR:
	case RPTP_OK:
	case RPTP_NOTIFY:
	    p++;
	    break;
	}

	while (p && *p)
	{
	    /* Skip white-space. */
	    if (isspace (*p))
	    {
		for (p++; isspace (*p); p++) ;
		continue;
	    }

	    /* `name' */
	    response_name = p;
	    p = strpbrk (p, "= \t\r\n");

	    /* `value' */
	    if (p && *p == '=')
	    {
		int quoted = 0;

		*p++ = '\0';	/* Remove the `=' */

		if (*p == '"')	/* Start of a quoted value */
		{
		    p++;
		    quoted++;
		}

		response_value = p;
		if (quoted)
		{
		    p = strchr (p, '"');
		}
		else
		{
		    p = strpbrk (p, " \t\r\n");
		}
		if (p)
		{
		    *p++ = '\0';
		}
	    }
	    else if (p)
	    {
		*p++ = '\0';
	    }

	    list_add (response_name, response_value);
	    response_name = "";
	    response_value = "";
	}

	list_pos = list;
	cache_pos = NULL;
    }

    /* Search for `name'. */
    if (name)
    {
	LIST *l;
	char *p;
	
	/* Skip any leading dashes. */
	while (*name == '-')
	{
	    name++;
	}
	
	if (cache_pos)
	{
	    for (p = cache_pos->name; *p && *p == '-'; p++) ; /* skip leading dashes */
	    if (strcmp (name, p) == 0)
	    {
		return cache_pos->value;
	    }
	}

	for (l = list; l; l = l->next)
	{
	    for (p = l->name; *p && *p == '-'; p++) ; /* skip leading dashes */
	    if (strcmp (p, name) == 0)
	    {
		return l->value;
	    }
	}
	return NULL;		/* Not found */
    }
    /* Cycle through the names. */
    else if (list_pos)
    {
	cache_pos = list_pos;
	list_pos = list_pos->next;
	return cache_pos->name;
    }

    return NULL;
}
