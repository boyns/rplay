/* $Id: audio_solaris.c,v 1.5 1999/03/10 07:58:10 boyns Exp $ */

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
#include <sys/audioio.h>
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
static int rplay_audio_fd = -1;

static RPLAY_AUDIO_TABLE amd_table[] =
{
    {8000, RPLAY_FORMAT_ULAW, 8, 1},
    {0, 0, 0, 0}
};

static RPLAY_AUDIO_TABLE dbri_table[] =
{
    {6615, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {6615, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {8000, RPLAY_FORMAT_ULAW, 8, 1},
    {8000, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {8000, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {9600, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {9600, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {11025, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {11025, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {16000, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {16000, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {18900, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {18900, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {22050, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {22050, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {32000, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {32000, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {33075, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {33075, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {37800, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {37800, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {44100, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {44100, RPLAY_FORMAT_LINEAR_16, 16, 2},
    {48000, RPLAY_FORMAT_LINEAR_16, 16, 1},
    {48000, RPLAY_FORMAT_LINEAR_16, 16, 2},
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
rplay_audio_init()
{
    audio_info_t a;
    audio_device_t d;

    if (rplay_audio_fd == -1)
    {
	rplay_audio_open();
	if (rplay_audio_fd == -1)
	{
	    report(REPORT_ERROR, "rplay_audio_init: cannot open %s\n",
		   rplay_audio_device);
	    return -1;
	}
    }

    if (ioctl(rplay_audio_fd, AUDIO_GETDEV, &d) < 0)
    {
	report(REPORT_ERROR, "rplay_audio_init: AUDIO_GETDEV: %s\n",
	       sys_err_str(errno));
	return -1;
    }

    if (strcmp(d.name, "SUNW,dbri") == 0)
    {
	report(REPORT_DEBUG, "%s device detected\n", d.name);
	rplay_audio_sample_rate = optional_sample_rate ? optional_sample_rate : 11025;
	rplay_audio_precision = optional_precision ? optional_precision : 16;
	rplay_audio_channels = optional_channels ? optional_channels : 1;
	rplay_audio_format = optional_format ? optional_format :
	    rplay_audio_precision == 16 ? RPLAY_FORMAT_LINEAR_16 : RPLAY_FORMAT_LINEAR_8;
	rplay_audio_port = optional_port ? optional_port : RPLAY_AUDIO_PORT_LINEOUT | RPLAY_AUDIO_PORT_SPEAKER;
	rplay_audio_table = dbri_table;
    }
    else if (strcmp(d.name, "SUNW,CS4231") == 0)
    {
	report(REPORT_DEBUG, "%s device detected\n", d.name);
	rplay_audio_sample_rate = optional_sample_rate ? optional_sample_rate : 11025;
	rplay_audio_precision = optional_precision ? optional_precision : 16;
	rplay_audio_channels = optional_channels ? optional_channels : 1;
	rplay_audio_format = optional_format ? optional_format :
	    rplay_audio_precision == 16 ? RPLAY_FORMAT_LINEAR_16 : RPLAY_FORMAT_LINEAR_8;
	rplay_audio_port = optional_port ? optional_port : RPLAY_AUDIO_PORT_LINEOUT | RPLAY_AUDIO_PORT_SPEAKER;
	rplay_audio_table = dbri_table;		/* use the dbri table */
    }
    else if (strcmp(d.name, "SUNW,am79c30") == 0)
    {
	report(REPORT_DEBUG, "%s device detected\n", d.name);
	rplay_audio_sample_rate = optional_sample_rate ? optional_sample_rate : 8000;
	rplay_audio_precision = optional_precision ? optional_precision : 8;
	rplay_audio_channels = optional_channels ? optional_channels : 1;
	rplay_audio_format = optional_format ? optional_format : RPLAY_FORMAT_ULAW;
	rplay_audio_port = optional_port ? optional_port : RPLAY_AUDIO_PORT_SPEAKER;
	rplay_audio_table = amd_table;
    }
    else if (strcmp(d.name, "SUNW,sb16") == 0)
    {
	report(REPORT_DEBUG, "%s device detected\n", d.name);
	rplay_audio_sample_rate = optional_sample_rate ? optional_sample_rate : 44100;
	rplay_audio_precision = optional_precision ? optional_precision : 16;
	rplay_audio_channels = optional_channels ? optional_channels : 2;
	rplay_audio_format = optional_format ? optional_format :
	    rplay_audio_precision == 16 ? RPLAY_FORMAT_LINEAR_16 : RPLAY_FORMAT_LINEAR_8;
	rplay_audio_port = optional_port ? optional_port : RPLAY_AUDIO_PORT_LINEOUT | RPLAY_AUDIO_PORT_SPEAKER;
	rplay_audio_table = dbri_table;		/* use the dbri table */
    }
    else
    {
	report(REPORT_ERROR, "`%s' unknown audio device detected\n", d.name);
	return -1;
    }

    /* Verify the precision and format. */
    switch (rplay_audio_precision)
    {
    case 8:
	if (rplay_audio_format != RPLAY_FORMAT_ULAW
	    && rplay_audio_format != RPLAY_FORMAT_LINEAR_8)
	{
	    report(REPORT_ERROR, "rplay_audio_init: can't use %d bits with format=%d\n",
		   rplay_audio_precision, rplay_audio_format);
	    return -1;
	}
	break;

    case 16:
	if (rplay_audio_format != RPLAY_FORMAT_LINEAR_16)
	{
	    report(REPORT_ERROR, "rplay_audio_init: can't use %d bits with format=%d\n",
		   rplay_audio_precision, rplay_audio_format);
	    return -1;
	}
	break;

    default:
	report(REPORT_ERROR, "rplay_audio_init: `%d' unsupported audio precision\n",
	       rplay_audio_precision);
	return -1;
    }

    AUDIO_INITINFO(&a);

    switch (rplay_audio_format)
    {
    case RPLAY_FORMAT_ULAW:
	a.play.encoding = AUDIO_ENCODING_ULAW;
	break;

    case RPLAY_FORMAT_LINEAR_8:
    case RPLAY_FORMAT_LINEAR_16:
	a.play.encoding = AUDIO_ENCODING_LINEAR;
	break;

    default:
	report(REPORT_ERROR, "rplay_audio_init: unsupported audio format `%d'\n",
	       rplay_audio_format);
	return -1;
    }

    /* Audio port. */
    if (rplay_audio_port == RPLAY_AUDIO_PORT_NONE)
    {
	a.play.port = ~0;	/* see AUDIO_INITINFO in /usr/include/sys/audioio.h. */
    }
    else
    {
	a.play.port = 0;
	if (BIT(rplay_audio_port, RPLAY_AUDIO_PORT_LINEOUT))
	{
#ifdef AUDIO_LINE_OUT
	    SET_BIT(a.play.port, AUDIO_LINE_OUT);
#else
	    CLR_BIT(rplay_audio_port, RPLAY_AUDIO_PORT_LINEOUT);
#endif
	}
	if (BIT(rplay_audio_port, RPLAY_AUDIO_PORT_HEADPHONE))
	{
#ifdef AUDIO_HEADPHONE
	    SET_BIT(a.play.port, AUDIO_HEADPHONE);
#else
	    CLR_BIT(rplay_audio_port, RPLAY_AUDIO_PORT_HEADPHONE);
#endif
	}
	if (BIT(rplay_audio_port, RPLAY_AUDIO_PORT_SPEAKER))
	{
#ifdef AUDIO_SPEAKER
	    SET_BIT(a.play.port, AUDIO_SPEAKER);
#endif
	    /* Assume speaker is okay. */
	}
    }

    a.play.sample_rate = rplay_audio_sample_rate;
    a.play.precision = rplay_audio_precision;
    a.play.channels = rplay_audio_channels;

    if (ioctl(rplay_audio_fd, AUDIO_SETINFO, &a) < 0)
    {
	report(REPORT_ERROR, "rplay_audio_init: AUDIO_SETINFO: %s\n", sys_err_str(errno));
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

    rplay_audio_fd = open(rplay_audio_device, O_WRONLY | O_NDELAY, 0);
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

    if (rplay_audio_init() < 0)
    {
	return -1;
    }

    /*
     * Make sure the audio device writes are non-blocking.
     */
    flags = fcntl(rplay_audio_fd, F_GETFL, 0);
    if (flags < 0)
    {
	return -1;
    }
    flags |= FNDELAY;
    if (fcntl(rplay_audio_fd, F_SETFL, flags) < 0)
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
	ioctl(rplay_audio_fd, AUDIO_DRAIN, 0);
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
    int n, nleft, nwritten;
    char *p;

    nleft = nbytes;
    nwritten = 0;

    for (p = buf; nleft > 0; nleft -= n, p += n)
    {
	n = write(rplay_audio_fd, p, nleft);
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
rplay_audio_close()
{
    if (rplay_audio_fd != -1)
    {
	close(rplay_audio_fd);
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
rplay_audio_get_volume()
{
#ifdef FAKE_VOLUME
    return rplay_audio_volume;
#else /* not FAKE_VOLUME */
    audio_info_t a;

    if (rplay_audio_fd < 0)
    {
	rplay_audio_open();
    }
    if (rplay_audio_fd < 0)
    {
	return -1;
    }
    if (ioctl(rplay_audio_fd, AUDIO_GETINFO, &a) < 0)
    {
	return -1;
    }
    else
    {
	return a.play.gain;
    }
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
    audio_info_t a;

    rplay_audio_volume = 0;

    if (rplay_audio_fd < 0)
    {
	rplay_audio_open();
    }
    if (rplay_audio_fd < 0)
    {
	return -1;
    }

    AUDIO_INITINFO(&a);
    a.play.gain = volume;
    if (ioctl(rplay_audio_fd, AUDIO_SETINFO, &a) < 0)
    {
	return -1;
    }

    rplay_audio_volume = rplay_audio_get_volume();

    return rplay_audio_volume;
#endif /* not FAKE_VOLUME */
}
