/* $Id: audio_generic.c,v 1.2 1998/08/13 06:14:17 boyns Exp $ */

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



#include "rplayd.h"

/*
 * System audio include files:
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#ifdef ultrix
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/errno.h>

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
static int rplay_audio_fd = -1;

static RPLAY_AUDIO_TABLE generic_table[] =
{
    {8000, RPLAY_FORMAT_ULAW, 1, 1},
    {0, 0, 0, 0}
};

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
    if (rplay_audio_fd == -1)
    {
	rplay_audio_open ();
	if (rplay_audio_fd == -1)
	{
	    report (REPORT_ERROR, "rplay_audio_init: cannot open %s\n",
		rplay_audio_device);
	    return -1;
	}
    }

    rplay_audio_sample_rate = 8000;
    rplay_audio_precision = 8;
    rplay_audio_channels = 1;
    rplay_audio_format = RPLAY_FORMAT_ULAW;
    rplay_audio_port = RPLAY_AUDIO_PORT_SPEAKER;
    rplay_audio_table = generic_table;

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
    int flags;

    rplay_audio_fd = open (rplay_audio_device, O_WRONLY | O_NDELAY, 0);
    if (rplay_audio_fd < 0)
    {
	return -1;
    }

    if (rplay_audio_init () < 0)
    {
	return -1;
    }

    /*
     * Make sure the audio device writes are non-blocking.
     */
    flags = fcntl (rplay_audio_fd, F_GETFL, 0);
    if (flags < 0)
    {
	return -1;
    }
    flags |= FNDELAY;
    if (fcntl (rplay_audio_fd, F_SETFL, flags) < 0)
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
    return rplay_audio_fd != -1;
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
    int n, nleft, nwritten;
    char *p;

    nleft = nbytes;
    nwritten = 0;

    for (p = buf; nleft > 0; nleft -= n, p += n)
    {
	n = write (rplay_audio_fd, p, nleft);
	if (n < 0)
	{
	    if (errno == EWOULDBLOCK)
	    {
		return nwritten;
	    }
	    else if (errno != EINTR)
	    {
		return -1;
	    }
	    n = 0;
	}
	else
	{
	    nwritten += n;
	}
    }

    return nwritten;
}

/*
 * Close the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_close ()
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
rplay_audio_get_volume ()
{
#ifdef FAKE_VOLUME
    return rplay_audio_volume;
#else /* not FAKE_VOLUME */
    return -1;
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
    return -1;
#endif /* not FAKE_VOLUME */
}
