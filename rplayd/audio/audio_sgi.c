/* audio_sgi.c - SGI audio stubs.  */

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



/* SGI audio code was written by Rob Kooper <kooper@dcs.qmw.ac.uk>.  */


#include "rplayd.h"

/*
 * System audio include files:
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <audio.h>
#include <errno.h>

/*
 * External variables:
 */
extern char *rplay_audio_device;
extern int rplay_audio_sample_rate;
extern int rplay_audio_channels;
extern int rplay_audio_precision;
extern int rplay_audio_format;
extern int rplay_audio_port;
extern int optional_sample_rate;
extern int optional_precision;
extern int optional_channels;
extern int optional_format;
extern int optional_port;

/*
 * Internal variables:
 */
static ALport rplay_audio_fd = NULL;

/*
 * Initialize the audio device.
 * This routine must set the following external variables:
 *      rplay_audio_sample_rate
 *      rplay_audio_precision
 *      rplay_audio_channels
 *      rplay_audio_format
 *      rplay_audio_port
 *
 * and may use the following optional parameters:
 *      optional_sample_rate
 *      optional_precision
 *      optional_channels
 *      optional_format
 *      optional_port
 *
 * optional_* variables with values of zero should be ignored.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_init ()
{
    ALconfig SGIconfig;
    long PVbuffer[4];

    /*
     * If the cache directory was not found the errno variable is 1. When
     * calling ALsetwidth errno was not set to 0 when it was OK, and thus
     * returned an error.
     */
    errno = 0;

    /*
     * Create a new config or use the one of the open port.
     */
    if (rplay_audio_fd == NULL)
    {
	SGIconfig = ALnewconfig ();
    }
    else
    {
	SGIconfig = ALgetconfig (rplay_audio_fd);
    }
    if (SGIconfig == NULL)
    {
	return -1;
    }

    /*
     * Set the precision and the format. The sgi can also play 24 bits
     * sample rate. But that is not implemented (yet).
     */
    rplay_audio_precision = optional_precision ? optional_precision : 16;
    switch (rplay_audio_precision)
    {
    case 8:
	ALsetwidth (SGIconfig, AL_SAMPLE_8);
	if (errno != 0)
	{
	    return -1;
	}
	break;
    case 16:
	ALsetwidth (SGIconfig, AL_SAMPLE_16);
	if (errno != 0)
	{
	    return -1;
	}
	break;
    default:
	return -1;
    }

    /*
     * The format is linear (twoscomp) or float/double. Since float/double
     * is not understood by rplay it is left out.
     */
    rplay_audio_format = (rplay_audio_precision == 16) ? RPLAY_FORMAT_LINEAR_16 : RPLAY_FORMAT_LINEAR_8;
#ifdef AL_SAMPFMT_TWOSCOMP
    if (ALsetsampfmt (SGIconfig, AL_SAMPFMT_TWOSCOMP) < 0)
	if (errno != 0)
	{
	    return -1;
	}
#endif

    /*
     * The sgi will work in stereo by default. The 4 channel mode of
     * the indy's is not (yet) supported.
     */
    rplay_audio_channels = optional_channels ? optional_channels : 2;
    switch (rplay_audio_channels)
    {
    case 1:
	ALsetchannels (SGIconfig, AL_MONO);
	if (errno != 0)
	{
	    return -1;
	}
	break;
    case 2:
	ALsetchannels (SGIconfig, AL_STEREO);
	if (errno != 0)
	{
	    return -1;
	}
	break;
    default:
	return -1;
    }

    /*
     * Open the audio port.
     */
    rplay_audio_fd = ALopenport ("rplayd", "w", SGIconfig);
    if (rplay_audio_fd == NULL)
    {
	return -1;
    }
    ALfreeconfig (SGIconfig);

    /*
     * Set the sample rate of the output port. This defaults to 44100Khz
     * which is CD quality!
     */
    rplay_audio_sample_rate = optional_sample_rate ? optional_sample_rate : 44100;
    PVbuffer[0] = AL_OUTPUT_RATE;
    PVbuffer[1] = rplay_audio_sample_rate;
    ALsetparams (AL_DEFAULT_DEVICE, PVbuffer, 2);
    if (errno != 0)
    {
	return -1;
    }

    /*
     * Speaker and headphone(line) is the same. There is also digital but
     * the volume is fixed there. So only speaker is implemented.
     */
    rplay_audio_port = RPLAY_AUDIO_PORT_SPEAKER;

    return 0;
}

/*
 * Open the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_open ()
{
    if (rplay_audio_init () < 0)
    {
	return -1;
    }

    return 0;
}

/*
 * Is the audio device open?
 *
 * Return 1 for true and 0 for false.
 */
int
rplay_audio_isopen ()
{
    return rplay_audio_fd != NULL;
}

/*
 * Flush the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_flush ()
{
    return 0;
}

/*
 * Write nbytes from buf to the audio device.
 *
 * Return the number of bytes written on success and -1 on error.
 */
#ifdef __STDC__
int
rplay_audio_write (char *buf, int nbytes)
#else
int
rplay_audio_write (buf, nbytes)
    char *buf;
    int nbytes;
#endif
{
    int samples;

    if (rplay_audio_fd == NULL)
    {
	if (rplay_audio_open () < 0)
	{
	    return -1;
	}
    }

    /*
     * Calculate the number of samples.
     */
    samples = nbytes / (rplay_audio_precision >> 3);

    /*
     * Write the samples to the audio port.
     */
    ALwritesamps (rplay_audio_fd, buf, samples);
    if (errno != 0)
    {
	return nbytes;
    }
    return -1;
}

/*
 * Close the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_close ()
{
    if (rplay_audio_fd != NULL)
    {
	ALcloseport (rplay_audio_fd);
    }

    rplay_audio_fd = NULL;

    return 0;
}

/*
 * Return the volume of the audio device.
 *
 * Return 0-255 or -1 on error.
 */
int
rplay_audio_get_volume ()
{
    long PVbuffer[4];

#ifdef FAKE_VOLUME
    return rplay_audio_volume;
#else /* not FAKE_VOLUME */
    if (rplay_audio_fd == NULL)
    {
	rplay_audio_open ();
    }
    if (rplay_audio_fd == NULL)
    {
	return -1;
    }
    PVbuffer[0] = AL_LEFT_SPEAKER_GAIN;
    PVbuffer[2] = AL_RIGHT_SPEAKER_GAIN;
    ALgetparams (AL_DEFAULT_DEVICE, PVbuffer, 4);
    if (errno != 0)
    {
	return -1;
    }
    return PVbuffer[1];
#endif /* not FAKE_VOLUME */
}

/*
 * Set the volume of the audio device.
 * Input should be 0-255.
 *
 * Return the volume of the audio device 0-255 or -1.
 */
#ifdef __STDC__
int
rplay_audio_set_volume (int volume)
#else
int
rplay_audio_set_volume (volume)
    int volume;
#endif
{
    long PVbuffer[4];

#ifdef FAKE_VOLUME
    if (volume < RPLAY_MIN_VOLUME)
    {
	volume = RPLAY_MIN_VOLUME;
    }
    else if (volume > RPLAY_MAX_VOLUME)
    {
	volume = RPLAY_MAX_VOLUME;
    }
    rplay_audio_volume = volume;

    return rplay_audio_volume;
#else /* not FAKE_VOLUME */

    rplay_audio_volume = 0;

    if (rplay_audio_fd == NULL)
    {
	rplay_audio_open ();
    }
    if (rplay_audio_fd == NULL)
    {
	return -1;
    }
    PVbuffer[0] = AL_LEFT_SPEAKER_GAIN;
    PVbuffer[1] = volume;
    PVbuffer[2] = AL_RIGHT_SPEAKER_GAIN;
    PVbuffer[3] = volume;
    ALsetparams (AL_DEFAULT_DEVICE, PVbuffer, 4);
    if (errno != 0)
    {
	return -1;
    }

    rplay_audio_volume = rplay_audio_get_volume ();

    return rplay_audio_volume;
#endif /* not FAKE_VOLUME */
}
