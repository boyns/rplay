/* $Id: rplay.c,v 1.7 2002/12/11 05:12:16 boyns Exp $ */

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
#include "version.h"
#include "rplay.h"
#include <sys/param.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#ifndef __FreeBSD__
#include <malloc.h>
#endif
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <unistd.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

/* Make sure MAXHOSTNAMELEN is defined. */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

int rplay_errno;

char *rplay_errlist[] =
{
    "no error",			/* RPLAY_ERROR_NONE */
    "out of memory",		/* RPLAY_ERROR_MEMORY */
    "host not found",		/* RPLAY_ERROR_HOST */
    "cannot connect to socket",	/* RPLAY_ERROR_CONNECT */
    "cannot create socket",	/* RPLAY_ERROR_SOCKET */
    "error writing to socket",	/* RPLAY_ERROR_WRITE */
    "error closing socket",	/* RPLAY_ERROR_CLOSE */
    "max packet size exceeded",	/* RPLAY_ERROR_PACKET_SIZE */
    "cannot enable broadcast",	/* RPLAY_ERROR_BROADCAST */
    "unknown rplay attribute",	/* RPLAY_ERROR_ATTRIBUTE */
    "unknown rplay command",	/* RPLAY_ERROR_COMMAND */
    "illegal rplay index value",	/* RPLAY_ERROR_INDEX */
    "unknown rplay modifier",	/* RPLAY_ERROR_MODIFIER */
};
/*
   #ifdef __STDC__
   unsigned long inet_addr (char *rp);
   #else
   unsigned long inet_addr ();
   #endif
 */
/* A simple version a strdup. */
#ifndef HAVE_STRDUP
#ifdef __STDC__
static char *
strdup(char *str)
#else
static char *
strdup(str)
    char *str;
#endif
{
    char *p;

    p = (char *) malloc(strlen(str) + 1);
    if (p == NULL)
    {
	return NULL;
    }
    else
    {
	strcpy(p, str);
	return p;
    }
}
#endif /* HAVE_STRDUP */

/*
 * create an rplay attributes object
 */
#ifdef __STDC__
static RPLAY_ATTRS *
rplay_attrs_create(void)
#else
static RPLAY_ATTRS *
rplay_attrs_create()
#endif
{
    RPLAY_ATTRS *attrs;

    attrs = (RPLAY_ATTRS *) malloc(sizeof(RPLAY_ATTRS));
    if (attrs == NULL)
    {
	return NULL;
    }
    attrs->next = NULL;
    attrs->sound = "";
    attrs->volume[0] = RPLAY_DEFAULT_VOLUME;
    attrs->volume[1] = RPLAY_DEFAULT_VOLUME;
    attrs->count = RPLAY_DEFAULT_COUNT;
    attrs->rptp_server = NULL;
    attrs->rptp_server_port = RPTP_PORT;
    attrs->rptp_search = TRUE;
    attrs->sample_rate = RPLAY_DEFAULT_SAMPLE_RATE;
    attrs->client_data = "";

    return attrs;
}

/*
 * free memory used by an rplay attributes object
 */
#ifdef __STDC__
static void
rplay_attrs_destroy(RPLAY_ATTRS *attrs)
#else
static void
rplay_attrs_destroy(attrs)
    RPLAY_ATTRS *attrs;
#endif
{
    if (*attrs->sound)
    {
	free((char *) attrs->sound);
    }
    if (attrs->rptp_server)
    {
	free((char *) attrs->rptp_server);
    }
    if (*attrs->client_data)
    {
	free((char *) attrs->client_data);
    }
    free((char *) attrs);
}

#define COPY_SIZE	128
#define COPY(rp, p, n) \
	grow = 0; \
	while (rp->len + n > rp->size) \
	{ \
		grow++; \
		rp->size += COPY_SIZE; \
		if (rp->size > MAX_PACKET) \
		{ \
			rplay_errno = RPLAY_ERROR_PACKET_SIZE; \
			return -1; \
		} \
	} \
	if (grow) \
	{ \
		rp->buf = realloc (rp->buf, rp->size); \
		if (rp->buf == NULL) \
		{ \
			rplay_errno = RPLAY_ERROR_MEMORY; \
			return -1; \
		} \
	} \
	memcpy ((char *) rp->buf + rp->len, (char *) p, n); \
	rp->len += n; \

/*
 * pack the rplay object into the internal buffer
 *
 * this routine is called everytime rplay_set is used
 */
#ifdef __STDC__
int
rplay_pack(RPLAY *rp)
#else
int
rplay_pack(rp)
    RPLAY *rp;
#endif
{
    RPLAY_ATTRS *attrs;
    int len, grow = 0, i;
    unsigned char val;
    unsigned long lval;
    unsigned short sval;
    short size;

    rp->len = 0;
    val = RPLAY_PACKET_ID;
    COPY(rp, &val, sizeof(val));
    val = rp->command;
    COPY(rp, &val, sizeof(val));

    if (rp->count != RPLAY_DEFAULT_LIST_COUNT)
    {
	val = RPLAY_LIST_COUNT;
	COPY(rp, &val, sizeof(val));
	val = rp->count;
	COPY(rp, &val, sizeof(val));
    }

    if (rp->priority != RPLAY_DEFAULT_PRIORITY)
    {
	val = RPLAY_PRIORITY;
	COPY(rp, &val, sizeof(val));
	val = rp->priority;
	COPY(rp, &val, sizeof(val));
    }

    if (*rp->list_name)
    {
	val = RPLAY_LIST_NAME;
	COPY(rp, &val, sizeof(val));
	len = strlen(rp->list_name) + 1;
	COPY(rp, rp->list_name, len);
    }

    if (rp->id != RPLAY_NULL)
    {
	val = RPLAY_ID;
	COPY(rp, &val, sizeof(val));
	lval = htonl(rp->id);
	COPY(rp, &lval, sizeof(lval));
    }

    if (rp->sequence != -1)
    {
	val = RPLAY_SEQUENCE;
	COPY(rp, &val, sizeof(val));
	lval = htonl(rp->sequence);
	COPY(rp, &lval, sizeof(lval));
    }

    if (rp->data != RPLAY_NULL && rp->data_size > 0)
    {
	val = RPLAY_DATA_SIZE;
	COPY(rp, &val, sizeof(val));
	sval = htons(rp->data_size);
	COPY(rp, &sval, sizeof(sval));

	val = RPLAY_DATA;
	COPY(rp, &val, sizeof(val));
	COPY(rp, rp->data, rp->data_size);
    }

    for (i = 0, attrs = rp->attrs; attrs; attrs = attrs->next, i++)
    {
	if (rp->random_sound != RPLAY_DEFAULT_RANDOM_SOUND)
	{
	    if (i != rp->random_sound)
	    {
		continue;
	    }
	}

	if (*attrs->sound)
	{
	    val = RPLAY_SOUND;
	    COPY(rp, &val, sizeof(val));
	    len = strlen(attrs->sound) + 1;
	    COPY(rp, attrs->sound, len);
	}

	if (attrs->volume[0] != RPLAY_DEFAULT_VOLUME)
	{
	    val = RPLAY_LEFT_VOLUME;
	    COPY(rp, &val, sizeof(val));
	    val = attrs->volume[0];
	    COPY(rp, &val, sizeof(val));
	}

	if (attrs->volume[1] != RPLAY_DEFAULT_VOLUME)
	{
	    val = RPLAY_RIGHT_VOLUME;
	    COPY(rp, &val, sizeof(val));
	    val = attrs->volume[1];
	    COPY(rp, &val, sizeof(val));
	}

	if (attrs->count != RPLAY_DEFAULT_COUNT)
	{
	    val = RPLAY_COUNT;
	    COPY(rp, &val, sizeof(val));
	    val = attrs->count;
	    COPY(rp, &val, sizeof(val));
	}

	if (attrs->rptp_server)
	{
	    val = RPLAY_RPTP_SERVER;
	    COPY(rp, &val, sizeof(val));
	    len = strlen(attrs->rptp_server) + 1;
	    COPY(rp, attrs->rptp_server, len);
	}

	if (attrs->rptp_server_port != RPTP_PORT)
	{
	    val = RPLAY_RPTP_SERVER_PORT;
	    COPY(rp, &val, sizeof(val));
	    size = htons(attrs->rptp_server_port);
	    COPY(rp, &size, sizeof(size));
	}

	if (attrs->rptp_search == FALSE)
	{
	    val = RPLAY_RPTP_SEARCH;
	    COPY(rp, &val, sizeof(val));
	    val = attrs->rptp_search;
	    COPY(rp, &val, sizeof(val));
	}

	if (attrs->sample_rate != RPLAY_DEFAULT_SAMPLE_RATE)
	{
	    val = RPLAY_SAMPLE_RATE;
	    COPY(rp, &val, sizeof(val));
	    lval = htonl(attrs->sample_rate);
	    COPY(rp, &lval, sizeof(lval));
	}

	if (*attrs->client_data)
	{
	    val = RPLAY_CLIENT_DATA;
	    COPY(rp, &val, sizeof(val));
	    len = strlen(attrs->client_data) + 1;
	    COPY(rp, attrs->client_data, len);
	}

	val = RPLAY_NULL;
	COPY(rp, &val, sizeof(val));
    }

    if (i == 0)
    {
	val = RPLAY_NULL;
	COPY(rp, &val, sizeof(val));
    }

    val = RPLAY_NULL;
    COPY(rp, &val, sizeof(val));

    return 0;
}

/*
 * unpack an rplay packet into an rplay object
 */
#ifdef __STDC__
RPLAY *
rplay_unpack(char *packet)
#else
RPLAY *
rplay_unpack(packet)
    char *packet;
#endif
{
    RPLAY *rp;
    int still_going = 1;
    int version;

    rplay_errno = RPLAY_ERROR_NONE;

    version = *packet++;
    rp = rplay_create(*packet++);
    if (rp == NULL)
    {
	rplay_errno = RPLAY_ERROR_MEMORY;
	return NULL;
    }

    *(rp->attrsp) = rplay_attrs_create();
    if (*(rp->attrsp) == NULL)
    {
	rplay_errno = RPLAY_ERROR_MEMORY;
	return NULL;
    }

    while (still_going)
    {
	switch (*packet++)
	{
	case RPLAY_LIST_COUNT:
	    rp->count = (unsigned char) *packet++;
	    break;

	case RPLAY_LIST_NAME:
	    rp->list_name = strdup(packet);
	    packet += strlen(packet) + 1;
	    break;

	case RPLAY_PRIORITY:
	    rp->priority = (unsigned char) *packet++;
	    break;

	case RPLAY_SOUND:
	    (*rp->attrsp)->sound = strdup(packet);
	    packet += strlen(packet) + 1;
	    break;

	case RPLAY_VOLUME:
	    (*rp->attrsp)->volume[0] = (unsigned char) *packet++;
	    (*rp->attrsp)->volume[1] = (*rp->attrsp)->volume[0];
	    break;

        case RPLAY_LEFT_VOLUME:
	    (*rp->attrsp)->volume[0] = (unsigned char) *packet++;
            break;

        case RPLAY_RIGHT_VOLUME:
	    (*rp->attrsp)->volume[1] = (unsigned char) *packet++;
            break;

	case RPLAY_COUNT:
	    (*rp->attrsp)->count = (unsigned char) *packet++;
	    break;

	case RPLAY_RPTP_SERVER:
	    (*rp->attrsp)->rptp_server = strdup(packet);
	    packet += strlen(packet) + 1;
	    break;

	case RPLAY_RPTP_SERVER_PORT:
	    memcpy((char *) &(*rp->attrsp)->rptp_server_port, packet, sizeof((*rp->attrsp)->rptp_server_port));
	    (*rp->attrsp)->rptp_server_port = ntohs((*rp->attrsp)->rptp_server_port);
	    packet += sizeof((*rp->attrsp)->rptp_server_port);
	    break;


	case RPLAY_RPTP_SEARCH:
	    (*rp->attrsp)->rptp_search = (unsigned char) *packet++;
	    break;

	case RPLAY_SAMPLE_RATE:
	    memcpy((char *) &(*rp->attrsp)->sample_rate, packet,
		   sizeof((*rp->attrsp)->sample_rate));
	    (*rp->attrsp)->sample_rate = ntohl((*rp->attrsp)->sample_rate);
	    packet += sizeof((*rp->attrsp)->sample_rate);
	    break;

	case RPLAY_CLIENT_DATA:
	    (*rp->attrsp)->client_data = strdup(packet);
	    packet += strlen(packet) + 1;
	    break;

	case RPLAY_ID:
	    memcpy((char *) &rp->id, packet, sizeof(rp->id));
	    rp->id = ntohl(rp->id);
	    packet += sizeof(rp->id);
	    break;

	case RPLAY_SEQUENCE:
	    memcpy((char *) &rp->sequence, packet, sizeof(rp->sequence));
	    rp->sequence = ntohl(rp->sequence);
	    packet += sizeof(rp->sequence);
	    break;

	case RPLAY_DATA_SIZE:
	    memcpy((char *) &rp->data_size, packet, sizeof(rp->data_size));
	    rp->data_size = ntohs(rp->data_size);
	    packet += sizeof(rp->data_size);
	    break;

	case RPLAY_DATA:
	    rp->data = (char *) malloc(rp->data_size);
	    memcpy(rp->data, packet, rp->data_size);
	    packet += rp->data_size;
	    break;

	case RPLAY_NULL:
	    rp->nsounds++;
	    rp->attrsp = &(*rp->attrsp)->next;
	    if (*packet == RPLAY_NULL)
	    {
		still_going = 0;
	    }
	    else
	    {
		*(rp->attrsp) = rplay_attrs_create();
		if (*(rp->attrsp) == NULL)
		{
		    rplay_errno = RPLAY_ERROR_MEMORY;
		    return NULL;
		}
	    }
	    break;

	default:
	    rplay_errno = RPLAY_ERROR_ATTRIBUTE;
#if 1
	    printf("unpack: unknown attr '%d'\n", *packet);
#endif
	    return NULL;
	}
    }

    return rp;
}

/*
 * create an rplay object for the given command
 *
 * currently all commands use the same internal representation but this can
 * be changed in the future
 */
#ifdef __STDC__
RPLAY *
rplay_create(int command)
#else
RPLAY *
rplay_create(command)
    int command;
#endif
{
    RPLAY *rp;

    rplay_errno = RPLAY_ERROR_NONE;

    rp = (RPLAY *) malloc(sizeof(RPLAY));
    if (rp == NULL)
    {
	rplay_errno = RPLAY_ERROR_MEMORY;
	return NULL;
    }

    rp->attrs = NULL;
    rp->attrsp = &rp->attrs;
    rp->buf = (char *) malloc(COPY_SIZE);
    if (rp->buf == NULL)
    {
	rplay_errno = RPLAY_ERROR_MEMORY;
	return NULL;
    }

    rp->len = 0;
    rp->size = 0;
    rp->command = RPLAY_NULL;
    rp->nsounds = 0;
    rp->count = RPLAY_DEFAULT_LIST_COUNT;
    rp->priority = RPLAY_DEFAULT_PRIORITY;
    rp->random_sound = RPLAY_DEFAULT_RANDOM_SOUND;
    rp->list_name = "";
    rp->id = RPLAY_NULL;
    rp->sequence = -1;
    rp->data = NULL;
    rp->data_size = 0;

    switch (command)
    {
    case RPLAY_PLAY:
    case RPLAY_STOP:
    case RPLAY_PAUSE:
    case RPLAY_CONTINUE:
    case RPLAY_PING:
    case RPLAY_RESET:
    case RPLAY_DONE:
    case RPLAY_PUT:
	rp->command = command;
	break;

    default:
	rplay_errno = RPLAY_ERROR_COMMAND;
	return NULL;
    }

    return rp;
}

/*
 * return the rplay attributes for the given index
 */
#ifdef __STDC__
static RPLAY_ATTRS *
get_attrs(RPLAY_ATTRS *attrs, int index)
#else
static RPLAY_ATTRS *
get_attrs(attrs, index)
    RPLAY_ATTRS *attrs;
    int index;
#endif
{
    int i;

    if (index < 0)
    {
	return NULL;
    }

    for (i = 0; i < index && attrs; i++, attrs = attrs->next) ;

    return attrs;
}

/*
 * set rplay attributes for the given rplay object
 *
 * The argument list should be NULL terminated but this is
 * not always checked.
 */
#ifdef __STDC__
long
rplay_set(RPLAY *rp,...)
#else
long
rplay_set(va_alist)
    va_dcl
#endif
{
    va_list args;
    RPLAY_ATTRS *attrs = NULL, *prev = NULL, *curr;
    int index, i, modifier;
    char *data;
    time_t seed;

#ifdef __STDC__
    va_start(args, rp);
#else
    RPLAY *rp;
    va_start(args);
    rp = va_arg(args, RPLAY *);
#endif

    rplay_errno = RPLAY_ERROR_NONE;

    modifier = va_arg(args, long);

    switch (modifier)
    {
    case RPLAY_APPEND:
	*(rp->attrsp) = attrs = rplay_attrs_create();
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_MEMORY;
	    return -1;
	}
	rp->attrsp = &attrs->next;
	rp->nsounds++;
	break;

    case RPLAY_INSERT:
	index = va_arg(args, long);
	if (index < 0)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	for (i = 0, curr = rp->attrs; i < index && curr; i++)
	{
	    prev = curr;
	    curr = curr->next;
	}
	if (curr == NULL && i != index)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	attrs = rplay_attrs_create();
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_MEMORY;
	    return -1;
	}
	if (prev)
	{
	    prev->next = attrs;
	}
	else
	{
	    *(rp->attrsp) = attrs;
	}
	attrs->next = curr;
	if (attrs->next == NULL)
	{
	    rp->attrsp = &attrs->next;
	}
	rp->nsounds++;
	break;

    case RPLAY_DELETE:
	index = va_arg(args, long);
	if (index < 0)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	for (i = 0, curr = rp->attrs; i < index && curr; i++)
	{
	    prev = curr;
	    curr = curr->next;
	}
	if (curr == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	if (prev)
	{
	    prev->next = curr->next;
	    if (prev->next == NULL)
	    {
		rp->attrsp = &prev->next;
	    }
	}
	else
	{
	    rp->attrs = curr->next;
	    if (rp->attrs == NULL)
	    {
		rp->attrsp = &rp->attrs;
	    }
	}
	rplay_attrs_destroy(curr);
	rp->nsounds--;
	break;

    case RPLAY_CHANGE:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	break;

    case RPLAY_LIST_COUNT:
	rp->count = va_arg(args, long);
	break;

    case RPLAY_LIST_NAME:
	if (*rp->list_name)
	{
	    free((char *) rp->list_name);
	}
	rp->list_name = strdup(va_arg(args, char *));
	break;

    case RPLAY_PRIORITY:
	rp->priority = va_arg(args, long);
	break;

    case RPLAY_RANDOM_SOUND:
	seed = time(0);
	srandom((int) seed);
	rp->random_sound = (int) (random() % rp->nsounds);
	break;

    case RPLAY_ID:
	rp->id = va_arg(args, long);
	break;

    case RPLAY_SEQUENCE:
	rp->sequence = va_arg(args, unsigned long);
	break;

    case RPLAY_DATA_SIZE:
	rplay_errno = RPLAY_ERROR_MODIFIER;
	return -1;

    case RPLAY_DATA:
	if (rp->data)
	{
	    free((char *) rp->data);
	}

	data = va_arg(args, char *);
	rp->data_size = va_arg(args, long);
	rp->data = (char *) malloc(rp->data_size);
	if (rp->data == NULL)
	{
	    rplay_errno = RPLAY_ERROR_MEMORY;
	    return -1;
	}
	memcpy(rp->data, data, rp->data_size);
	break;

    default:
	rplay_errno = RPLAY_ERROR_MODIFIER;
	return -1;
    }

    if (attrs)
    {
	int attribute;

	while ((attribute = va_arg(args, long)))
	{
	    switch (attribute)
	    {
	    case RPLAY_LIST_COUNT:
		rp->count = va_arg(args, long);
		break;

	    case RPLAY_LIST_NAME:
		if (*rp->list_name)
		{
		    free((char *) rp->list_name);
		}
		rp->list_name = strdup(va_arg(args, char *));
		break;

	    case RPLAY_PRIORITY:
		rp->priority = va_arg(args, long);
		break;

	    case RPLAY_SOUND:
		if (*attrs->sound)
		{
		    free((char *) attrs->sound);
		}
		attrs->sound = strdup(va_arg(args, char *));
		break;

	    case RPLAY_VOLUME:
		attrs->volume[0] = va_arg(args, long);
		attrs->volume[1] = attrs->volume[0];
		break;

	    case RPLAY_LEFT_VOLUME:
		attrs->volume[0] = va_arg(args, long);
		break;

	    case RPLAY_RIGHT_VOLUME:
		attrs->volume[1] = va_arg(args, long);
		break;

	    case RPLAY_COUNT:
		attrs->count = va_arg(args, long);
		break;

	    case RPLAY_RPTP_SERVER:
	    case RPLAY_RPTP_FROM_SENDER:
		{
		    struct hostent *hp;
		    u_long addr;
		    struct sockaddr_in s;
		    char *host = va_arg(args, char *);
		    char hostname[MAXHOSTNAMELEN];

		    if (attribute == RPLAY_RPTP_FROM_SENDER)
		    {
			if (gethostname(hostname, sizeof(hostname)) < 0)
			{
			    rplay_errno = RPLAY_ERROR_HOST;
			    return -1;
			}
			host = hostname;
		    }
		    else
		    {
			host = va_arg(args, char *);
		    }

		    memset((char *) &s, 0, sizeof(s));
		    addr = inet_addr(host);
		    if (addr == 0xffffffff)
		    {
			hp = gethostbyname(host);
			if (hp == NULL)
			{
			    rplay_errno = RPLAY_ERROR_HOST;
			    return -1;
			}
			memcpy((char *) &s.sin_addr.s_addr, (char *) hp->h_addr, hp->h_length);
		    }
		    else
		    {
			memcpy((char *) &s.sin_addr.s_addr, (char *) &addr, sizeof(addr));
		    }
		    attrs->rptp_server = strdup((char *) inet_ntoa(s.sin_addr));
		    break;
		}

	    case RPLAY_RPTP_SERVER_PORT:
		attrs->rptp_server_port = (unsigned short) va_arg(args, long);
		break;

	    case RPLAY_RPTP_SEARCH:
		attrs->rptp_search = va_arg(args, long);
		break;

	    case RPLAY_SAMPLE_RATE:
		attrs->sample_rate = va_arg(args, unsigned long);
		break;

	    case RPLAY_CLIENT_DATA:
		if (*attrs->client_data)
		{
		    free((char *) attrs->client_data);
		}
		attrs->client_data = strdup(va_arg(args, char *));
		break;

	    default:
		rplay_errno = RPLAY_ERROR_ATTRIBUTE;
		return -1;
	    }
	}
    }

    return rplay_pack(rp);
}

/*
 * retrieve rplay attributes from the given rplay object
 */
#ifdef __STDC__
long
rplay_get(RPLAY *rp,...)
#else
long
rplay_get(va_alist)
    va_dcl
#endif
{
    va_list args;
    RPLAY_ATTRS *attrs;
    int get, index;

#ifdef __STDC__
    va_start(args, rp);
#else
    RPLAY *rp;
    va_start(args);
    rp = va_arg(args, RPLAY *);
#endif
    rplay_errno = RPLAY_ERROR_NONE;

    get = va_arg(args, long);

    switch (get)
    {
    case RPLAY_NSOUNDS:
	return rp->nsounds;

    case RPLAY_COMMAND:
	return rp->command;

    case RPLAY_LIST_COUNT:
	return rp->count;

    case RPLAY_LIST_NAME:
	return (long) rp->list_name;

    case RPLAY_PRIORITY:
	return rp->priority;

    case RPLAY_RANDOM_SOUND:
	return rp->random_sound;

    case RPLAY_ID:
	return rp->id;

    case RPLAY_SEQUENCE:
	return rp->sequence;

    case RPLAY_DATA_SIZE:
	return rp->data_size;

    case RPLAY_DATA:
	return (long) rp->data;

    case RPLAY_SOUND:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return (long) attrs->sound;

    case RPLAY_VOLUME:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return MAX(attrs->volume[0], attrs->volume[1]);

    case RPLAY_LEFT_VOLUME:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return attrs->volume[0];

    case RPLAY_RIGHT_VOLUME:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return attrs->volume[1];

    case RPLAY_COUNT:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return attrs->count;

    case RPLAY_RPTP_SERVER:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return (long) attrs->rptp_server;

    case RPLAY_RPTP_SERVER_PORT:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return (long) attrs->rptp_server_port;

    case RPLAY_RPTP_SEARCH:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return attrs->rptp_search;

    case RPLAY_SAMPLE_RATE:
	index = va_arg(args, long);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return attrs->sample_rate;

    case RPLAY_CLIENT_DATA:
	index = va_arg(args, int);
	attrs = get_attrs(rp->attrs, index);
	if (attrs == NULL)
	{
	    rplay_errno = RPLAY_ERROR_INDEX;
	    return -1;
	}
	return (long) attrs->client_data;

    default:
	rplay_errno = RPLAY_ERROR_ATTRIBUTE;
	return -1;
    }
}

/*
 * free memory used by the given rplay object
 */
#ifdef __STDC__
void
rplay_destroy(RPLAY *rp)
#else
void
rplay_destroy(rp)
    RPLAY *rp;
#endif
{
    RPLAY_ATTRS *p, *q;

    rplay_errno = RPLAY_ERROR_NONE;

    for (p = rp->attrs; p; q = p, p = p->next, rplay_attrs_destroy(q)) ;
    if (*rp->list_name)
    {
	free((char *) rp->list_name);
    }
    if (rp->data)
    {
	free((char *) rp->data);
    }
    free((char *) rp->buf);
    free((char *) rp);
}

/*
 * support routine to convert rplay 2.0 packets to rplay 3.0
 */
#ifdef OLD_RPLAY
#ifdef __STDC__
char *
rplay_convert(char *p)
#else
char *
rplay_convert(p)
    char *p;
#endif
{
    static char buf[MAX_PACKET];
    char *q = buf;
    int len;

    *q++ = RPLAY_PACKET_ID;
    switch (*p++)
    {
    case OLD_RPLAY_PLAY:
	*q++ = RPLAY_PLAY;
	break;

    case OLD_RPLAY_STOP:
	*q++ = RPLAY_STOP;
	break;

    case OLD_RPLAY_PAUSE:
	*q++ = RPLAY_PAUSE;
	break;

    case OLD_RPLAY_CONTINUE:
	*q++ = RPLAY_CONTINUE;
	break;
    }

    do
    {
	*q++ = RPLAY_SOUND;
	strcpy(q, p);
	len = strlen(p) + 1;
	p += len;
	q += len;
	*q++ = RPLAY_VOLUME;
	*q++ = *p++;
	*q++ = RPLAY_NULL;
    }
    while (*p);

    *q++ = RPLAY_NULL;

    return buf;
}
#endif /* OLD_RPLAY */

static int
default_rplay_port()
{
    struct servent *sp;
    int port;

    sp = getservbyname("rplay", "udp");
    if (sp)
    {
	port = ntohs(sp->s_port);	/* htons is used later */
    }
    else
    {
	port = RPLAY_PORT;
    }

    return port;
}

#ifdef __STDC__
int
rplay_open(char *host)
#else
int
rplay_open(host)
    char *host;
#endif
{
    int port;

    port = default_rplay_port();

    return rplay_open_port(host, port);
}

/*
 * Open a UDP socket for the given host and port.
 *
 * The host can be either a name or an IP address.
 */
#ifdef __STDC__
int
rplay_open_port(char *host, int port)
#else
int
rplay_open_port(host, port)
    char *host;
    int port;
#endif
{
    struct hostent *hp;
    u_long addr;
    struct sockaddr_in s;

    rplay_errno = RPLAY_ERROR_NONE;

    memset((char *) &s, 0, sizeof(s));

    addr = inet_addr(host);
    if (addr == 0xffffffff)
    {
	hp = gethostbyname(host);
	if (hp == NULL)
	{
	    rplay_errno = RPLAY_ERROR_HOST;
	    return -1;
	}
	memcpy((char *) &s.sin_addr.s_addr, (char *) hp->h_addr, hp->h_length);
    }
    else
    {
	memcpy((char *) &s.sin_addr.s_addr, (char *) &addr, sizeof(addr));
    }

    s.sin_port = htons(port);
    s.sin_family = AF_INET;

    return rplay_open_sockaddr_in(&s);
}

#ifdef __STDC__
int
rplay_open_sockaddr_in(struct sockaddr_in *saddr)
#else
int
rplay_open_sockaddr_in(saddr)
    struct sockaddr_in *saddr;
#endif
{
    int rplay_fd;
    int on = 1;

    rplay_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (rplay_fd < 0)
    {
	rplay_errno = RPLAY_ERROR_SOCKET;
	return -1;
    }

    /*
     * enable broadcasting
     */
    if (setsockopt(rplay_fd, SOL_SOCKET, SO_BROADCAST, (char *) &on, sizeof(on)) < 0)
    {
	rplay_errno = RPLAY_ERROR_BROADCAST;
	return -1;
    }

    if (connect(rplay_fd, (struct sockaddr *) saddr, sizeof(*saddr)) < 0)
    {
	rplay_errno = RPLAY_ERROR_CONNECT;
	return -1;
    }

    return rplay_fd;
}

/*
 * send an rplay packet to a host
 */
#ifdef __STDC__
int
rplay(int rplay_fd, RPLAY *rp)
#else
int
rplay(rplay_fd, rp)
    int rplay_fd;
    RPLAY *rp;
#endif
{
    rplay_errno = RPLAY_ERROR_NONE;

    if (write(rplay_fd, rp->buf, rp->len) != rp->len)
    {
	rplay_errno = RPLAY_ERROR_WRITE;
	return -1;
    }

    return 0;
}

/*
 * close the socket
 */
#ifdef __STDC__
int
rplay_close(int rplay_fd)
#else
int
rplay_close(rplay_fd)
    int rplay_fd;
#endif
{
    rplay_errno = RPLAY_ERROR_NONE;

    if (close(rplay_fd) < 0)
    {
	rplay_errno = RPLAY_ERROR_CLOSE;
	return -1;
    }

    return 0;
}

/*
 * report rplay error messages
 */
#ifdef __STDC__
void
rplay_perror(char *s)
#else
void
rplay_perror(s)
    char *s;
#endif
{
    fprintf(stderr, "%s: %s\n", s, rplay_errlist[rplay_errno]);
}

/*
 * open a socket using the current X display
 * (uses the DISPLAY environment variable)
 */
#ifdef __STDC__
int
rplay_open_display(void)
#else
int
rplay_open_display()
#endif
{
    char *display, *p;
    char host[MAXHOSTNAMELEN];

    display = getenv("DISPLAY");
    if (display == NULL || display[0] == ':')
    {
	strcpy(host, "localhost");
    }
    else
    {
	strcpy(host, display);
	p = strchr(host, ':');
	if (p)
	{
	    *p = '\0';
	}
	if (strcmp(host, "unix") == 0 || strcmp(host, "local") == 0 || strcmp(host, "X") == 0)
	{
	    strcpy(host, "localhost");
	}
    }

    return rplay_open(host);
}

/*
 * play a sound on the current X display
 */
#ifdef __STDC__
int
rplay_display(char *sound)
#else
int
rplay_display(sound)
    char *sound;
#endif
{
    int rplay_fd;

    rplay_fd = rplay_open_display();
    if (rplay_fd < 0)
    {
	return -1;
    }

    return rplay_sound(rplay_fd, sound);
}

/*
 * play a sound on the localhost
 */
#ifdef __STDC__
int
rplay_local(char *sound)
#else
int
rplay_local(sound)
    char *sound;
#endif
{
    return rplay_host("localhost", sound);
}

/*
 * play a sound on a host
 */
#ifdef __STDC__
int
rplay_host(char *host, char *sound)
#else
int
rplay_host(host, sound)
    char *host;
    char *sound;
#endif
{
    int rplay_fd;

    rplay_fd = rplay_open(host);
    if (rplay_fd < 0)
    {
	return -1;
    }

    return rplay_sound(rplay_fd, sound);
}

/*
 * play a sound given a socket
 */
#ifdef __STDC__
int
rplay_sound(int rplay_fd, char *sound)
#else
int
rplay_sound(rplay_fd, sound)
    int rplay_fd;
    char *sound;
#endif
{
    RPLAY *rp;

    rp = rplay_create(RPLAY_PLAY);
    if (rp == NULL)
    {
	return -1;
    }

    if (rplay_set(rp, RPLAY_APPEND, RPLAY_SOUND, sound, NULL) < 0)
    {
	return -1;
    }

    if (rplay(rplay_fd, rp) < 0)
    {
	return -1;
    }

    rplay_destroy(rp);

    return 0;
}

/*
 * ping an rplay server
 */
#ifdef __STDC__
int
rplay_ping(char *host)
#else
int
rplay_ping(host)
    char *host;
#endif
{
    int rplay_fd;
    int port;
    int error1 = 0, error2 = 0;

    /* Ping the default port.  */
    port = default_rplay_port();
    rplay_fd = rplay_open_port(host, port);
    if (rplay_fd < 0)
    {
	return -1;
    }
    error1 = rplay_ping_sockfd(rplay_fd);

#ifdef OTHER_RPLAY_PORTS
    /* Pick an alternative port.  */
    if (port == RPLAY_PORT)
    {
	port = OLD_RPLAY_PORT;
    }
    else
    {
	port = RPLAY_PORT;
    }

    /* Ping the alternative port.  */
    rplay_fd = rplay_open_port(host, port);
    if (rplay_fd < 0)
    {
	return -1;
    }
    error2 = rplay_ping_sockfd(rplay_fd);
#endif /* OTHER_RPLAY_PORTS */

    /* Only return -1 if both pings fail. */
    if (error1 < 0 && error2 < 0)
    {
	return -1;
    }
    else
    {
	return 0;
    }
}

#ifdef __STDC__
int
rplay_ping_sockfd(int rplay_fd)
#else
int
rplay_ping_sockfd(rplay_fd)
    int rplay_fd;
#endif
{
    RPLAY *rp;

    rp = rplay_create(RPLAY_PING);
    if (rp == NULL)
    {
	return -1;
    }

    if (rplay_pack(rp) < 0)
    {
	return -1;
    }

    if (rplay(rplay_fd, rp) < 0)
    {
	return -1;
    }

    rplay_close(rplay_fd);
    rplay_destroy(rp);

    return 0;
}

#ifdef __STDC__
int
rplay_ping_sockaddr_in(struct sockaddr_in *saddr)
#else
int
rplay_ping_sockaddr_in(saddr)
    struct sockaddr_in *saddr;
#endif
{
    int rplay_fd;

    rplay_fd = rplay_open_sockaddr_in(saddr);
    if (rplay_fd < 0)
    {
	return -1;
    }

    return rplay_ping_sockfd(rplay_fd);
}

/*
 * play a sound on a host with a specific volume
 */
#ifdef __STDC__
int
rplay_host_volume(char *host, char *sound, int volume)
#else
int
rplay_host_volume(host, sound, volume)
    char *host;
    char *sound;
    int volume;
#endif
{
    int rplay_fd;
    RPLAY *rp;

    rplay_fd = rplay_open(host);
    if (rplay_fd < 0)
    {
	return -1;
    }

    rp = rplay_create(RPLAY_PLAY);
    if (rp == NULL)
    {
	return -1;
    }

    if (rplay_set(rp, RPLAY_APPEND, RPLAY_SOUND, sound, RPLAY_VOLUME, volume, NULL) < 0)
    {
	return -1;
    }

    if (rplay(rplay_fd, rp) < 0)
    {
	return -1;
    }

    rplay_destroy(rp);

    return 0;
}

/*
 * obtain the default rplay host
 */
#ifdef __STDC__
char *
rplay_default_host(void)
#else
char *
rplay_default_host()
#endif
{
    char *host;

    host = getenv("RPLAY_HOST");

    return host ? host : "localhost";
}

/*
 * play a sound on the default rplay host
 */
#ifdef __STDC__
int
rplay_default(char *sound)
#else
int
rplay_default(sound)
    char *sound;
#endif
{
    return rplay_host(rplay_default_host(), sound);
}

/*
 * open the default host
 */
#ifdef __STDC__
int
rplay_open_default(void)
#else
int
rplay_open_default()
#endif
{
    return rplay_open(rplay_default_host());
}
