/* $Id: audio_oss.c,v 1.5 1999/03/10 07:58:10 boyns Exp $ */

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



#include "rplayd.h"

/*
 * System audio include files:
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <errno.h>
#include <sys/soundcard.h>

/*
 * External variables:
 */
extern char *rplay_audio_device;
extern int rplay_audio_sample_rate;
extern int rplay_audio_channels;
extern int rplay_audio_precision;
extern int rplay_audio_format;
extern int rplay_audio_port;
extern int rplay_audio_fragsize;
extern int optional_sample_rate;
extern int optional_precision;
extern int optional_channels;
extern int optional_format;
extern int optional_port;
extern int optional_fragsize;

/*
 * Internal variables:
 */
static int rplay_audio_fd = -1;
static int rplay_audio_mixer_fd = -1;

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
rplay_audio_init()
{
    int n;

    if (rplay_audio_fd == -1)
    {
	rplay_audio_open();
	if (rplay_audio_fd == -1)
	{
	    report(REPORT_ERROR, "rplay_audio_init: cannot open %s\n", rplay_audio_device);
	    return -1;
	}
    }

    /* /dev/audio */
    if (strcmp(rplay_audio_device, "/dev/audio") == 0)
    {
	rplay_audio_sample_rate = 8000;
	rplay_audio_precision = 8;
	rplay_audio_channels = 1;
	rplay_audio_format = RPLAY_FORMAT_ULAW;
	rplay_audio_set_port();
    }
    /* /dev/dsp */
    else if (strcmp(rplay_audio_device, "/dev/dsp") == 0)
    {
	rplay_audio_sample_rate = optional_sample_rate ? optional_sample_rate : 44100;
	rplay_audio_precision = optional_precision ? optional_precision : 16;
	rplay_audio_channels = optional_channels ? optional_channels : 2;
	rplay_audio_set_port();

	/* Precision */
	n = rplay_audio_precision;
	if (ioctl(rplay_audio_fd, SNDCTL_DSP_SETFMT, &n) == -1)
	{
	    report(REPORT_ERROR, "rplay_audio_init: can't set audio precision to %d (%d)\n",
		   rplay_audio_precision, n);
	    return -1;
	}
	if (n != rplay_audio_precision)
	{
	    report(REPORT_NOTICE, "rplay_audio_init: audio precision changed from %d to %d\n",
		   rplay_audio_precision, n);
	    rplay_audio_precision = n;
	}

	/* Channels */
	n = rplay_audio_channels;
	if (ioctl(rplay_audio_fd, SNDCTL_DSP_CHANNELS, &n) == -1)
	{
	    report(REPORT_ERROR, "rplay_audio_init: can't set audio channels to %d (%d)\n",
		   rplay_audio_channels, n);
	    return -1;
	}
	if (n != rplay_audio_channels)
	{
	    report(REPORT_NOTICE, "rplay_audio_init: audio channels changed from %d to %d\n",
		   rplay_audio_channels, n);
	    rplay_audio_channels = n;
	}

	/* Sample rate */
	n = rplay_audio_sample_rate;
	ioctl(rplay_audio_fd, SNDCTL_DSP_SYNC, NULL);
	if (ioctl(rplay_audio_fd, SNDCTL_DSP_SPEED, &n) == -1)
	{
	    report(REPORT_ERROR, "rplay_audio_init: can't set audio sample rate to %d (%d)\n",
		   rplay_audio_sample_rate, n);
	    return -1;
	}
	if (n != rplay_audio_sample_rate)
	{
	    report(REPORT_NOTICE, "rplay_audio_init: audio sample rate changed from %d to %d\n",
		   rplay_audio_sample_rate, n);
	    rplay_audio_sample_rate = n;
	}

	/* Format */
	rplay_audio_format = optional_format ? optional_format :
	    rplay_audio_precision == 16 ? RPLAY_FORMAT_LINEAR_16 : RPLAY_FORMAT_ULINEAR_8;
    }
    else
    {
	report(REPORT_ERROR, "rplay_audio_init: `%s' unknown audio device\n",
	       rplay_audio_device);
	return -1;
    }

    return 0;
}

/*
 * Open the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_open()
{
    int flags;
    audio_buf_info info;
    int n, i;

    rplay_audio_fd = open(rplay_audio_device, O_WRONLY, 0);
    if (rplay_audio_fd < 0)
    {
	return -1;
    }

    if (fcntl(rplay_audio_fd, F_SETFD, 1) < 0)
    {
	report(REPORT_ERROR,
	       "rplay_audio_open: close-on-exec %d\n",
	       sys_err_str(errno));
	/* return -1; */
    }

    /*
       From the OSS driver docs:

       Argument of this call is an integer encoded as 0xMMMMSSSS (in hex).

       The 16 least significant bits determine the fragment size. The size is
       2SSSS. For example SSSS=0008 gives fragment size of 256 bytes (28).
       The minimum is 16 bytes (SSSS=4) and the maximum is
       total_buffer_size/2. Some devices or processor architectures may
       require larger fragments in this case the requested fragment size is
       automatically increased.

       The 16 most significant bits (MMMM) determine maximum number of
       fragments. By default the deriver computes this based on available
       buffer space. The minimum value is 2 and the maximum depends on the
       situation. Set MMMM=0x7fff if you don't want to limit the number of
       fragments.
     */

    /* Set the audio buffer fragment size.  Default to zero which
       lets the driver pick. */
    rplay_audio_fragsize = optional_fragsize ? optional_fragsize : 0;
    if (rplay_audio_fragsize)
    {
	n = rplay_audio_fragsize;
    }
    else
    {
	n = 4096;
    }

    for (i = 0; n > 1; i++)
    {
	n >>= 1;
    }

    rplay_audio_setfragsize((0x7fff << 16) | i);

    ioctl(rplay_audio_fd, SNDCTL_DSP_GETOSPACE, &info);
    report(REPORT_DEBUG, "OSS info: fragments=%d totalfrags=%d fragsize=%d DSP=%d\n",
	   info.fragments, info.fragstotal, info.fragsize, info.fragsize * info.fragstotal);

    rplay_audio_fragsize = info.fragsize;

    if (rplay_audio_init() < 0)
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
rplay_audio_isopen()
{
    return rplay_audio_fd != -1;
}

/*
 * Flush the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_flush()
{
    if (rplay_audio_fd != -1)
    {
	ioctl(rplay_audio_fd, SNDCTL_DSP_SYNC, 0);
    }

    return 0;
}

/*
 * Write nbytes from buf to the audio device.
 *
 * Return the number of bytes written on success and -1 on error.
 */
#ifdef __STDC__
int
rplay_audio_write(char *buf, int nbytes)
#else
int
rplay_audio_write(buf, nbytes)
    char *buf;
    int nbytes;
#endif
{
    return write(rplay_audio_fd, buf, nbytes);
}

/*
 * Close the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_close()
{
    if (rplay_audio_fd != -1)
    {
	close(rplay_audio_fd);
    }

    rplay_audio_fd = -1;

    return 0;
}

int
rplay_audio_set_port()
{
    int mask = 0;
    int vol = 0;

    if (rplay_audio_mixer_fd == -1)
    {
	if (rplay_audio_mixer_open() < 0)
	{
	    return -1;
	}
    }

    if (ioctl(rplay_audio_mixer_fd, SOUND_MIXER_READ_DEVMASK, &mask) < 0)
    {
	return -1;
    }

    rplay_audio_port = optional_port ? optional_port :
	RPLAY_AUDIO_PORT_LINEOUT | RPLAY_AUDIO_PORT_SPEAKER | RPLAY_AUDIO_PORT_HEADPHONE;

    /* Clear any ports that aren't available. */
    if (BIT(rplay_audio_port, RPLAY_AUDIO_PORT_LINEOUT | RPLAY_AUDIO_PORT_HEADPHONE))
    {
	if (!(mask & SOUND_MASK_VOLUME))
	{
	    CLR_BIT(rplay_audio_port, RPLAY_AUDIO_PORT_LINEOUT | RPLAY_AUDIO_PORT_HEADPHONE);
	}
    }
    if (BIT(rplay_audio_port, RPLAY_AUDIO_PORT_SPEAKER))
    {
	if (!(mask & SOUND_MASK_SPEAKER))
	{
	    CLR_BIT(rplay_audio_port, RPLAY_AUDIO_PORT_SPEAKER);
	}
    }

    if (BIT(rplay_audio_port, RPLAY_AUDIO_PORT_LINEOUT | RPLAY_AUDIO_PORT_HEADPHONE))
    {
	rplay_audio_port |= RPLAY_AUDIO_PORT_LINEOUT | RPLAY_AUDIO_PORT_HEADPHONE;

	/* Make sure the volume isn't zero. */
	vol = 0;
	ioctl(rplay_audio_mixer_fd, SOUND_MIXER_READ_VOLUME, &vol);
	if (vol == 0)
	{
	    vol = 50;
	    vol |= ((vol & 0x00ff) << 8);
	    ioctl(rplay_audio_mixer_fd, SOUND_MIXER_WRITE_VOLUME, &vol);
	}
    }
    else
    {
	/* Zero the volume. */
	vol = 0;
	ioctl(rplay_audio_mixer_fd, SOUND_MIXER_WRITE_VOLUME, &vol);
    }

    if (BIT(rplay_audio_port, RPLAY_AUDIO_PORT_SPEAKER))
    {
	vol = 0;
	ioctl(rplay_audio_mixer_fd, SOUND_MIXER_READ_SPEAKER, &vol);
	/* Make sure the volume isn't zero. */
	if (vol == 0)
	{
	    vol = 50;
	    vol |= ((vol & 0x00ff) << 8);
	    ioctl(rplay_audio_mixer_fd, SOUND_MIXER_WRITE_SPEAKER, &vol);
	}
    }
    else
    {
	/* Zero the volume. */
	vol = 0;
	ioctl(rplay_audio_mixer_fd, SOUND_MIXER_WRITE_SPEAKER, &vol);
    }
}

/*
 * Return the volume of the audio device.
 *
 * Return 0-255 or -1 on error.
 */
int
rplay_audio_get_volume()
{
#ifdef FAKE_VOLUME
    return rplay_audio_volume;
#else /* not FAKE_VOLUME */
    int mask;
    int left_vol;
    int right_vol;
    int vol;

    vol = left_vol = right_vol = 0;

    if (rplay_audio_mixer_fd == -1)
    {
	if (rplay_audio_mixer_open() < 0)
	{
	    return -1;
	}
    }

    if (ioctl(rplay_audio_mixer_fd, SOUND_MIXER_READ_DEVMASK, &mask) < 0)
    {
	report(REPORT_ERROR, "rplay_audio_get_volume: unable to get mixer device mask\n");
	return -1;
    }

    if (!(mask & SOUND_MASK_PCM))
    {
	report(REPORT_ERROR, "rplay_audio_get_volume: pcm mixer device not installed\n");
	return -1;
    }

    if (ioctl(rplay_audio_mixer_fd, SOUND_MIXER_READ_PCM, &vol) < 0)
    {
	report(REPORT_ERROR, "rplay_audio_get_volume: unable to get mixer volume\n");
	return -1;
    }
    else
    {
	left_vol = (int) ((vol & 0x00ff) * 2.55);
	right_vol = (int) (((vol & 0xff00) >> 8) * 2.55);
	vol = (int) ((left_vol + right_vol) / 2);
    }

    return vol;
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
rplay_audio_set_volume(int volume)
#else
int
rplay_audio_set_volume(volume)
    int volume;
#endif
{
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
    int mask;
    int left_vol;
    int right_vol;
    int vol = (int) (volume / 2.55);

    vol |= ((vol & 0x00ff) << 8);

    if (rplay_audio_mixer_fd == -1)
    {
	if (rplay_audio_mixer_open() < 0)
	{
	    return -1;
	}
    }

    if (ioctl(rplay_audio_mixer_fd, SOUND_MIXER_READ_DEVMASK, &mask) < 0)
    {
	report(REPORT_ERROR, "rplay_audio_set_volume: unable to get mixer device mask\n");
	return -1;
    }

    if (!(mask & SOUND_MASK_PCM))
    {
	report(REPORT_ERROR, "rplay_audio_set_volume: pcm mixer device not installed\n");
	return -1;
    }

    if (ioctl(rplay_audio_mixer_fd, SOUND_MIXER_WRITE_PCM, &vol) < 0)
    {
	report(REPORT_ERROR, "rplay_audio_set_volume: unable to set mixer volume\n");
	return -1;
    }
    else
    {
	left_vol = (int) ((vol & 0x00ff) * 2.55);
	right_vol = (int) (((vol & 0xff00) >> 8) * 2.55);
	vol = (int) ((left_vol + right_vol) / 2);
    }

    rplay_audio_volume = vol;

    return rplay_audio_volume;
#endif /* not FAKE_VOLUME */
}

int
rplay_audio_getblksize()
{
    int blksize;

    if (rplay_audio_fd == -1)
    {
	return -1;
    }

    if (ioctl(rplay_audio_fd, SNDCTL_DSP_GETBLKSIZE, &blksize) < 0)
    {
	report(REPORT_NOTICE, "DSP_GETBLKSIZE failed\n");
	return -1;
    }
    else
    {
	return blksize;
    }
}

#ifdef __STDC__
int
rplay_audio_setfragsize(int frag)
#else
int
rplay_audio_setfragsize(frag)
    int frag;
#endif
{
    int n;

    if (rplay_audio_fd == -1)
    {
	return -1;
    }

    n = ioctl(rplay_audio_fd, SNDCTL_DSP_SETFRAGMENT, &frag);
    if (n < 0)
    {
	report(REPORT_NOTICE, "DSP_SETFRAGMENT failed n = %d\n", n);
	return -1;
    }

    return 0;
}

int
rplay_audio_getospace_bytes()
{
    audio_buf_info info;

    if (rplay_audio_fd < 0)
    {
	return 0;
    }
    ioctl(rplay_audio_fd, SNDCTL_DSP_GETOSPACE, &info);
/*     printf ("frags=%d fragtotal=%d fragsize=%d bytes=%d\n", */
/*          info.fragments, info.fragstotal, info.fragsize, info.bytes); */
    return info.bytes;
}

int
rplay_audio_getospace_fragsize()
{
    audio_buf_info info;

    if (rplay_audio_fd < 0)
    {
	return 0;
    }
    ioctl(rplay_audio_fd, SNDCTL_DSP_GETOSPACE, &info);
    return info.fragsize;
}

int
rplay_audio_getfd()
{
    return rplay_audio_fd;
}

int
rplay_audio_mixer_open()
{
    rplay_audio_mixer_fd = open(RPLAY_AUDIO_MIXER_DEVICE, O_RDONLY);
    if (rplay_audio_mixer_fd < 0)
    {
	report(REPORT_ERROR, "rplay_audio_mixer_open: unable to open mixer device\n");
	return -1;
    }

    return 0;
}

int
rplay_audio_mixer_close()
{
    if (rplay_audio_mixer_fd != -1)
    {
	close(rplay_audio_mixer_fd);
	rplay_audio_mixer_fd = -1;
    }
}

int
rplay_audio_mixer_isopen()
{
    return rplay_audio_mixer_fd != -1;
}
