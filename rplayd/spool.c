/* $Id: spool.c,v 1.2 1998/08/13 06:14:07 boyns Exp $ */

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
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <unistd.h>
#include "rplayd.h"
#include "rplay.h"
#include "spool.h"
#include "sound.h"
#include "buffer.h"
#include "timer.h"
#include "connection.h"
#include "native.h"
#ifdef TEST_FLANGE
#include "flange.h"
#endif

SPOOL *spool = NULL;		/* List of spool entries.  */
int spool_size = 0;		/* Number of entries in the spool.  */
int spool_nplaying = 0;		/* Number of playing sounds.  */
int spool_npaused = 0;		/* Number of paused sounds.  */
int spool_prio = 0;		/* Highest priority spool entry.  */
int spool_needs_update = 0;	/* Need to call spool_update().  */

/*
 * initialize the sound spool
 */
void
spool_init ()
{
    SPOOL *sp, *sp_next;

    /* Destroy any existing spool entries (reset).  */
    for (sp = spool; sp; sp = sp_next)
    {
	sp_next = sp->next;
	spool_destroy (sp);
    }

    spool = NULL;
    spool_nplaying = 0;
    spool_prio = 0;
    spool_size = 0;
}

/* Create a new spool entry.  */
SPOOL *
spool_create ()
{
    SPOOL *sp = (SPOOL *) malloc (sizeof (SPOOL));
    if (sp == NULL)
    {
	report (REPORT_ERROR, "spool_create: out of memory\n");
	done (1);
    }

    /* `sp' must be initialized before being reset.  */
    memset ((char *) sp, 0, sizeof (SPOOL));
    sp->id = -1;		/* XXX */

    spool_reset (sp);

    /* Insert the new entry into the spool list.  */
    sp->next = spool;
    sp->prev = NULL;
    if (spool)
    {
	spool->prev = sp;
    }
    spool = sp;

    spool_size++;

    return sp;
}

/* Destroy the given spool entry.  */
#ifdef __STDC__
void
spool_destroy (SPOOL *sp)
#else
void
spool_destroy (sp)
    SPOOL *sp;
#endif
{
    /* Reset `sp' in case any resources are allocated. */
    spool_reset (sp);

    /* Remove `sp' from the spool list.  */
    if (sp->prev)
    {
	sp->prev->next = sp->next;
    }
    else
    {
	spool = sp->next;
    }
    if (sp->next)
    {
	sp->next->prev = sp->prev;
    }

    spool_size--;

    free ((char *) sp);
}

/*
 * Return the next available spool entry.
 * 
 * `priority' is the priority of a new sound and is used to replace
 * lower priority spool entries when the spool is full.
 */
#ifdef __STDC__
SPOOL *
spool_next (int priority)
#else
SPOOL *
spool_next (priority)
    int priority;
#endif
{
    int i;
    int min_priority = RPLAY_MAX_PRIORITY, index = -1;
    SPOOL *sp, *lowest = NULL;

    /* There's room for a new entry.  */
    if (spool_size < SPOOL_SIZE)
    {
	sp = spool_create ();
	sp->id = spool_id ();
	return sp;
    }

    /* The spool is full, try to replace an entry.  */

    /* Find the lowest priority sound.  */
    for (sp = spool; sp; sp = sp->next)
    {
	if (sp->state == SPOOL_PLAY)
	{
	    if (sp->rp->priority < min_priority)
	    {
		min_priority = sp->rp->priority;
		lowest = sp;
	    }
	}
    }

    /* Replace the lowest priority sound, if possible.  */
    if (priority > min_priority && lowest)
    {
	sp = lowest;
	if (sp->notify_position)
	{
	    connection_notify (0, NOTIFY_POSITION, sp);
	    sp->notify_position = 0;
	}
	connection_notify (0, NOTIFY_DONE, sp);
	spool_reset (sp);
	spool_nplaying--;
	sp->id = spool_id ();
	return sp;
    }
    else
    {
	return NULL;
    }
}

/* Reset a spool entry to an initial state.
   `next' and `prev' are NOT changed.  */
#ifdef __STDC__
void
spool_reset (SPOOL *sp)
#else
void
spool_reset (sp)
    SPOOL *sp;
#endif
{
    sp->state = SPOOL_NULL;
    if (sp->id > 0)
    {
	/* Wakeup any connections that may be waiting for this
	   spool entry. Their flow data will be ignored. */
	connection_flow_continue (sp);
    }
    sp->id = -1;
    sp->time = 0;
    if (sp->rp)
    {
	int i;

	for (i = 0; i < sp->rp->nsounds; i++)
	{
	    sound_clean (sp->sound[i]);
	}
	
	rplay_destroy (sp->rp);
    }
    sp->rp = NULL;
    sp->ptr = NULL;
    sp->ptr_end = NULL;
    sp->sample_rate = 0;
    sp->sample_index = 0;
    sp->sample_factor = 0;
    sp->oversample[0] = 0;
    sp->oversample[1] = 0;
    sp->oversample_inc[0] = 0;
    sp->oversample_inc[1] = 0;
    if (sp->curr_buffer)
    {
	buffer_dealloc (sp->curr_buffer, 1);
    }
    sp->curr_buffer = NULL;
    if (sp->next_buffer)
    {
	buffer_dealloc (sp->next_buffer, 1);
    }
    sp->next_buffer = NULL;
    sp->offset = 0;
    if (sp->si)
    {
	sound_close (sp->si);
    }
    sp->si = NULL;
    sp->to_native = NULL;
    sp->skip_count = 0;
    sp->auto_pause = 0;
    sp->notify_position = 0;

    spool_setprio ();
}

/* Find the maximum spool priority.  */
#ifdef __STDC__
void
spool_setprio (void)
#else
void
spool_setprio ()
#endif
{
    SPOOL *sp;

    spool_prio = 0;
    for (sp = spool; sp; sp = sp->next)
    {
	if (sp->state == SPOOL_PLAY)
	{
	    spool_prio = MAX (spool_prio, sp->rp->priority);
	}
    }
}

/* Find the given RPLAY object in the spool.  */
#ifdef __STDC__
int
spool_match (RPLAY *match, void (*action) (SPOOL *), struct sockaddr_in sin)
#else
int
spool_match (match, action, sin)
    RPLAY *match;
    void (*action) ();
    struct sockaddr_in sin;
#endif
{
    int i, id, nmatch = 0;
    RPLAY *rp;
    RPLAY_ATTRS *a1, *a2;
    SPOOL *sp, *sp_next;

    for (sp = spool; sp; sp = sp_next)
    {
	sp_next = sp->next;	/* sp may be destroyed */

	if (sp->state == SPOOL_NULL)
	{
	    continue;
	}

	a2 = match->attrs;
	if (a2 && a2->sound[0] == '#')
	{
	    for (; a2; a2 = a2->next)
	    {
		id = atoi (a2->sound + 1);
		if (id == 0 || id == sp->id)
		{
		    sp->sin = sin;
		    (*action) (sp);
		    nmatch++;
		    break;
		}
	    }
	    continue;
	}

	rp = sp->rp;
	for (a1 = rp->attrs, a2 = match->attrs; a1 && a2; a1 = a1->next, a2 = a2->next)
	{
	    if (strcmp (a1->sound, a2->sound))
	    {
		break;
	    }
	}
	if (!a1 && !a2)
	{
	    sp->sin = sin;
	    (*action) (sp);
	    nmatch++;
	}
    }

    return nmatch;
}

/* Remove any spool entries with `sound' from the spool. */
#ifdef __STDC__
void
spool_remove (SOUND *sound)
#else
void
spool_remove (sound)
    SOUND *sound;
#endif
{
    int i, j, n;
    SPOOL *sp, *sp_next;

    for (sp = spool; sp; sp = sp_next)
    {
	sp_next = sp->next;	/* sp may be destroyed */

	if (sp->state == SPOOL_NULL)
	{
	    continue;
	}

	n = 0;
	for (j = 0; j < sp->rp->nsounds; j++)
	{
	    if (sp->sound[j] == sound)
	    {
		n++;
	    }
	}
	if (n)
	{
	    spool_done (sp);
	}
    }
}

/*
 * Make all spool entries waiting for `sound' ready to be played.
 */
#ifdef __STDC__
void
spool_ready (SOUND *sound)
#else
void
spool_ready (sound)
    SOUND *sound;
#endif
{
    int j, number_not_ready, has_the_sound;
    SPOOL *sp;

    for (sp = spool; sp; sp = sp->next)
    {
	if (sp->state == SPOOL_WAIT)
	{
	    number_not_ready = 0;
	    has_the_sound = 0;
	    for (j = 0; j < sp->rp->nsounds; j++)
	    {
		if (sp->sound[j]->status != SOUND_READY)
		{
		    number_not_ready++;
		    if (sp->sound[j] == sound)
		    {
			number_not_ready--;
		    }
		}
		if (sp->sound[j] == sound)
		{
		    has_the_sound++;
		}
	    }
	    if (number_not_ready == 0 && has_the_sound)
	    {
		for (j = 0; j < sp->rp->nsounds; j++)
		{
		    sound_map (sp->sound[j]);
		}

		spool_play (sp);
	    }
	}
    }
}

/* Return a buffer list for the `list spool' command.  */
BUFFER *
spool_list_create ()
{
    BUFFER *spool_list, *b;
    int i, n;
    char buf[RPTP_MAX_LINE];
    SOUND *s;
    SPOOL *sp;

    b = buffer_create ();
    spool_list = b;
    SNPRINTF (SIZE(b->buf,BUFFER_SIZE), "+message=\"spool\"\r\n");
    b->nbytes += strlen (b->buf);

    for (sp = spool; sp; sp = sp->next)
    {
	SNPRINTF (SIZE(buf,sizeof(buf)), "id=#%d", sp->id);
	SNPRINTF (SIZE(buf+strlen(buf),sizeof(buf)), " state=");
	switch (sp->state)
	{
	case SPOOL_PLAY:
	    SNPRINTF (SIZE(buf+strlen(buf),sizeof(buf)), "play");
	    break;

	case SPOOL_PAUSE:
	    SNPRINTF (SIZE(buf+strlen(buf),sizeof(buf)), "pause");
	    break;

	case SPOOL_WAIT:
	    SNPRINTF (SIZE(buf+strlen(buf),sizeof(buf)), "wait");
	    break;

	case SPOOL_NEXT:
	    SNPRINTF (SIZE(buf+strlen(buf),sizeof(buf)), "next");
	    break;

	case SPOOL_SKIP:
	    SNPRINTF (SIZE(buf+strlen(buf),sizeof(buf)), "skip");
	    break;

	default:
	    continue;
	}

	s = sp->sound[sp->curr_sound];

	SNPRINTF (SIZE(buf + strlen (buf),sizeof(buf)), "\
 sound=\"%s\" host=%s volume=%d priority=%d count=%d position=%.2f remain=%.2f seconds=%.2f size=%d\
 sample-rate=%d channels=%d bits=%g input=%s client-data=\"%s\" list-name=\"%s\"\r\n",
		 sp->curr_attrs->sound,
		 inet_ntoa (sp->sin.sin_addr),
		 sp->curr_attrs->volume,
		 sp->rp->priority,
		 sp->curr_count,
		 sp->sample_rate && s->samples ? sp->sample_index / sp->sample_rate : 0,
		 sp->sample_rate && s->samples ? ((double) s->samples - sp->sample_index) / sp->sample_rate : 0,
		 sp->sample_rate && s->samples ? (double) s->samples / sp->sample_rate : 0,
		 s->size,
		 sp->sample_rate,
		 s->channels,
		 s->input_precision,
		 input_to_string (s->type),
		 sp->curr_attrs->client_data,
		 sp->rp->list_name);

	n = strlen (buf);
	if (b->nbytes + n > BUFFER_SIZE)
	{
	    b->next = buffer_create ();
	    b = b->next;
	}

	SNPRINTF (SIZE(b->buf+strlen(b->buf), BUFFER_SIZE), buf);
	b->nbytes += n;
    }

    if (b->nbytes + 3 > BUFFER_SIZE)
    {
	b->next = buffer_create ();
	b = b->next;
    }
    SNPRINTF (SIZE(b->buf+strlen(b->buf), BUFFER_SIZE), ".\r\n");
    b->nbytes += 3;

    return spool_list;
}

/*
 * Return a unique spool id.
 */
int
spool_id ()
{
    static int id = 0;
    SPOOL *sp;

    for (;;)
    {
	id++;
	if (id > SPOOL_MAX_ID)
	{
	    id = SPOOL_MIN_ID;
	}
	for (sp = spool; sp; sp = sp->next)	/* see if the id is in use */
	{
	    if (sp->state != SPOOL_NULL && sp->id == id)
	    {
		break;
	    }
	}
	if (!sp)
	{
	    return id;
	}
    }
}

#ifdef __STDC__
void
spool_stop (SPOOL *sp)
#else
void
spool_stop (sp)
    SPOOL *sp;
#endif
{
    timer_block ();
    switch (sp->state)
    {
    case SPOOL_PLAY:
    case SPOOL_PAUSE:
    case SPOOL_WAIT:
	if (sp->state == SPOOL_PLAY)
	{
	    spool_nplaying--;
	}
	else if (sp->state == SPOOL_PAUSE)
	{
	    spool_npaused--;
	}
	if (sp->notify_position)
	{
	    connection_notify (0, NOTIFY_POSITION, sp);
	    sp->notify_position = 0;
	}
	connection_notify (0, NOTIFY_STOP, sp);
	connection_notify (0, NOTIFY_DONE, sp);
	spool_destroy (sp);
	break;
    }
    timer_unblock ();
}

#ifdef __STDC__
void
spool_pause (SPOOL *sp)
#else
void
spool_pause (sp)
    SPOOL *sp;
#endif
{
    timer_block ();
    if (sp->state == SPOOL_PLAY)
    {
	sp->state = SPOOL_PAUSE;
	spool_nplaying--;
	spool_npaused++;
	if (sp->notify_position)
	{
	    connection_notify (0, NOTIFY_POSITION, sp);
	    sp->notify_position = 0;
	}
	connection_notify (0, NOTIFY_PAUSE, sp);
    }
    timer_unblock ();
}

#ifdef __STDC__
void
spool_continue (SPOOL *sp)
#else
void
spool_continue (sp)
    SPOOL *sp;
#endif
{
    timer_block ();
    if (sp->state == SPOOL_PAUSE)
    {
	sp->state = SPOOL_PLAY;
	if (rplay_audio_match)
	{
	    rplayd_audio_match (sp);
	}
	spool_nplaying++;
	spool_npaused--;
	spool_needs_update++;
	connection_notify (0, NOTIFY_CONTINUE, sp);
    }
    timer_unblock ();
}

#ifdef __STDC__
void
spool_done (SPOOL *sp)
#else
void
spool_done (sp)
    SPOOL *sp;
#endif
{
    timer_block ();
    switch (sp->state)
    {
    case SPOOL_PLAY:
    case SPOOL_PAUSE:
    case SPOOL_WAIT:
	if (sp->si && sp->si->is_flow && sp->state == SPOOL_PLAY)
	{
	    sp->si->eof = 1;  /* spool_update will deal with this */
	}
	else
	{
	    if (sp->state == SPOOL_PLAY)
	    {
		spool_nplaying--;
	    }
	    else if (sp->state == SPOOL_PAUSE)
	    {
		spool_npaused--;
	    }
	    if (sp->notify_position)
	    {
		connection_notify (0, NOTIFY_POSITION, sp);
		sp->notify_position = 0;
	    }
	    connection_notify (0, NOTIFY_DONE, sp);
	    spool_destroy (sp);
	}
	break;
    }
    timer_unblock ();
}

#ifdef __STDC__
void
spool_flow_pause (SPOOL *sp)
#else
void
spool_flow_pause (sp)
    SPOOL *sp;
#endif
{
    timer_block ();

    if (sp->state == SPOOL_PLAY)
    {
	sp->state = SPOOL_WAIT;
	spool_nplaying--;
    }

    timer_unblock ();
}

#ifdef __STDC__
void
spool_flow_continue (SPOOL *sp)
#else
void
spool_flow_continue (sp)
    SPOOL *sp;
#endif
{
    timer_block ();

    if (sp->state == SPOOL_WAIT)
    {
	sp->state = SPOOL_PLAY;
	spool_nplaying++;
	spool_needs_update++;
    }

    timer_unblock ();
}

#ifdef __STDC__
int
spool_flow_insert (SPOOL *sp, BUFFER *b)
#else
int
spool_flow_insert (sp, b)
    SPOOL *sp;
    BUFFER *b;
#endif
{
    SOUND *s;
    BUFFER *tmp_buf;

    s = sp->sound[sp->curr_sound];
    *s->flowp = b;
    for (tmp_buf = b; tmp_buf->next; tmp_buf = tmp_buf->next);
    s->flowp = &tmp_buf->next;

    /* Enable the sound. */
    if (s->status == SOUND_NOT_READY)
    {
	s->status = SOUND_READY;
	if (sound_map (s) < 0)
	{
	    /* Punt - can't determine what type of sound it is. */
	    spool_remove (s);
	    /* spool_remove now deletes the sound - sound_delete (s, 0); */
	    return -1;
	}
	spool_ready (s);
    }

    for (tmp_buf = b; tmp_buf; tmp_buf = tmp_buf->next)
    {
	//s->size += tmp_buf->nbytes;
	sp->si->water_mark += tmp_buf->nbytes;
    }

    s->samples = number_of_samples (s, sp->si->water_mark);

    /* Wakeup the spool entry. */
    if (sp->state == SPOOL_WAIT)
    {
	spool_flow_continue (sp);
    }

    return 0;
}

#ifdef __STDC__
void
spool_skip (SPOOL *sp, int count)
#else
void
spool_skip (sp, count)
    SPOOL *sp;
    int count;
#endif
{
    SOUND *s;

    timer_block ();
    s = sp->sound[sp->curr_sound];
    /* Only sounds that are playing or paused can be skipped.
       Flows can't be skipped right now. */
    if ((sp->state == SPOOL_PLAY || sp->state == SPOOL_PAUSE)
	&& s->type == SOUND_FILE)
    {
	if (sp->state == SPOOL_PAUSE)
	{
	    spool_npaused--;
	    sp->auto_pause++;
	}
	sp->state = SPOOL_SKIP;
	sp->skip_count = count;
	spool_needs_update++;
	connection_notify (0, NOTIFY_SKIP, sp);
    }
    timer_unblock ();
}

#ifdef __STDC__
void
spool_play (SPOOL *sp)
#else
void
spool_play (sp)
    SPOOL *sp;
#endif
{
    SOUND *s;

    s = sp->sound[sp->curr_sound];

#ifdef HAVE_CDROM
    /* Parse the name of the cdrom device that was specified. */
    if (s->type == SOUND_CDROM)
    {
	char name_buf[MAXPATHLEN];
	char *p;
	
	strncpy (name_buf, sp->curr_attrs->sound, sizeof(name_buf));
	p = strchr (name_buf, ':');
	if (p == NULL)
	{
	    report (REPORT_DEBUG, "spool_play: can't parse cdrom name `%s'\n", s->name);
	    return;
	}
	*p++ = '\0';

	if (strchr (p, '-'))
	{
	    p = strtok (p, "-");
	    if (p)
	    {
		s->starting_track = atoi (p);
		p = strtok (NULL, "");
		if (p)
		{
		    s->ending_track = atoi (p);
		}
	    }
	}
	else
	{
	    s->starting_track = atoi (p);
	    s->ending_track = atoi (p);
	}
    }
#endif /* HAVE_CDROM */    

    sp->si = sound_open (s, 1);
    if (sp->si == NULL)
    {
	spool_destroy (sp);
	return;
    }

    sound_seek (sp->si, s->offset, SEEK_SET);	/* skip the audio header */

    sp->curr_buffer = NULL;
    sp->next_buffer = NULL;
    sp->sample_index = 0;
    sp->to_native = native_table[s->format].to_native[s->byte_order - 1];


    spool_set_sample_rate (sp, 
			   (sp->curr_attrs->sample_rate == RPLAY_DEFAULT_SAMPLE_RATE)
			   ? s->sample_rate : sp->curr_attrs->sample_rate);

    /* Change the state to SPOOL_PLAY when ALL fields have been assigned. */
    sp->state = SPOOL_PLAY;
    
    spool_nplaying++;
    spool_setprio ();
    spool_needs_update++;
    connection_notify (0, NOTIFY_PLAY, sp);
    connection_notify (0, NOTIFY_POSITION, sp);

    if (sp->auto_pause)
    {
	spool_pause (sp);
	sp->auto_pause = 0;
    }
}

#ifdef __STDC__
void
spool_update (void)
#else
void
spool_update ()
#endif
{
    int i, n;
    SPOOL *sp, *sp_next;
    SOUND *s;
    BUFFER *b;
    time_t now;
    
    spool_needs_update = 0;
    now = time (0);
    
    for (sp = spool; sp; sp = sp_next)
    {
	sp_next = sp->next;	/* sp may be destroyed */

	/* Some sounds must end here.  */
	if (sp->state == SPOOL_PLAY && !sp->curr_buffer && !sp->si)
	{
	    sp->state = SPOOL_NEXT;
	}

	switch (sp->state)
	{
	case SPOOL_SKIP:
	case SPOOL_NEXT:
	    if (sp->notify_position)
	    {
		connection_notify (0, NOTIFY_POSITION, sp);
		sp->notify_position = 0;
	    }
	    
	    if (!sp->auto_pause)	/* something was playing */
	    {
		spool_nplaying--;
	    }
	    
	    if (sp->si)
	    {
		sound_close (sp->si);
		sp->si = NULL;
	    }
	    buffer_dealloc (sp->curr_buffer, 0);
	    buffer_dealloc (sp->next_buffer, 0);
	    sp->curr_buffer = NULL;
	    sp->next_buffer = NULL;
	    sp->offset = 0;

	    if (sp->state == SPOOL_SKIP)
	    {
		/* The current sound is now `done'. */
		connection_notify (0, NOTIFY_DONE, sp);
		
		if (sp->skip_count == 0)
		{
		    sp->curr_count++;	/* "play it again Sam" */
		}
		else
		{
		    sp->curr_count = 1;		/* end the current sound */

		    /*
		     * Skip to the next sound.
		     */

		    /* Repeat the current sound if more than 1 second
		       has been played.  Otherwise, move to the
		       previous sound. */
		    if (sp->skip_count < 0
			&& sp->sample_index > rplay_audio_sample_rate)
		    {
			sp->skip_count++;
		    }
		    sp->curr_sound += sp->skip_count - 1;

		    /* Skipping before the first sound will always
		       skip to the first sound.  */
		    if (sp->curr_sound < 0)
		    {
			sp->curr_sound = -1;
		    }
		    /* Skipping after the last sound will always
		       skip to the last sound.  */
		    else if (sp->curr_sound == sp->rp->nsounds)
		    {
			sp->curr_sound = sp->rp->nsounds - 1;
		    }
		}
	    }

	    if (sp->curr_count > 1)
	    {
		sp->curr_count--;
	    }
	    else if (sp->curr_count)
	    {
		/* Save a pointer to the sound that was playing. */
		s = sp->sound[sp->curr_sound];

		sp->curr_sound++;
		if (sp->curr_sound == sp->rp->nsounds)
		{
		    if (sp->list_count > 1)
		    {
			sp->list_count--;
			sp->curr_attrs = sp->rp->attrs;
			sp->curr_sound = 0;
		    }
		    else if (sp->list_count == 0)
		    {
			sp->curr_attrs = sp->rp->attrs;
			sp->curr_sound = 0;
		    }
		    else /* The spool entry is finished. */
		    {			
			if (sp->state != SPOOL_SKIP) /* already sent `done' */
			{
			    connection_notify (0, NOTIFY_DONE, sp);
			}

			spool_destroy (sp);
			continue;
		    }
		}
		else
		{
		    /* Update `curr_attrs' to point to the next
		       set of attributes. */
		    if (sp->state == SPOOL_SKIP)
		    {
			/* This does a linear search.  Bad, bad, bad */
			int i;
			sp->curr_attrs = sp->rp->attrs;
			for (i = 0; i < sp->curr_sound; i++)
			{
			    sp->curr_attrs = sp->curr_attrs->next;
			}
			sp->skip_count = 0;
		    }
		    else
		    {
			sp->curr_attrs = sp->curr_attrs->next;
		    }
		}

		sp->curr_count = sp->curr_attrs->count;
	    }
	    spool_play (sp);
	    /* break; */

	case SPOOL_PLAY:
	    if (!sp->curr_buffer ||
		(!sp->next_buffer && sp->si && !sp->si->eof))
	    {
		s = sp->sound[sp->curr_sound];

#ifdef HAVE_OSS		
		b = buffer_alloc (MIN (s->sample_rate * s->output_sample_size, SOUND_FILL_SIZE),
				  BUFFER_REUSE);
#else
		b = buffer_alloc (MIN (MAX (curr_bufsize * curr_rate,
					    s->sample_rate * s->output_sample_size),
				       SOUND_FILL_SIZE),
				  BUFFER_REUSE);
#endif		

		n = sound_fill (sp->si, b, 0);
		
		if (n <= 0)
		{
		    buffer_dealloc (b, 1);

		    if (s->needs_helper)
		    {
			if (!sp->curr_buffer && !sp->next_buffer)
			{
			    if (sp->si->eof)
			    {
				sound_close (sp->si);
				sp->si = NULL; /* rplayd_write will see this as a dead sound */
				spool_needs_update++;
			    }
			    else
			    {
				spool_flow_pause (sp);
			    }
			}
		    }
		    else if (s->type == SOUND_FILE)
		    {
			sound_close (sp->si);
			sp->si = NULL; /* rplayd_write will see this as a dead sound */
		    }
		    else if (s->type == SOUND_FLOW)
		    {
			if (!sp->curr_buffer && !sp->next_buffer)
			{
			    if (sp->si->eof)
			    {
				sound_close (sp->si);
				sp->si = NULL; /* rplayd_write will see this as a dead sound */
				spool_needs_update++;
			    }
			    else
			    {
				spool_flow_pause (sp);
			    }
			}
		    }
#ifdef HAVE_CDROM
		    else if (s->type == SOUND_CDROM)
		    {
			if (!sp->curr_buffer && !sp->next_buffer)
			{
			    if (sp->si->eof)
			    {
				sound_close (sp->si);
				sp->si = NULL; /* rplayd_write will see this as a dead sound */
				spool_needs_update++;
			    }
			    else
			    {
				spool_flow_pause (sp);
			    }
			}
#endif /* HAVE_CDROM */
		    }
		}
		else
		{
		    sp->time = now;
		    
		    if (!sp->curr_buffer)
		    {
			timer_block ();
			sp->ptr = (unsigned char *) b->buf + sp->offset;
			sp->ptr_end = (unsigned char *) b->buf + b->nbytes;
			sp->curr_buffer = b;
			timer_unblock ();
		    }
		    else
		    {
			sp->next_buffer = b;
		    }
	    
#ifdef HAVE_HELPERS
		    if (s->needs_helper
			&& ((sp->si->water_mark - sp->si->offset) < sp->si->low_water_mark))
		    {
			FD_SET (sp->si->fd, &read_mask);
		    }
#else
		    if (0)
		    {
			/* hack */
		    }
#endif /* HAVE_HELPERS */
		    else if (s->type == SOUND_FLOW
			&& ((sp->si->water_mark - sp->si->offset) < sp->si->low_water_mark))
		    {
			connection_notify (0, NOTIFY_FLOW, sp);
			connection_flow_continue (sp);
		    }
#ifdef HAVE_CDROM
		    else if (s->type == SOUND_CDROM
			     && ((sp->si->water_mark - sp->si->offset) < sp->si->low_water_mark))
		    {
			FD_SET (sp->si->fd, &read_mask);
		    }
#endif /* HAVE_CDROM */
		}
	    }
	    break;
	}
    }
}

#ifdef __STDC__
int
spool_process (char *buf, int nbytes)
#else
int
spool_process (buf, nbytes)
    char *buf;
    int nbytes;
#endif
{
    SPOOL *sp, *sp_next;
    int number_playing = 0;
    int nsamples;

    /* Determine the number of samples that need to be processed.  */
    nsamples = nbytes / ((rplay_audio_precision >> 3) * rplay_audio_channels);

    zero_native (rplay_audio_buf, nsamples, rplay_audio_channels);
	
    for (sp = spool; sp; sp = sp_next)
    {
	sp_next = sp->next;

	if (sp->state == SPOOL_PLAY)
	{
	    number_playing++;

	    /* See if the end of sound has been reached.  */
	    if (!(*sp->to_native) (sp, rplay_audio_buf, nsamples, rplay_audio_channels))
	    {
		sp->state = SPOOL_NEXT;
		spool_needs_update++;
	    }
	    /* See if more buffers need to be filled by `spool_update'.  */
	    else if (sp->curr_buffer == NULL ||
		(sp->si && !sp->si->eof && !sp->next_buffer))
	    {
		spool_needs_update++;
	    }

	    sp->notify_position++;
	}
    }

    if (number_playing)
    {
#ifdef TEST_FLANGE
	flange (rplay_audio_buf, nsamples, rplay_audio_channels);
#endif

#ifdef FAKE_VOLUME
	fake_volume (rplay_audio_buf, nsamples, rplay_audio_channels);
#endif
			
	/* Only calculate left and right levels if there's a connection
	   that needs the information. */
	if (connection_want_level_notify)
	{
	    level (rplay_audio_buf, nsamples, rplay_audio_channels);
	    connection_level_notify++;	    
	}

	/* Convert rplay_audio_buf to the audio device format.  */
	(*native_table[rplay_audio_format].from_native[RPLAY_AUDIO_BYTE_ORDER - 1])
	    (rplay_audio_buf, nsamples, rplay_audio_channels);

	rplay_audio_size = nbytes;
    }
}

#ifdef __STDC__
SPOOL *
spool_find (int id)
#else
SPOOL *
spool_find (id)
    int id;
#endif
{
    SPOOL *sp;
    
    for (sp = spool; sp; sp = sp->next)
    {
	if (sp->id == id)
	{
	    break;
	}
    }

    return sp;
}

/* This routine can be used to remove idle flows from the spool. */
#ifdef __STDC__
void
spool_cleanup (void)
#else
void
spool_cleanup ()
#endif
{
    SPOOL *sp;
    int flows_cleaned = 0;
    
    for (sp = spool; sp; sp = sp->next)
    {
	if (sp->state == SPOOL_WAIT
	    && sp->sound[sp->curr_sound]->type == SOUND_FLOW)
	{
	    spool_done (sp);
	    flows_cleaned++;
	}
    }

    report (REPORT_DEBUG, "cleaning up the spool - %d flows\n",
	    flows_cleaned);
}

#ifdef __STDC__
void
spool_set_count (SPOOL *sp, int count)
#else
void
spool_set_count (sp, count)
    SPOOL *sp;
    int count;
#endif
{
    if (sp)
    {
	sp->curr_count = count;
	sp->curr_attrs->count = count;
    }
}

#ifdef __STDC__
void
spool_set_list_count (SPOOL *sp, int count)
#else
void
spool_set_list_count (sp, count)
    SPOOL *sp;
    int count;
#endif
{
    if (sp)
    {
	sp->list_count = count;
	sp->rp->count = count;
    }
}

#ifdef __STDC__
void
spool_set_priority (SPOOL *sp, int priority)
#else
void
spool_set_priority (sp, priority)
    SPOOL *sp;
    int priority;
#endif
{
    if (sp)
    {
	sp->rp->priority = priority;
	spool_setprio ();
    }
}

#ifdef __STDC__
void
spool_set_sample_rate (SPOOL *sp, int sample_rate)
#else
void
spool_set_sample_rate (sp, sample_rate)
    SPOOL *sp;
    int sample_rate;
#endif
{
    if (sp)
    {
	sp->sample_rate = sample_rate > 0 ? sample_rate : sp->sound[sp->curr_sound]->sample_rate;
	sp->curr_attrs->sample_rate = sample_rate;
	sp->sample_factor = (double) sp->sample_rate / (double) rplay_audio_sample_rate;

	if (rplay_audio_match)
	{
	    rplayd_audio_match (sp);
	}
    }
}

#ifdef __STDC__
void
spool_set_volume (SPOOL *sp, int volume)
#else
void
spool_set_volume (sp, volume)
    SPOOL *sp;
    int volume;
#endif
{
    if (sp)
    {
	sp->curr_attrs->volume = volume;
    }
}

#ifdef __STDC__
void
spool_set_client_data (SPOOL *sp, char *client_data)
#else
void
spool_set_client_data (sp, client_data)
    SPOOL *sp;
    char *client_data;
#endif
{
    if (sp)
    {
	if (*sp->curr_attrs->client_data)
	{
	    free ((char *) sp->curr_attrs->client_data);
	}
	
	sp->curr_attrs->client_data = strdup (client_data);
    }
}

#ifdef __STDC__
SPOOL *
spool_find_pid (int pid)
#else
SPOOL *
spool_find_pid (pid)
    int pid;
#endif
{
    SPOOL *sp;
    
#if defined(HAVE_CDROM) || defined(HAVE_HELPERS)
    for (sp = spool; sp; sp = sp->next)
    {
	if (sp->si && sp->si->pid == pid)
	{
	    return sp;
	}
    }
#endif

    return NULL;
}
