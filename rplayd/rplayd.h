/* $Id: rplayd.h,v 1.4 2002/12/11 05:12:16 boyns Exp $ */

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

#ifndef _rplayd_h
#define _rplayd_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include "rplay.h"
#include "misc.h"
#include "audio.h"
#include "buffer.h"
#include "spool.h"

/* Report levels */
#define REPORT_NONE	0	/* Nothing. */
#define REPORT_ERROR	1	/* System and rplayd errors. */
#define REPORT_NOTICE	2	/* ... + RPTP connections, get, put, find */
#define REPORT_INFO	3	/* ... + play, stop, pause, continue, etc */
#define REPORT_DEBUG	4	/* ... + Debug messages. */

#define REPORT_MIN	0
#define REPORT_MAX	4

/* $Id: rplayd.h,v 1.4 2002/12/11 05:12:16 boyns Exp $ */
#define SET_BIT(w, bit)         ( (w) |= (bit) )
#define CLR_BIT(w, bit)         ( (w) &= ~(bit) )
#define BIT(w, bit)             ( (w) & (bit) )
#define TOGGLE_BIT(w, bit)      ( (w) ^= (bit) )

/* UIO_MAXIOV probably isn't defined so lets assume it's 16. */
#ifndef UIO_MAXIOV
#define UIO_MAXIOV 16
#endif

/* Table of audio parameters supported by the audio device. */
typedef struct
{
    int sample_rate;
    int format;
    int bits;
    int channels;
}
RPLAY_AUDIO_TABLE;
extern RPLAY_AUDIO_TABLE *rplay_audio_table;


/* Global variables that are declared in rplayd.c.
   (See rplayd.c for descriptions) */

extern fd_set read_mask;
extern fd_set write_mask;
extern int debug;
extern int inetd;
extern int rptp_timeout;
extern char hostname[];
extern char *hostaddr;
extern int rplay_fd;
extern int rptp_fd;
#ifdef OTHER_RPLAY_PORTS
extern int other_rplay_fd;
extern int other_rptp_fd;
#endif
extern int curr_rate;
#ifndef HAVE_OSS
extern int curr_bufsize;
#endif /* !HAVE_OSS */
extern int rplayd_pid;
extern time_t starttime;
#ifdef AUTH
extern int auth_enabled;
#endif /* AUTH */
extern int rplay_priority_threshold;
extern char *rplay_audio_device;
#ifndef HAVE_OSS
extern int rplay_audio_bufsize;
#endif /* !HAVE_OSS */
extern int rplay_audio_rate;
extern int rplay_audio_sample_rate;
extern int rplay_audio_channels;
extern int rplay_audio_format;
extern int rplay_audio_precision;
extern int rplay_audio_port;
extern int audio_enabled;
extern int rplay_audio_volume;
extern int rplay_audio_match;
extern int rplay_audio_size;
extern char *rplay_audio_buf;
extern int max_rplay_audio_bufsize;
extern int rplay_audio_left_level;
extern int rplay_audio_right_level;

extern int optional_sample_rate;
extern int optional_precision;
extern int optional_channels;
extern int optional_format;
extern int optional_port;

extern int default_sample_rate;
extern int default_precision;
extern int default_channels;
extern int default_format;
extern int default_port;

#ifdef HAVE_OSS
extern int rplay_audio_fragsize;
extern int optional_fragsize;
extern int default_fragsize;
#endif /* HAVE_OSS */

extern int monitor_count;
extern BUFFER *monitor_buffers;

extern int errno;

#ifdef __STDC__
extern void usage ();
extern void done (int exit_value);
extern unsigned long inet_addr (char *);
extern char *inet_ntoa (struct in_addr);
extern void report (int level_mask, char *fmt,...);
extern void rplayd_read (int fd);
extern void rplayd_write (int nbytes);
extern void rplayd_init ();
extern int rplayd_play (RPLAY *rp, struct sockaddr_in sin);
extern int rplayd_stop (RPLAY *rp, struct sockaddr_in sin);
extern int rplayd_pause (RPLAY *rp, struct sockaddr_in sin);
extern int rplayd_continue (RPLAY *rp, struct sockaddr_in sin);
extern int rplayd_done (RPLAY *rp, struct sockaddr_in sin);
extern int rplayd_put (RPLAY *rp, struct sockaddr_in sin);
extern BUFFER *rplayd_status ();
extern void need_reset ();
extern int rplayd_audio_init ();
extern void rplayd_audio_match (SPOOL *sp);
extern void monitor_alloc();
#else
extern void usage ();
extern void done ( /* int exit_value */ );
extern unsigned long inet_addr ( /* char * */ );
extern char *inet_ntoa ( /* struct in_addr */ );
extern void report ( /* int level_mask, char *fmt, ... */ );
extern void rplayd_read ( /* int fd */ );
extern void rplayd_write ( /* int nbytes */ );
extern void rplayd_init ();
extern int rplayd_play ( /* RPLAY *rp, struct sockaddr_in sin */ );
extern int rplayd_stop ( /* RPLAY *rp, struct sockaddr_in sin */ );
extern int rplayd_pause ( /* RPLAY *rp, struct sockaddr_in sin */ );
extern int rplayd_continue ( /* RPLAY *rp, struct sockaddr_in sin */ );
extern int rplayd_done ( /* RPLAY *rp, struct sockaddr_in sin */ );
extern int rplayd_put ( /* RPLAY *rp, struct sockaddr_in sin */ );
extern BUFFER *rplayd_status ();
extern void need_reset ();
extern int rplayd_audio_init ();
extern void rplayd_audio_match ( /* SPOOL *sp */ );
extern void monitor_alloc();
#endif

/*
 * Prototypes for the audio stubs:
 */
#ifdef __STDC__
extern int rplay_audio_init ();
extern int rplay_audio_open ();
extern int rplay_audio_isopen ();
extern int rplay_audio_flush ();
extern int rplay_audio_close ();
extern int rplay_audio_write (char *buf, int nbytes);
extern int rplay_audio_audio_get_volume ();
extern int rplay_audio_audio_set_volume (int volume);
extern int rplay_audio_get_volume ();
extern int rplay_audio_set_volume (int volume);
extern int rplay_audio_set_port();
#ifdef linux
extern int rplay_audio_setfragsize(int fragsize);
#endif /* linux */
#else
extern int rplay_audio_init ();
extern int rplay_audio_open ();
extern int rplay_audio_isopen ();
extern int rplay_audio_flush ();
extern int rplay_audio_close ();
extern int rplay_audio_write ( /* char *buf, int nbytes */ );
extern int rplay_audio_audio_get_volume ();
extern int rplay_audio_audio_set_volume ( /* int volume */ );
extern int rplay_audio_get_volume ();
extern int rplay_audio_set_volume ( /* int volume */ );
extern int rplay_audio_set_port();
#ifdef linux
extern int rplay_audio_setfragsize(/*int fragsize*/);
#endif /* linux */
#endif

#endif /* _rplayd_h */
