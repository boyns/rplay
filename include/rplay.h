/* $Id: rplay.h,v 1.2 1998/08/13 06:13:26 boyns Exp $ */

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


#ifndef _rplay_h
#define _rplay_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#undef FALSE
#define FALSE	0
#undef TRUE
#define TRUE	1

#define RPLAY_PORT		5555
#define RPTP_PORT		5556

#define OLD_RPLAY_PORT		55555
#define OLD_RPTP_PORT		55556

#define RPLAY_PACKET_ID		30	/* id sent with rplay 3.x packets */

/* Attributes: */
#define RPLAY_NULL		0
#define RPLAY_PLAY		1
#define RPLAY_STOP		2
#define RPLAY_PAUSE		3
#define RPLAY_CONTINUE		4
#define RPLAY_SOUND		5
#define RPLAY_VOLUME		6
#define RPLAY_NSOUNDS		7
#define RPLAY_COMMAND		8
#define RPLAY_APPEND		9
#define RPLAY_INSERT		10
#define RPLAY_DELETE		11
#define RPLAY_CHANGE		12
#define RPLAY_COUNT		13
#define RPLAY_LIST_COUNT	14
#define RPLAY_PRIORITY		15
#define RPLAY_RANDOM_SOUND	16
#define RPLAY_PING		17
#define RPLAY_RPTP_SERVER	18
#define RPLAY_RPTP_SERVER_PORT	19
#define RPLAY_RPTP_SEARCH	20
#define RPLAY_RPTP_FROM_SENDER	21
#define RPLAY_SAMPLE_RATE	22
#define RPLAY_RESET		23
#define RPLAY_DONE		24
#define RPLAY_CLIENT_DATA	25
#define RPLAY_LIST_NAME		26
#define RPLAY_PUT		27
#define RPLAY_ID		28
#define RPLAY_SEQUENCE		29
#define RPLAY_DATA		30
#define RPLAY_DATA_SIZE		31

/* audio formats */
#define RPLAY_FORMAT_NONE	0
#define RPLAY_FORMAT_LINEAR_8	1 /* 8-bit linear PCM */
#define RPLAY_FORMAT_ULINEAR_8	2 /* 8-bit unsigned linear PCM */
#define RPLAY_FORMAT_LINEAR_16	3 /* 16-bit linear PCM */
#define RPLAY_FORMAT_ULINEAR_16	4 /* 16-bit unsigned linear PCM */
#define RPLAY_FORMAT_ULAW	5 /* 8-bit ISDN u-law */
#define RPLAY_FORMAT_G721	6 /* CCITT G.721 4-bits ADPCM */
#define RPLAY_FORMAT_G723_3	7 /* CCITT G.723 3-bits ADPCM */
#define RPLAY_FORMAT_G723_5	8 /* CCITT G.723 5-bits ADPCM */
#define RPLAY_FORMAT_GSM	9 /* GSM 0.610 13 kbit/s RPE/LTP speech compression */

/* audio byte order */
#define RPLAY_BIG_ENDIAN	1
#define RPLAY_LITTLE_ENDIAN	2

/* Audio ports: */
#define RPLAY_AUDIO_PORT_NONE		(1<<0)
#define RPLAY_AUDIO_PORT_SPEAKER	(1<<1)
#define RPLAY_AUDIO_PORT_HEADPHONE	(1<<2)
#define RPLAY_AUDIO_PORT_LINEOUT	(1<<3)

/* Attribute restrictions: */
#define RPLAY_MIN_VOLUME		0
#define RPLAY_MAX_VOLUME		255
#define RPLAY_MIN_PRIORITY		0
#define RPLAY_MAX_PRIORITY		255

/* Attribute defaults: */
#define RPLAY_DEFAULT_VOLUME		127
#define RPLAY_DEFAULT_PRIORITY		0
#define RPLAY_DEFAULT_COUNT		1
#define RPLAY_DEFAULT_LIST_COUNT	1
#define RPLAY_DEFAULT_RANDOM_SOUND	-1
#define RPLAY_DEFAULT_SAMPLE_RATE	0
#define RPLAY_DEFAULT_OFFSET		0
#define RPLAY_DEFAULT_BYTE_ORDER	0
#define RPLAY_DEFAULT_CHANNELS		0
#define RPLAY_DEFAULT_BITS		0

/* RPLAY errors used by rplay_errno: */
#define RPLAY_ERROR_NONE	0
#define RPLAY_ERROR_MEMORY	1
#define RPLAY_ERROR_HOST	2
#define RPLAY_ERROR_CONNECT	3
#define RPLAY_ERROR_SOCKET	4
#define RPLAY_ERROR_WRITE	5
#define RPLAY_ERROR_CLOSE	6
#define RPLAY_ERROR_PACKET_SIZE	7
#define RPLAY_ERROR_BROADCAST	8
#define RPLAY_ERROR_ATTRIBUTE	9
#define RPLAY_ERROR_COMMAND	10
#define RPLAY_ERROR_INDEX	11
#define RPLAY_ERROR_MODIFIER	12

/* RPTP errors used by rptp_errno: */
#define RPTP_ERROR_NONE		0
#define RPTP_ERROR_MEMORY	1
#define RPTP_ERROR_HOST		2
#define RPTP_ERROR_CONNECT	3
#define RPTP_ERROR_SOCKET	4
#define RPTP_ERROR_OPEN		5
#define RPTP_ERROR_READ		6
#define RPTP_ERROR_WRITE	7
#define RPTP_ERROR_PING		8
#define RPTP_ERROR_TIMEOUT	9
#define RPTP_ERROR_PROTOCOL	10

/* RPTP response types: */
#define RPTP_ERROR		'-'
#define RPTP_OK			'+'
#define RPTP_TIMEOUT		'!'
#define RPTP_NOTIFY		'@'

/* RPLAY 2.0 support: */
#define OLD_RPLAY_PLAY		1
#define OLD_RPLAY_STOP		2
#define OLD_RPLAY_PAUSE		3
#define OLD_RPLAY_CONTINUE	4

/* Definitions for the RPTP asynchronous I/O system: */
#define	RPTP_ASYNC_READ		1
#define	RPTP_ASYNC_WRITE	2
#define	RPTP_ASYNC_RAW		4
#define	RPTP_ASYNC_ENABLE	1
#define	RPTP_ASYNC_DISABLE	2

/* These event can be used to specify a mask: */
#define	RPTP_EVENT_OK		(1 << 0)
#define	RPTP_EVENT_ERROR	(1 << 1)
#define	RPTP_EVENT_TIMEOUT	(1 << 2)
#define	RPTP_EVENT_OTHER	(1 << 3)
#define	RPTP_EVENT_CONTINUE	(1 << 4)
#define	RPTP_EVENT_DONE		(1 << 5)
#define	RPTP_EVENT_PAUSE	(1 << 6)
#define	RPTP_EVENT_PLAY		(1 << 7)
#define	RPTP_EVENT_SKIP		(1 << 8)
#define	RPTP_EVENT_STATE	(1 << 9)
#define	RPTP_EVENT_STOP		(1 << 10)
#define	RPTP_EVENT_VOLUME	(1 << 11)
#define	RPTP_EVENT_CLOSE	(1 << 12)
#define	RPTP_EVENT_FLOW		(1 << 13)
#define	RPTP_EVENT_MODIFY	(1 << 14)
#define	RPTP_EVENT_LEVEL	(1 << 15)
#define RPTP_EVENT_POSITION	(1 << 16)
#define	RPTP_EVENT_ALL		0x0000ffff

/* Size restrictions for RPTP: */
#define RPTP_MAX_LINE	1024
#define RPTP_MAX_ARGS	32

/* rplay object attributes: */
typedef struct _rplay_attrs
{
    struct _rplay_attrs *next;
    char *sound;
    int volume;
    int count;
    char *rptp_server;
    unsigned short rptp_server_port;
    int rptp_search;
    unsigned long sample_rate;
    char *client_data;
}
RPLAY_ATTRS;

/* the rplay object */
typedef struct _rplay
{
    struct _rplay_attrs *attrs;
    struct _rplay_attrs **attrsp;
    char *buf;
    int len;
    int size;
    int command;
    int nsounds;
    int count;
    int priority;
    int random_sound;
    char *list_name;
    int id;
    unsigned long sequence;
    unsigned short data_size;
    char *data;
}
RPLAY;

extern int rplay_errno;
extern int rptp_errno;
extern char *rplay_errlist[];
extern char *rptp_errlist[];

#ifdef __cplusplus
#ifndef __STDC__
#define __STDC__
#endif
extern "C"
{
#endif

#ifdef __STDC__
extern void * xmalloc(size_t size);
extern RPLAY *rplay_create (int rplay_command);
extern char *rplay_convert (char *buf);
extern int rplay_pack (RPLAY * rp);
extern RPLAY *rplay_unpack (char *buf);
extern void rplay_destroy (RPLAY * rp);
extern long rplay_set (RPLAY *,...);
extern long rplay_get (RPLAY *,...);
extern int rplay_open (char *host);
extern int rplay_open_port (char *host, int port);
extern int rplay_open_sockaddr_in (struct sockaddr_in *saddr);
extern int rplay (int rplay_fd, RPLAY * rp);
extern int rplay_close (int rplay_fd);
extern void rplay_perror (char *message);
extern int rplay_open_display (void);
extern int rplay_display (char *sound);
extern int rplay_local (char *sound);
extern int rplay_host (char *host, char *sound);
extern int rplay_host_volume (char *host, char *sound, int volume);
extern int rplay_sound (int rplay_fd, char *sound);
extern int rplay_ping (char *host);
extern int rplay_ping_sockaddr_in (struct sockaddr_in *saddr);
extern int rplay_ping_sockfd (int sock_fd);
extern char *rplay_default_host (void);
extern int rplay_default (char *sound);
extern int rplay_open_default (void);
extern int rptp_open (char *host, int port, char *response, int response_size);
extern int rptp_read (int rptp_fd, char *buf, int nbytes);
extern int rptp_write (int rptp_fd, char *buf, int nbytes);
extern int rptp_close (int rptp_fd);
extern void rptp_perror (char *message);
extern int rptp_putline (int rptp_fd, char *fmt,...);
extern int rptp_getline (int rptp_fd, char *buf, int nbytes);
extern int rptp_command (int rptp_fd, char *command, char *response, int response_size);
extern char *rptp_parse (char *response, char *name);
extern int rptp_async_putline (int rptp_fd, void (*callback)(), char *fmt, ...);
extern int rptp_async_write (int rptp_fd, void (*callback)(), char *ptr, int nbytes);
extern void rptp_async_register(int rptp_fd, int what, void (*callback)());
extern void rptp_async_notify (int rptp_fd, int mask, void (*callback)());
extern void rptp_async_process (int rptp_fd, int what);
extern int rptp_main_loop (void);
extern void rptp_stop_main_loop (int);
#else
extern RPLAY *rplay_create ( /* int rplay_command */ );
extern char *rplay_convert ( /* char *buf */ );
extern int rplay_pack ( /* RPLAY *rp */ );
extern RPLAY *rplay_unpack ( /* char *buf */ );
extern void rplay_destroy ( /* RPLAY *rp */ );
extern int rplay_set ( /* RPLAY *, ... */ );
extern int rplay_get ( /* RPLAY *, ... */ );
extern int rplay_open ( /* char *host */ );
extern int rplay_open_port ( /* char *host, int port */ );
extern int rplay_open_sockaddr_in ( /* struct sockaddr_in *saddr */ );
extern int rplay ( /* int rplay_fd, RPLAY *rp */ );
extern int rplay_close ( /* int rplay_fd */ );
extern void rplay_perror ( /* char *message */ );
extern int rplay_open_display ();
extern int rplay_display ( /* char *sound */ );
extern int rplay_local ( /* char *sound */ );
extern int rplay_host ( /* char *host, char *sound */ );
extern int rplay_host_volume ( /* char *host, char *sound, int volume */ );
extern int rplay_sound ( /* int rplay_fd, char *sound */ );
extern int rplay_ping ( /* char *host */ );
extern int rplay_ping_sockaddr_in ( /* struct sockaddr_in *saddr */ );
extern int rplay_ping_sockfd ( /* int sock_fd */ );
extern char *rplay_default_host ();
extern int rplay_default ( /* char *sound */ );
extern int rplay_open_default ();
extern int rptp_open ( /* char *host, int port, char *response, int response_size */ );
extern int rptp_read ( /* int rptp_fd, char *buf, int nbytes */ );
extern int rptp_write ( /* int rptp_fd, char *buf, int nbytes */ );
extern int rptp_close ( /* int rptp_fd */ );
extern void rptp_perror ( /* char *message */ );
extern int rptp_putline ( /* int rptp_fd, char *fmt, ... */ );
extern int rptp_getline ( /* int rptp_fd, char *buf, int nbytes */ );
extern int rptp_command ( /* int rptp_fd, char *command, char *response, int response_size */ );
extern char *rptp_parse ( /* char *response, char *name */ );
extern int rptp_async_putline ( /* int rptp_fd, void (callback *)(), char *fmt, ... */ );
extern int rptp_async_write ( /* int rptp_fd, void (callback *)(), char *ptr, int nbytes */ );
extern void rptp_async_register (/* int rptp_fd, int what, void (callback*)() */ );
extern void rptp_async_notify ( /* int rptp_fd, int what, void (*callback)() */ );
extern void rptp_async_process ( /* int rptp_fd, int what */ );
extern int rptp_main_loop ();
extern void rptp_stop_main_loop ();
#endif

#ifdef __cplusplus
}
#endif

#endif /* _rplay_h */
