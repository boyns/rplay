/* $Id: host.c,v 1.6 1999/06/09 06:27:44 boyns Exp $ */

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
#include "rplayd.h"
#include <netdb.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "host.h"
#include "buffer.h"
#include "misc.h"
#ifdef HAVE_RX_RXPOSIX_H
#include <rx/rxposix.h>
#else
#ifdef HAVE_RXPOSIX_H
#include <rxposix.h>
#else
#include "rxposix.h"
#endif
#endif

#ifdef AUTH

static regex_t access_read, access_write, access_execute, access_monitor;
BUFFER *host_list = NULL;
BUFFER *b;

static time_t host_read_time = 0;

/*
 * read the rplay hosts file
 *
 * This routine will build a regular expression for each type of access.
 * The expressions are of the form:
 *      \(130\.191\.224\.2\|130\.191\..*\)
 */
#ifdef __STDC__
void
host_read(char *filename)
#else
void
host_read(filename)
    char *filename;
#endif
{
    FILE *fp;
    char buf[BUFSIZ], *perms, *name, *p;
    char expr_read[HOST_EXPR_SIZE];
    char expr_write[HOST_EXPR_SIZE];
    char expr_execute[HOST_EXPR_SIZE];
    char expr_monitor[HOST_EXPR_SIZE];
    int error = 0;

    host_read_time = time(0);

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
	report(REPORT_ERROR, "host_read: cannot open %s\n", filename);
	/* Don't exit anymore.  Localhost will be added automatically
	   later. */
    }

    b = buffer_create();
    b->status = BUFFER_KEEP;
    strcpy(b->buf, "+message=\"hosts\"\r\n");
    b->nbytes += strlen(b->buf);
    host_list = b;

    //memset ((char *) &access_read, 0, sizeof (access_read));
    //memset ((char *) &access_write, 0, sizeof (access_write));
    //memset ((char *) &access_execute, 0, sizeof (access_execute));

    strcpy(expr_read, "^\\(");
    strcpy(expr_write, "^\\(");
    strcpy(expr_execute, "^\\(");
    strcpy(expr_monitor, "^\\(");

    do
    {
	if (fp)
	{
	    p = fgets(buf, sizeof(buf), fp);
	    if (!p)
	    {
		fclose(fp);
		break;
	    }
	}
	else
	{
	    /* rplay.hosts wasn't found and AUTH was defined in config.h.
	       Assume that only the localhost should have access. */
	    SNPRINTF(SIZE(buf, sizeof(buf)), "%s:rwx", hostaddr);
	    report(REPORT_NOTICE, "host_read: adding %s\n", buf);
	}

	switch (buf[0])
	{
	case '#':
	case ' ':
	case '\t':
	case '\n':
	    continue;
	}

	p = strchr(buf, '\n');
	if (p)
	{
	    *p = '\0';
	}

	name = buf;
	perms = strchr(buf, ':');
	if (perms)
	{
	    *perms = '\0';
	    perms++;
	}
	else
	{
	    perms = HOST_DEFAULT_ACCESS;
	}

	host_insert(expr_read, expr_write, expr_execute, expr_monitor,
		    name, perms);
    }
    while (fp);

    if (b->nbytes + 3 > BUFFER_SIZE)	/* room for .\r\n */
    {
	b->next = buffer_create();
	b = b->next;
	b->status = BUFFER_KEEP;
    }
    SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE - b->nbytes), ".\r\n");
    b->nbytes += 3;

    if (strlen(expr_read) == 3)
    {
	strcat(expr_read, "\\)");
    }
    else
    {
	expr_read[strlen(expr_read) - 1] = ')';
    }
    strcat(expr_read, "$");
    if (strlen(expr_write) == 3)
    {
	strcat(expr_write, "\\)");
    }
    else
    {
	expr_write[strlen(expr_write) - 1] = ')';
    }
    strcat(expr_write, "$");
    if (strlen(expr_execute) == 3)
    {
	strcat(expr_execute, "\\)");
    }
    else
    {
	expr_execute[strlen(expr_execute) - 1] = ')';
    }
    strcat(expr_execute, "$");
    if (strlen(expr_monitor) == 3)
    {
	strcat(expr_monitor, "\\)");
    }
    else
    {
	expr_monitor[strlen(expr_monitor) - 1] = ')';
    }
    strcat(expr_monitor, "$");

    error = regncomp(&access_read, expr_read, strlen(expr_read),
		     REG_ICASE | REG_NOSUB);
    if (error)
    {
	report(REPORT_ERROR, "host_read: regncomp: %d\n", error);
	done(1);
    }

    error = regncomp(&access_write, expr_write, strlen(expr_write),
		     REG_ICASE | REG_NOSUB);
    if (error)
    {
	report(REPORT_ERROR, "host_read: regncomp: %d\n", error);
	done(1);
    }

    error = regncomp(&access_execute, expr_execute, strlen(expr_execute),
		     REG_ICASE | REG_NOSUB);
    if (error)
    {
	report(REPORT_ERROR, "host_read: regncomp: %d\n", error);
	done(1);
    }

    error = regncomp(&access_monitor, expr_monitor, strlen(expr_monitor),
		     REG_ICASE | REG_NOSUB);
    if (error)
    {
	report(REPORT_ERROR, "host_read: regncomp: %d\n", error);
	done(1);
    }
}

#ifdef __STDC__
void
host_reread(char *filename)
#else
void
host_reread(filename)
    char *filename;
#endif
{
    BUFFER *b, *bb;

    report(REPORT_DEBUG, "re-reading hosts\n");

    /*
     * Free the old host buffer list.
     */
    for (b = host_list; b;
	 bb = b, b = b->next, bb->status = BUFFER_FREE, buffer_destroy(bb)) ;

    host_list = NULL;
    host_read(filename);
}

#ifdef __STDC__
void
host_stat(char *filename)
#else
void
host_stat(filename)
    char *filename;
#endif
{
    if (modified(filename, host_read_time))
    {
	host_reread(filename);
    }
}

#ifdef __STDC__
void
host_insert(char *expr_read, char *expr_write, char *expr_execute, char *expr_monitor,
	    char *name, char *perms)
#else
void
host_insert(expr_read, expr_write, expr_execute, expr_monitor, name, perms)
    char *expr_read;
    char *expr_write;
    char *expr_execute;
    char *expr_monitor;
    char *name;
    char *perms;
#endif
{
    unsigned long addr;
    struct in_addr addr_in;
    struct hostent *hp;
    int n;
    char **ap, *p;
    char *re_name = 0;
    char line[RPTP_MAX_LINE];

    SNPRINTF(SIZE(line, sizeof(line)), "host=%s access=%s\r\n", name, perms);
    n = strlen(line);

    if (b->nbytes + n > BUFFER_SIZE)
    {
	b->next = buffer_create();
	b = b->next;
	b->status = BUFFER_KEEP;
    }

    strcat(b->buf, line);
    b->nbytes += n;

    if (strchr(name, '*'))
    {
	re_name = host_ip_to_regex(name);
    }
    else
    {
	addr = inet_addr(name);
	if (addr == 0xffffffff)
	{
	    hp = gethostbyname(name);
	    if (hp == NULL)
	    {
		report(REPORT_NOTICE, "warning: %s unknown host\n", name);
		return;
	    }
	    /*
	     * Handle multiple IP address.
	     */
	    for (ap = hp->h_addr_list + 1; *ap; ap++)
	    {
		memcpy((char *) &addr_in, *ap, hp->h_length);
		host_insert(expr_read, expr_write, expr_execute, expr_monitor,
			    inet_ntoa(addr_in), perms);
	    }
	    memcpy((char *) &addr_in, (char *) hp->h_addr, sizeof(addr_in));
	    re_name = host_ip_to_regex(inet_ntoa(addr_in));
	}
	else
	{
	    memcpy((char *) &addr_in, (char *) &addr, sizeof(addr_in));
	    re_name = host_ip_to_regex(inet_ntoa(addr_in));
	}

	/*
	 * Add localhost automatically.
	 */
	if (strcmp(hostaddr, "127.0.0.1") != 0
	    && strcmp(inet_ntoa(addr_in), hostaddr) == 0)
	{
	    report(REPORT_DEBUG, "host_insert: adding localhost (127.0.0.1)\n");
	    host_insert(expr_read, expr_write, expr_execute, expr_monitor, "127.0.0.1", perms);
	}
    }

    for (p = perms; *p; p++)
    {
	switch (*p)
	{
	case HOST_READ:
	    strcat(expr_read, re_name);
	    strcat(expr_read, "\\|");
	    break;

	case HOST_WRITE:
	    strcat(expr_write, re_name);
	    strcat(expr_write, "\\|");
	    break;

	case HOST_EXECUTE:
	    strcat(expr_execute, re_name);
	    strcat(expr_execute, "\\|");
	    break;

	case HOST_MONITOR:
	    strcat(expr_monitor, re_name);
	    strcat(expr_monitor, "\\|");
	    break;

	default:
	    report(REPORT_ERROR, "host_insert: '%c' unknown host access permission\n", *p);
	    done(1);
	}
    }

    if (re_name)
    {
	free(re_name);
    }
}

/* return 1 if allowed */
#ifdef __STDC__
int
host_access(struct sockaddr_in sin, char access_mode)
#else
int
host_access(sin, access_mode)
    struct sockaddr_in sin;
    char access_mode;
#endif
{
    char *p;
    regex_t *re;
    int n;

    /*
     * Accept all accesses when authentication is not enabled.
     */
    if (!auth_enabled)
    {
	return 1;
    }

    p = inet_ntoa(sin.sin_addr);

    switch (access_mode)
    {
    case HOST_READ:
	re = &access_read;
	break;

    case HOST_WRITE:
	re = &access_write;
	break;

    case HOST_EXECUTE:
	re = &access_execute;
	break;

    case HOST_MONITOR:
	re = &access_monitor;
	break;

    default:
	report(REPORT_ERROR, "host_access: unknown access mode '%s'\n", access_mode);
	done(1);
    }

    n = regnexec(re, p, strlen(p), 0, 0, 0);

    return !n;
}

#ifdef __STDC__
char *
host_ip_to_regex(char *p)
#else
char *
host_ip_to_regex(p)
    char *p;
#endif
{
    char buf[64];
    char *q;

    for (q = buf; *p; p++, q++)
    {
	switch (*p)
	{
	case '.':
	    *q++ = '\\';
	    *q = '.';
	    break;

	case '*':
	    *q++ = '.';
	    *q = '*';
	    break;

	default:
	    *q = *p;
	}
    }

    *q = '\0';

    return strdup(buf);
}

#endif /* AUTH */
