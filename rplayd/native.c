/* native.c - Native audio conversion.  */

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
#include "spool.h"
#include "sound.h"
#include "buffer.h"
#include "rplayd.h"
#include "native.h"
#include "ulaw.h"

/* Native means 16-bit signed. */

static int ulaw_to_native ( /* SPOOL *sp, char *native_buf, int nsamples, int nchannels */ );
static int s8_to_native ( /* SPOOL *sp, char *native_buf, int nsamples, int nchannels */ );
static int u8_to_native ( /* SPOOL *sp, char *native_buf, int nsamples, int nchannels */ );
static int big_s16_to_native ( /* SPOOL *sp, char *native_buf, int nsamples, int nchannels */ );
static int big_u16_to_native ( /* SPOOL *sp, char *native_buf, int nsamples, int nchannels */ );
static int little_s16_to_native ( /* SPOOL *sp, char *native_buf, int nsamples, int nchannels */ );
static int little_u16_to_native ( /* SPOOL *sp, char *native_buf, int nsamples, int nchannels */ );
static int native_to_native ( /* SPOOL *sp, char *native_buf, int nsamples, int nchannels */ );
static int native_to_ulaw ( /* char *native_buf, int nsamples, int nchannels */ );
static int native_to_s8 ( /* char *native_buf, int nsamples, int nchannels */ );
static int native_to_u8 ( /* char *native_buf, int nsamples, int nchannels */ );
static int native_to_s16 ( /* char *native_buf, int nsamples, int nchannels */ );
static int native_to_u16 ( /* char *native_buf, int nsamples, int nchannels */ );
static int native_to_g721 ( /* char *native_buf, int nsamples, int nchannels */ );
static int native_to_g723_3 ( /* char *native_buf, int nsamples, int nchannels */ );
static int native_to_g723_5 ( /* char *native_buf, int nsamples, int nchannels */ );
static int native_to_gsm ( /* char *native_buf, int nsamples, int nchannels */ );

/* Setup the to and from native table. */
NATIVE_TABLE native_table[] =
{
    {{0, 0}, {0, 0}},
    {{s8_to_native, s8_to_native}, {native_to_s8, native_to_s8}},
    {{u8_to_native, u8_to_native}, {native_to_u8, native_to_u8}},
    {{big_s16_to_native, little_s16_to_native}, {native_to_s16, native_to_s16}},
    {{big_u16_to_native, little_u16_to_native}, {native_to_u16, native_to_u16}},
    {{ulaw_to_native, ulaw_to_native}, {native_to_ulaw, native_to_ulaw}},
#ifdef HAVE_ADPCM    
    {{native_to_native, native_to_native}, {native_to_g721, native_to_g721}},
    {{native_to_native, native_to_native}, {native_to_g723_3, native_to_g723_5}},
    {{native_to_native, native_to_native}, {native_to_g723_3, native_to_g723_5}},
#endif /* HAVE_ADPCM */    
#ifdef HAVE_GSM
    {{native_to_native, native_to_native}, {native_to_gsm, native_to_gsm}},
#endif /* HAVE_GSM */    
};

/* Initialize the native audio buffer.  */
void
zero_native (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    memset (native_buf, 0, nsamples * nchannels * 2); /* 2 == 16-bit */
}


void
level (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    short *p = (short *) native_buf, sample;
    int n = nsamples * nchannels;
    long left_max = 0, right_max = 0;

    /* Optimize the cases for stereo and mono output. */
    
    if (nchannels == 2)		/* stereo */
    {
	while (n--)
	{
#ifdef FAKE_VOLUME
	    sample = (*p++ >> 7);
#else
	    sample = (*p++ * rplay_audio_volume) >> 14;
#endif	    
	    if (n & 1)
	    {
		if (sample > right_max)
		{
		    right_max = sample;
		}
	    }
	    else
	    {
		if (sample > left_max)
		{
		    left_max = sample;
		}
	    }
	}
    }
    else			/* mono */
    {
	while (n--)
	{
#ifdef FAKE_VOLUME
	    sample = (*p++ >> 7);
#else
	    sample = (*p++ * rplay_audio_volume) >> 14;
#endif	    
	    if (sample > left_max)
	    {
		left_max = sample;
	    }
	}
	right_max = left_max;
    }

    rplay_audio_left_level = left_max;
    rplay_audio_right_level = right_max;
}


#ifdef FAKE_VOLUME
/* Simulate hardware volume control.  */
void
fake_volume (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    short *p = (short *) native_buf;
    int n = nsamples * nchannels;

    while (n--)
    {
	*p++ = (*p * rplay_audio_volume) >> 7;
    }
}
#endif /* FAKE_VOLUME */


/* Move `sp' to the next sample.  */
#define next_sample(sp, s)				\
{							\
    int sample_delta;					\
							\
    sample_delta = sp->sample_index;			\
    sp->sample_index += sp->sample_factor;		\
    sample_delta = sp->sample_index - sample_delta;	\
    sp->offset = sample_delta * s->output_sample_size;	\
    if (s->samples > 0                                  \
	&& sp->sample_index >= s->samples		\
	&& (s->type == SOUND_FILE || sp->si->eof))	\
    {							\
	return 0;					\
    }							\
}

#define check_buffers(sp)						\
{									\
    /* Compute the next offset.  */					\
    sp->offset = sp->ptr - sp->ptr_end;					\
									\
	/* Try to use curr_buffer's next buffer.  */			\
    if (sp->curr_buffer->next && sp->curr_buffer->next->nbytes > 0)	\
    {									\
	BUFFER *b = sp->curr_buffer;					\
									\
	/* report (REPORT_DEBUG, "* using curr_buffer->next\n"); */	\
									\
	sp->curr_buffer = sp->curr_buffer->next;			\
	sp->ptr = (unsigned char *)sp->curr_buffer->buf;		\
	sp->ptr_end = (unsigned char *)sp->curr_buffer->buf		\
	    + sp->curr_buffer->nbytes;					\
									\
	buffer_destroy (b);						\
    }									\
    /* Try to use next_buffer.  */					\
    else if (sp->next_buffer)						\
    {									\
	BUFFER *b = sp->curr_buffer;					\
									\
	/* report (REPORT_DEBUG, "* using next_buffer\n"); */		\
									\
	sp->curr_buffer = sp->next_buffer;				\
	sp->ptr = (unsigned char *)sp->curr_buffer->buf;		\
	sp->ptr_end = (unsigned char *)sp->curr_buffer->buf		\
	    + sp->curr_buffer->nbytes;					\
									\
	sp->next_buffer = NULL;						\
									\
	buffer_dealloc (b, 0);						\
    }									\
    /* No more buffers are available.  */				\
    else								\
    {									\
	/* report (REPORT_DEBUG, "* no buffers!\n"); */			\
									\
	buffer_dealloc (sp->curr_buffer, 0);				\
	sp->curr_buffer = NULL;						\
									\
	break;								\
    }									\
}

/* Return 0 when the sound is over, otherwise return 1.  */
#define x_to_native(FUNC, SAMPLE_TO_NATIVE)											\
static int															\
FUNC (sp, native_buf, nsamples, nchannels)											\
    SPOOL	*sp;														\
    short	*native_buf;													\
    int	nsamples;														\
    int	nchannels;														\
{																\
    SOUND *curr_sound;														\
    short new_volume;														\
    int	sample_count = 0;													\
    int	curr_channel;														\
    short linear;														\
    short orig_linear;														\
    short *buf = (short *)native_buf;												\
																\
    /* Find the sound that being played.  */											\
    curr_sound = sp->sound[sp->curr_sound];											\
																\
    /* Compute the new volume for the sound using priorities.  */								\
    new_volume = sp->curr_attrs->volume;											\
    if (spool_prio && spool_nplaying > 1)											\
    {																\
	new_volume -= (spool_prio - sp->rp->priority) >> 1;									\
	new_volume = MAX(new_volume, RPLAY_MIN_VOLUME);										\
    }																\
																\
    /* do oversampling */													\
    if (sp->sample_factor < 1)													\
    {																\
	while (sp->curr_buffer && sample_count < nsamples)									\
	{															\
	    sp->ptr += sp->offset;												\
	    if (sp->ptr < sp->ptr_end)												\
	    {															\
		for (curr_channel = 0; curr_channel < nchannels; curr_channel++)						\
		{														\
		    if (curr_channel < curr_sound->channels)									\
		    {														\
			SAMPLE_TO_NATIVE;											\
			orig_linear = linear;											\
			linear = sp->oversample[curr_channel];									\
			sp->oversample[curr_channel] += sp->oversample_inc[curr_channel];					\
			if (sp->sample_index + sp->sample_factor > sp->offset + 1)						\
			{													\
			    sp->oversample_inc[curr_channel] = (orig_linear -							\
								sp->oversample[curr_channel]) * sp->sample_factor;		\
			}													\
		    }														\
		    *buf++ += ((linear * new_volume) >> 7);									\
		}														\
		sample_count++;													\
		next_sample (sp, curr_sound);											\
	    }															\
	    else														\
	    {															\
		check_buffers (sp);												\
	    }															\
	}															\
    }																\
    /* read the sound as-is */													\
    else															\
    {																\
	while (sp->curr_buffer && sample_count < nsamples)									\
	{															\
	    sp->ptr += sp->offset;												\
	    if (sp->ptr < sp->ptr_end)												\
	    {															\
		for (curr_channel = 0; curr_channel < nchannels; curr_channel++)						\
		{														\
		    if (curr_channel < curr_sound->channels)									\
		    {														\
			SAMPLE_TO_NATIVE;											\
		    }														\
		    *buf++ += ((linear * new_volume) >> 7);									\
		}														\
		sample_count++;													\
		next_sample (sp, curr_sound);											\
	    }															\
	    else														\
	    {															\
		check_buffers (sp);												\
	    }															\
	}															\
    }																\
    																\
    return 1;															\
}

/* ulaw */
x_to_native
(
 ulaw_to_native,
 linear = ulaw_to_linear (*(sp->ptr + curr_channel))
)

/* signed 8-bit */
x_to_native
(
 s8_to_native,
 linear = (char) *(sp->ptr + curr_channel);
 linear <<= 8
)

/* unsigned 8-bit */
x_to_native
(
 u8_to_native,
 linear = (unsigned char) *(sp->ptr + curr_channel) ^ 0x80;
 linear <<= 8
)

/* signed 16-bit big-endian */
x_to_native
(
 big_s16_to_native,
 linear = (short) (sp->ptr[curr_channel << 1] << 8 | sp->ptr[(curr_channel << 1) + 1])
)

/* signed 16-bit little-endian */
x_to_native
(
 little_s16_to_native,
 linear = (short) (sp->ptr[(curr_channel << 1) + 1] << 8 | sp->ptr[curr_channel << 1])
)

/* unsigned 16-bit big-endian */
x_to_native
(
 big_u16_to_native,
 linear = (short) (sp->ptr[curr_channel << 1] << 8 | sp->ptr[(curr_channel << 1) + 1]) ^ 0x8000;
)

/* unsigned 16-bit little-endian */
x_to_native
(
 little_u16_to_native,
 linear = (short) (sp->ptr[(curr_channel << 1) + 1] << 8 | sp->ptr[curr_channel << 1]) ^ 0x8000;
)

/* native to native */
x_to_native
(
 native_to_native,
 linear = ( *((short *) sp->ptr + curr_channel) )
)
    
static int
native_to_ulaw (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    short *from = (short *) native_buf;
    char *to = (char *) native_buf;
    int n = nsamples * nchannels;

    while (n--)
    {
	*to++ = linear_to_ulaw (*from++);
    }

    return 0;
}

static int
native_to_s8 (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    short *from = (short *) native_buf;
    char *to = (char *) native_buf;
    int n = nsamples * nchannels;

    while (n--)
    {
	*to++ = (*from++ >> 8);
    }

    return 0;
}

static int
native_to_u8 (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    short *from = (short *) native_buf;
    char *to = (char *) native_buf;
    int n = nsamples * nchannels;

    while (n--)
    {
	*to++ = (*from++ >> 8) ^ 0x80;
    }

    return 0;
}

static int
native_to_s16 (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
/* The native audio buffer is already signed 16-bit.  */
    return 0;
}

static int
native_to_u16 (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    short *from = (short *) native_buf;
    short *to = (short *) native_buf;
    int n = nsamples * nchannels;

    while (n--)
    {
	*to++ = *from++ ^ 0x8000;
    }

    return 0;
}

#ifdef HAVE_ADPCM
static int
native_to_g721 (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    /* NOT SUPPORTED */
    return 0;
}

static int
native_to_g723_3 (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    /* NOT SUPPORTED */
    return 0;
}

static int
native_to_g723_5 (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    /* NOT SUPPORTED */
    return 0;
}
#endif /* HAVE_ADPCM */

#ifdef HAVE_GSM
static int
native_to_gsm (native_buf, nsamples, nchannels)
    char *native_buf;
    int nsamples;
    int nchannels;
{
    /* NOT SUPPORTED */
    return 0;
}
#endif /* HAVE_GSM */
