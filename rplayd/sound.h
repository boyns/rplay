/* $Id: sound.h,v 1.4 1999/06/09 06:27:44 boyns Exp $ */

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



#ifndef _sound_h
#define _sound_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "buffer.h"
#include "rplay.h"
#include "server.h"
#include <sys/types.h>
#include <sys/uio.h>
#ifdef HAVE_ADPCM
#include "g72x.h"    
#endif /* HAVE_ADPCM */
#ifdef HAVE_GSM
#ifdef HAVE_GSM_GSM_H
#include <gsm/gsm.h>
#else
#ifdef HAVE_GSM_H
#include <gsm.h>
#else
#include "gsm.h"
#endif
#endif
#endif /* HAVE_GSM */

#undef NUMBER_OF_SAMPLES
#define NUMBER_OF_SAMPLES(s) (((s->tail ? (s->tail - s->offset) : (s->size - s->offset)) << 3) / (s->input_precision * s->channels))

#undef HIGH_WATER_MARK
#define HIGH_WATER_MARK(s) (s->sample_rate * s->input_sample_size * 2) /* 2 seconds */

#undef LOW_WATER_MARK
#define LOW_WATER_MARK(s) (s->sample_rate * s->input_sample_size * 1) /* 1 seconds */

#define SOUND_LIST_SIZE	256	/* maximum size of a sound list */

/*
 * Sound types
 */
#define SOUND_FILE	1	/* A sound file. */
#define SOUND_FLOW	2	/* A sound flow. */
#ifdef HAVE_CDROM
#define SOUND_CDROM	3	/* Sound is on a CDROM. */
#endif /* HAVE_CDROM */
#define SOUND_VIRTUAL	4	/* Virtual sound */

/*
 * Sound storage methods
 */
#define SOUND_STORAGE_NULL	0
#define SOUND_STORAGE_NONE	1 /* Input is discarded. */
#define SOUND_STORAGE_DISK	2 /* Input is stored on disk. */
#define SOUND_STORAGE_MEMORY	3 /* Input is stored in memory. */

/*
 * sound status values
 */
#define SOUND_NULL	0
#define SOUND_READY	1	/* sound is ready to be played */
#define SOUND_NOT_READY	2	/* sound is being transferred */
#define SOUND_SEARCH	3	/* sound is being searched for */

/*
 * sound lookup modes
 */
#define SOUND_FIND		1
#define SOUND_DONT_FIND		2
#define SOUND_DONT_COUNT	3
#define SOUND_LOAD		4
#define SOUND_CREATE            5

/*
 * Maximum size of an audio file header.
 */
#define SOUND_MAX_HEADER_SIZE	1024

/*
 * Maximum piece of a sound that can be read by sound_fill ().
 */
#define SOUND_FILL_SIZE	(UIO_MAXIOV * BUFFER_SIZE)

typedef struct _sound
{
    struct _sound *list;	/* next pointer for entire sound list */
    struct _sound *list_prev;	/* prev pointer for entire sound list */
    struct _sound *next;	/* list for sounds with the same hash key */
    struct _sound *prev;	/* prev pointer for the next list */
    int type;			/* type of sound */
    int storage;		/* storage method */
    char *name;			/* name of the sound */
    char *hash_key;		/* sound's hash key */
    char *path;			/* complete path of the sound */
    int count;			/* relative sound count, use for caching */
    int status;			/* status of the sound */
    int format;			/* format of the audio data - RPLAY_FORMAT_* */
    int byte_order;		/* big endian or little endian */
    int sample_rate;		/* audio sample rate */
    float input_precision;	/* bits per sample in the audio file */
    int output_precision;	/* bits per sample that are output */
    int channels;		/* number of channels */
    int samples;		/* number of samples */
    float input_sample_size;	/* size of one input sample = bits/sample * channels */
    int output_sample_size;	/* size of one output sample = bits/sample * channels */
    int offset;			/* audio data offset in the file */
    int size;			/* total size of the sound file */
    int tail;			/* audio offset from the end of the file */
    int chunk_size;             /* size of audio data */
    int mapped;			/* sound info is known */
    char *cache;		/* audio data cache */
    BUFFER *flow;		/* list of flow data */
    BUFFER **flowp;		/* where the next flow data will be put */
#ifdef HAVE_CDROM
    int starting_track;		/* CD start track */
    int ending_track;		/* CD end track */
#endif /* HAVE_CDROM */
#ifdef HAVE_HELPERS
    int needs_helper;
#endif /* HAVE_HELPERS */
}
SOUND;

typedef struct _sindex
{
    SOUND *sound;		/* the sound being referenced */
    int offset;			/* current position */
    int is_cached;		/* is the sound cached? */
    int fd;			/* file descriptor used when sound isn't cached */
    int eof;			/* the end has been reached */
    int skip;
    int buffer_offset;		/* offset in a shared BUFFER */
    int is_flow;		/* is this sound a flow? */
    BUFFER **flowp;		/* pointer to a sound's flow */
    int water_mark;
    int low_water_mark;
    int high_water_mark;
#ifdef HAVE_ADPCM
    struct g72x_state adpcm_state[2]; /* state used in adpcm_decode */
    unsigned int adpcm_in_buffer; /* state variable used adpcm_unpack */
    int adpcm_in_bits;		/* state variable used adpcm_unpack */
#endif /* HAVE_ADPCM */
#ifdef HAVE_GSM
    gsm gsm_object;		/* GSM state object */
    gsm_frame gsm_bit_frame;	/* GSM bit buffer - 33 bytes */
    int gsm_bit_frame_bytes;	/* Number of bytes in gsm_bit_frame */
    int gsm_fixed_buffer_size;
#endif /* HAVE_GSM */
#ifdef HAVE_CDROM
    int pid;
#endif /* HAVE_CDROM */
}
SINDEX;

/* The deadsnd header. */
typedef struct
{
    long id;
    long freq;
    int bps;
    int mode;
    char title[80];
}
DSID;


extern SOUND *sounds;		/* list of all sounds. */
extern int sound_count;		/* sound reference count used in LRU caching */
extern int sound_cache_size;	/* current size of the cache */
extern int sound_cache_max_sound_size;	/* maximum size of a sound that can be cached */
extern int sound_cache_max_size;	/* maximum size of cached sounds */

#ifdef __STDC__
extern void sound_read (char *filename);
extern void sound_reread (char *filename);
extern void sound_stat (char *filename);
extern SOUND *sound_lookup (char *sound_name, int mode, SERVER *server);
extern SOUND *sound_create (void);
extern SOUND *sound_insert (char *, int, int);
extern int sound_map (SOUND *s);
extern int sound_unmap (SOUND *s);
extern void sound_delete (SOUND *s, int remove);
extern BUFFER *sound_list_create ();
extern void sound_cleanup ();
extern SINDEX *sound_open (SOUND *, int);
extern int sound_close (SINDEX *);
extern int sound_fill (SINDEX *, BUFFER *data, int as_is);
extern int sound_seek (SINDEX *, int offset, int whence);
extern void sound_clean (SOUND *s);
extern int number_of_samples (SOUND *s, int bytes);
#else
extern void sound_read ( /* char *filename */ );
extern void sound_reread ( /* char *filename */ );
extern void sound_stat ( /* char *filename */ );
extern SOUND *sound_lookup ( /* char *sound_name, int mode, SERVER *server */ );
extern SOUND *sound_create ();
extern SOUND *sound_insert ( /* char *, int, int */ );
extern int sound_map ( /* SOUND *s */ );
extern int sound_unmap ( /* SOUND *s */ );
extern void sound_delete ( /* SOUND *s, int remove */ );
extern BUFFER *sound_list_create ();
extern void sound_cleanup ();
extern SINDEX *sound_open ( /* SOUND *, int */ );
extern int sound_close ( /* SINDEX * */ );
extern int sound_fill ( /* SINDEX *, BUFFER *data, int as_is */ );
extern int sound_seek ( /* SINDEX *, int offset, int whence */ );
extern void sound_clean ( /* SOUND *s */ );
extern int number_of_samples ( /* SOUND *s, int bytes */ );
#endif

#if defined(HAVE_CDROM) || defined(HAVE_HELPERS)
#ifdef __STDC__
extern BUFFER *sound_pipe_read (SINDEX *si);
#else
extern BUFFER *sound_pipe_read (/* SINDEX *si */);
#endif
#endif

#endif /* _sound_h */
