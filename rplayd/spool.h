/* $Id: spool.h,v 1.3 1999/03/10 07:58:04 boyns Exp $ */

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



#ifndef _spool_h
#define _spool_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include "rplay.h"
#include "sound.h"
#include "buffer.h"

#define SPOOL_NULL	0	/* nothing */
#define SPOOL_PLAY	1	/* Spool entry is playing. */
#define SPOOL_PAUSE	2	/* Spool entry is plaused. */
#define SPOOL_WAIT	3	/* Spool entry is waiting for more data. */
#define SPOOL_NEXT	4	/* Current sound is done. */
#define SPOOL_SKIP	5	/* Current sound has been skipped. */

#define SPOOL_MIN_ID	1
#define SPOOL_MAX_ID	999

typedef struct _spool
{
    struct _spool *next;
    struct _spool *prev;
    int id;			/* unique spool id */
    int state;			/* state of spool entry */
    time_t time;		/* time since data was last read */
    int curr_sound;		/* current sound */
    int curr_count;		/* # times left to play curr_sound */
    int list_count;		/* # times left to play the sound list */
    SOUND *sound[SOUND_LIST_SIZE];	/* list of sounds */
    SINDEX *si;			/* Index of the current sound. */
    BUFFER *curr_buffer;	/* Buffer of audio that's currently playing. */
    BUFFER *next_buffer;	/* Buffer of audio that will be played next. */
    int offset;			/* Position in the current audio file. */
    unsigned char *ptr;		/* Pointer to the current sample. */
    unsigned char *ptr_end;	/* The end of the current buffer. */
    RPLAY *rp;			/* RPLAY packet received */
    RPLAY_ATTRS *curr_attrs;	/* current rplay attributes */
    struct sockaddr_in sin;	/* client's address */
    unsigned long sample_rate;	/* sample rate of curr_sound */
    double sample_index;	/* Must use float to avoid rounding errors. */
    double sample_factor;	/* Factor to increase the sample index by. */
    double oversample[2];	/* The current sample value when oversampling */
    double oversample_inc[2];	/* The delta to increase the oversample with */
    int (*to_native) ();	/* Convert curr_sound samples to native.  */
    int skip_count;		/* skip curr_sound */
    int auto_pause;		/* pause before play */
    int notify_position;	/* notify clients needing position information */
}
SPOOL;

extern SPOOL *spool;
extern int spool_size;
extern int spool_nplaying;
extern int spool_npaused;
extern int spool_prio;
extern int spool_needs_update;

#ifdef __STDC__
extern void spool_init ();
extern SPOOL *spool_create ();
extern void spool_destroy (SPOOL *sp);
extern int spool_match (RPLAY *match, void (*action) (SPOOL *), struct sockaddr_in sin);
extern SPOOL *spool_next (int priority);
extern void spool_ready (SOUND *sound);
extern void spool_remove (SOUND *sound);
extern BUFFER *spool_list_create ();
extern void spool_reset (SPOOL *sp);
extern void spool_setprio (void);
extern int spool_id (void);
extern void spool_stop (SPOOL *sp);
extern void spool_pause (SPOOL *sp);
extern void spool_continue (SPOOL *sp);
extern void spool_done (SPOOL *sp);
extern void spool_flow_pause (SPOOL *sp);
extern void spool_flow_continue (SPOOL *sp);
extern int spool_flow_insert (SPOOL *sp, BUFFER *b);
extern void spool_skip (SPOOL *sp, int count);
extern void spool_play (SPOOL *sp);
extern void spool_update (void);
extern int spool_process (char *buf, int nbytes);
extern SPOOL *spool_find (int id);
extern void spool_cleanup (void);
extern void spool_set_count (SPOOL *sp, int count);
extern void spool_set_list_count (SPOOL *sp, int count);
extern void spool_set_priority (SPOOL *sp, int priority);
extern void spool_set_sample_rate (SPOOL *sp, int sample_rate);
extern void spool_set_volume (SPOOL *sp, int volume);
extern void spool_set_client_data (SPOOL *sp, char *client_data);
extern SPOOL *spool_find_pid (int pid);
#else
extern void spool_init ();
extern SPOOL *spool_create ();
extern void spool_destroy ( /* SPOOL *sp */ );
extern int spool_match ( /* RPLAY *match,  void (*action)(SPOOL *), struct sockaddr_in sin */ );
extern SPOOL *spool_next ( /* int priority */ );
extern void spool_ready ( /* SOUND *sound */ );
extern void spool_remove ( /* SOUND *sound */ );
extern BUFFER *spool_list_create ();
extern void spool_reset ( /* SPOOL *sp */ );
extern void spool_setprio ();
extern int spool_id ();
extern void spool_stop ( /* SPOOL *sp */ );
extern void spool_pause ( /* SPOOL *sp */ );
extern void spool_continue ( /* SPOOL *sp */ );
extern void spool_done ( /* SPOOL *sp */ );
extern void spool_flow_pause ( /* SPOOL *sp */ );
extern void spool_flow_continue ( /* SPOOL *sp */ );
extern int spool_flow_insert ( /* SPOOL *sp, BUFFER *b */ );
extern void spool_skip ( /* SPOOL *sp, int count */ );
extern void spool_play ( /* SPOOL *sp */ );
extern void spool_update ();
extern int spool_process ( /* char *buf, int nbytes */ );
extern SPOOL *spool_find ( /* int id */ );
extern void spool_cleanup ( /* void */ );
extern void spool_set_count ( /* SPOOL *sp, int count */ );
extern void spool_set_list_count ( /* SPOOL *sp, int count */ );
extern void spool_set_priority ( /* SPOOL *sp, int count */ );
extern void spool_set_sample_rate ( /* SPOOL *sp, int sample_rate */ );
extern void spool_set_volume ( /* SPOOL *sp, int volume */ );
extern void spool_set_client_data ( /* SPOOL *sp, char *client_data */ );
extern SPOOL *spool_find_pid ( /* int pid */ );
#endif

#endif /* _spool_h */
