/* $Id: audio_FreeBSD.c,v 1.2 1998/08/13 06:14:15 boyns Exp $ */

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



/* FreeBSD audio code was written by Andreas S. Wetzel <mickey@deadline.snafu.de>. */

#include "rplayd.h"

/*
 * System audio include files:
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <errno.h>
#include <machine/soundcard.h>

/*
 * for the poor folks who don't have a sound card, but use the pc speaker
 * driver pcsndrv version 0.6 (no SNDCTL_DSP_POST ....yet)
 */
#ifndef SNDCTL_DSP_POST
#define SNDCTL_DSP_POST SNDCTL_DSP_SYNC
#endif

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

extern int max_rplay_audio_bufsize;

/*
 * Internal variables:
 */
int rplay_audio_fd = -1;

/*
 * Initialize the audio device.
 * This routine must set the following external variables:
 *      rplay_audio_sample_rate
 *      rplay_audio_precision
 *      rplay_audio_channels
 *      rplay_audio_format
 *
 * and may use the following optional parameters:
 *      optional_sample_rate
 *      optional_precision
 *      optional_channels
 *      optional_format
 *
 * optional_* variables with values of zero should be ignored.
 *
 * Return 0 on success and -1 on error.
 */

int 
rplay_audio_init (void)
{
    int n;

    if (rplay_audio_fd == -1)
    {
	if (rplay_audio_open () == -1)
	{
	    report (REPORT_ERROR, "rplay_audio_init: cannot open %s\n", rplay_audio_device);
	    return -1;
	}
    }

    rplay_audio_sample_rate = optional_sample_rate ? optional_sample_rate : 44100;
    rplay_audio_precision = optional_precision ? optional_precision : 16;
    rplay_audio_channels = optional_channels ? optional_channels : 2;
    rplay_audio_format = optional_format ? optional_format :
	rplay_audio_precision == 16 ? RPLAY_FORMAT_LINEAR_16 : RPLAY_FORMAT_ULINEAR_8;

    /*
     * Set dsp sample size
     */
    n = rplay_audio_precision;

    if (ioctl (rplay_audio_fd, SNDCTL_DSP_SAMPLESIZE, &n) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_init: can't set audio precision to %d (%d)\n",
		rplay_audio_precision, n);
	return -1;
    }

    if (n != rplay_audio_precision)
    {
	report (REPORT_NOTICE, "rplay_audio_init: audio precision changed from %d to %d\n",
		rplay_audio_precision, n);
	rplay_audio_precision = n;
    }

    /*
     * Set # of channels
     */
    n = rplay_audio_channels;

    if (ioctl (rplay_audio_fd, SOUND_PCM_WRITE_CHANNELS, &n) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_init: can't set audio channels to %d (%d)\n",
		rplay_audio_channels, n);
	return -1;
    }

    if (n != rplay_audio_channels)
    {
	report (REPORT_NOTICE, "rplay_audio_init: audio channels changed from %d to %d\n",
		rplay_audio_channels, n);
	rplay_audio_channels = n;
    }

    /*
     * Set the sampling rate
     */

    n = rplay_audio_sample_rate;

    if (ioctl (rplay_audio_fd, SNDCTL_DSP_SPEED, &n) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_init: can't set audio sample rate to %d (%d)\n",
		rplay_audio_sample_rate, n);
	return (-1);
    }

    if (n != rplay_audio_sample_rate)
    {
	report (REPORT_NOTICE, "rplay_audio_init: audio sample rate changed from %d to %d\n",
		rplay_audio_sample_rate, n);
	rplay_audio_sample_rate = n;
    }

    /*
     * Set the data format
     */
    switch (rplay_audio_format)
    {
    case RPLAY_FORMAT_NONE:
	break;			/* ??? */
    case RPLAY_FORMAT_LINEAR_8:
	n = AFMT_S8;
	break;
    case RPLAY_FORMAT_ULINEAR_8:
	n = AFMT_U8;
	break;
    case RPLAY_FORMAT_LINEAR_16:
	n = AFMT_S16_LE;
	break;
    case RPLAY_FORMAT_ULINEAR_16:
	n = AFMT_U16_LE;
	break;
    case RPLAY_FORMAT_ULAW:
	n = AFMT_MU_LAW;
	break;
    case RPLAY_FORMAT_G721:
    case RPLAY_FORMAT_G723_3:
    case RPLAY_FORMAT_G723_5:
	n = AFMT_IMA_ADPCM;
	break;
    default:
	report (REPORT_ERROR, "rplay_audio_init: unknown audio format (%d)\n", rplay_audio_format);
	return (-1);
    }

    if (ioctl (rplay_audio_fd, SNDCTL_DSP_SETFMT, &n) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_init: can't set audio format\n");
	return (-1);
    }

    /*
     * Set the audio blocksize
     */
    if (n = curr_bufsize)
    {
	if (ioctl (rplay_audio_fd, SNDCTL_DSP_SETBLKSIZE, &n) == -1)
	{
	    report (REPORT_ERROR, "rplay_audio_init: can't set audio blocksize to %d\n", curr_bufsize);
	    return (-1);
	}
	report (REPORT_DEBUG, "rplay_audio_init: device blksize set to %d\n", n);
    }

    rplay_audio_port = RPLAY_AUDIO_PORT_SPEAKER;

    return 0;
}

/*
 * Open the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int 
rplay_audio_open (void)
{
    if ((rplay_audio_fd = open (rplay_audio_device, (O_WRONLY | O_NONBLOCK), 0)) < 0)
	return -1;

    if (rplay_audio_init () < 0)
	return -1;

    return 0;
}

/*
 * Is the audio device open?
 *
 * Return 1 for true and 0 for false.
 */
int 
rplay_audio_isopen (void)
{
    return (rplay_audio_fd != -1);
}

/*
 * Flush the audio device.
 *
 * Return 0 on success and -1 on error.
 */

int 
rplay_audio_flush (void)
{
    if (rplay_audio_fd != -1)
	return (ioctl (rplay_audio_fd, SNDCTL_DSP_POST, 0));

    return 0;
}

/*
 * Write nbytes from buf to the audio device.
 *
 * Return bytes written on success and -1 on error.
 */
int 
rplay_audio_write (char *buf, int nbytes)
{
    int remain = nbytes;
    int xr;

    while (remain > 0)
    {
	if ((xr = write (rplay_audio_fd, buf, remain)) == -1)
	{
	    switch (errno)
	    {
	    case EWOULDBLOCK:
		continue;
	    default:
		report (REPORT_ERROR, "Error while writing to audio device (%s)\n", strerror (errno));
		return (-1);
	    }
	}
	else
	{
	    remain -= xr;
	}
    }

    return (nbytes);
}

/*
 * Close the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int 
rplay_audio_close (void)
{
    if (rplay_audio_fd != -1)
    {
	close (rplay_audio_fd);
    }

    rplay_audio_fd = -1;

    return 0;
}

/*
 * Return the volume of the audio device.
 *
 * Return 0-255 or -1 on error.
 */
int 
rplay_audio_get_volume (void)
{
#ifndef FAKE_VOLUME
    
    int mx;
    int mxdevmask;
    int left_vol;
    int right_vol;
    int vol;

    vol = left_vol = right_vol = 0;

    if ((mx = open (RPLAY_MIXER_DEVICE, O_RDONLY)) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: unable to open mixer device\n");
	return (-1);
    }

    if (ioctl (mx, SOUND_MIXER_READ_DEVMASK, &mxdevmask) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: unable to get mixer device mask\n");
	close (mx);
	return (-1);
    }

    if (!(mxdevmask & SOUND_MIXER_PCM))
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: pcm mixer device not installed\n");
	close (mx);
	return (-1);
    }

    if (ioctl (mx, SOUND_MIXER_READ_PCM, &vol) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: unable to get mixer volume\n");
	close (mx);
	return (-1);
    }
    else
    {
	left_vol = (int) ((vol & 0x00ff) * 2.55);
	right_vol = (int) (((vol & 0xff00) >> 8) * 2.55);

	vol = (int) ((left_vol + right_vol) / 2);

	report (REPORT_DEBUG, "current pcm-volume: %d:%d => %d\n", left_vol, right_vol, vol);
    }

    close (mx);

    return (vol);

#else
    return rplay_audio_volume;
#endif
}

/*
 * Set the volume of the audio device.
 * Input should be 0-255.
 *
 * Return the volume of the audio device 0-255 or -1.
 */
int 
rplay_audio_set_volume (int volume)
{
#ifndef FAKE_VOLUME

    int mx;
    int mxdevmask;
    int left_vol;
    int right_vol;
    int vol = (int) (volume / 2.55);

    vol |= ((vol & 0x00ff) << 8);

    if ((mx = open (RPLAY_MIXER_DEVICE, O_RDONLY)) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_set_volume: unable to open mixer device\n");
	return (-1);
    }

    if (ioctl (mx, SOUND_MIXER_READ_DEVMASK, &mxdevmask) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_set_volume: unable to get mixer device mask\n");
	close (mx);
	return (-1);
    }

    if (!(mxdevmask & SOUND_MIXER_PCM))
    {
	report (REPORT_ERROR, "rplay_audio_set_volume: pcm mixer device not installed\n");
	close (mx);
	return (-1);
    }

    if (ioctl (mx, SOUND_MIXER_WRITE_PCM, &vol) == -1)
    {
	report (REPORT_ERROR, "rplay_audio_set_volume: unable to set mixer volume\n");
	close (mx);
	return (-1);
    }
    else
    {
	left_vol = (int) ((vol & 0x00ff) * 2.55);
	right_vol = (int) (((vol & 0xff00) >> 8) * 2.55);

	vol = (int) ((left_vol + right_vol) / 2);

	report (REPORT_DEBUG, "rplay_audio_set_volume: pcm-volume set to %d:%d => %d\n",
		left_vol, right_vol, vol);
    }

    close (mx);

    rplay_audio_volume = vol;
    
    return rplay_audio_volume;

#else

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

#endif
}
