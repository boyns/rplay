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



/*
 * The _hpux version was cobbled together from the _generic version by Hendrik
 * (J.C.Harrison@ncl.ac.uk).
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
#include <sys/audio.h>
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
    struct audio_describe ad;
    struct audio_status as;
    int af;			/* audio_format */
    int ar;			/* audio_rate   */
    int ao;			/* audio_output */
    int ac;			/* audio_channels */

    /* Open the audio device */
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

    /* Reset the audio device */
    if (ioctl (rplay_audio_fd, AUDIO_RESET, RESET_RX_BUF | RESET_TX_BUF | RESET_RX_OVF | RESET_TX_UNF) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_RESET: %s\n", sys_err_str (errno));
	return -1;
    }

    /* Interrogate the audio device */
    if (ioctl (rplay_audio_fd, AUDIO_DESCRIBE, &ad) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_DESCRIBE: %s\n", sys_err_str (errno));
	return -1;
    }

    if (ioctl (rplay_audio_fd, AUDIO_GET_STATUS, &as) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_GET_STATUS: %s\n", sys_err_str (errno));
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_GET_SAMPLE_RATE, &ar) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_GET_SAMPLE_RATE: %s\n", sys_err_str (errno));
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_GET_DATA_FORMAT, &af) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_GET_DATA_FORMAT: %s\n", sys_err_str (errno));
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_GET_OUTPUT, &ao) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_GET_OUTPUT: %s\n", sys_err_str (errno));
	return -1;
    }

#ifdef DEBUG
    printf ("*** AUDIO_DESCRIBE DATA ***\n");
    printf ("audio_id:\t\t%d\tnrates:\t\t\t%d\tflags:\t%X\n",
	    ad.audio_id, ad.nrates, ad.flags);
    printf ("max_bits_per_sample:\t%d\tnchannels:\t\t%d\n",
	    ad.max_bits_per_sample, ad.nchannels);
    printf ("min_receive_gain:\t%d\tmin_transmit_gain:\t%d\tmin_monitor_gain:\t%d\n",
	    ad.min_receive_gain, ad.min_transmit_gain, ad.min_monitor_gain);
    printf ("max_receive_gain:\t%d\tmax_transmit_gain:\t%d\tmax_monitor_gain:\t%d\n",
	    ad.max_receive_gain, ad.max_transmit_gain, ad.max_monitor_gain);
    printf ("*** AUDIO_STATUS DATA ***\n");
    printf ("receive_status:  %x\treceive_buffer_count:  %d\treceive_overflow_count:   %d\n",
	    as.receive_status, as.receive_buffer_count, as.receive_overflow_count);
    printf ("transmit_status: %x\ttransmit_buffer_count: %d\ttransmit_underflow_count: %d\n",
	    as.transmit_status, as.transmit_buffer_count, as.transmit_underflow_count);
    printf ("*** END DATA ***\n");
#endif

    /* Set internal configuration */

    /* Having tried sample rates from 0 upto 500000 in steps of 1, the device will
       only accept a rate of 8000Hz. Dunno about any other models of HP though. */

    rplay_audio_sample_rate = ar = optional_sample_rate ? optional_sample_rate : 8000;
    rplay_audio_channels = ac = optional_channels ? optional_channels : ad.nchannels;

    /* Set the channel mask according to the number of channels allowed */
    ac = (ac == 2) ? (AUDIO_CHANNEL_LEFT | AUDIO_CHANNEL_RIGHT) :
	((ac == 1) ? (AUDIO_CHANNEL_LEFT) : 0);

    /* Only Ulaw, Alaw and linear 16-bit formats are allowed - anyone know what Alaw is ? */
    rplay_audio_format = optional_format ?
	((optional_format == RPLAY_FORMAT_ULAW) ? RPLAY_FORMAT_ULAW :
	RPLAY_FORMAT_LINEAR_16) : RPLAY_FORMAT_LINEAR_16;
    af = (rplay_audio_format == RPLAY_FORMAT_ULAW) ? AUDIO_FORMAT_ULAW :
	AUDIO_FORMAT_LINEAR16BIT;

    /* Precision cannot be set seperatly to sound format (I think) */
    rplay_audio_precision = (af == AUDIO_FORMAT_LINEAR16BIT) ? 16 : 8;

    /* Set the output device - how should this be specified ? */
    rplay_audio_port = optional_port ? optional_port :
	RPLAY_AUDIO_PORT_SPEAKER | RPLAY_AUDIO_PORT_HEADPHONE; /* default ports */
    ao = 0;
    if (BIT (rplay_audio_port, RPLAY_AUDIO_PORT_LINEOUT))
    {
#ifdef AUDIO_LINE_OUT
	SET_BIT (ao, AUDIO_LINE_OUT);
#endif
#ifdef AUDIO_OUT_LINE
	SET_BIT (ao, AUDIO_OUT_LINE);
#endif
    }
    if (BIT (rplay_audio_port, RPLAY_AUDIO_PORT_HEADPHONE))
    {
	SET_BIT (ao, AUDIO_OUT_HEADPHONE);
    }
    if (BIT (rplay_audio_port, RPLAY_AUDIO_PORT_SPEAKER))
    {
	SET_BIT (ao, AUDIO_OUT_SPEAKER);
    }

    /* Program the audio device */
    if (ioctl (rplay_audio_fd, AUDIO_SET_SAMPLE_RATE, ar) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_SET_SAMPLE_RATE: %s\n", sys_err_str (errno));
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_SET_CHANNELS, ac) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_SET_CHANNELS: %s\n", sys_err_str (errno));
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_SET_DATA_FORMAT, af) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_SET_DATA_FORMAT: %s\n", sys_err_str (errno));
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_SET_OUTPUT, ao) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_init: AUDIO_SET_OUTPUTT: %s\n", sys_err_str (errno));
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
rplay_audio_open ()
{
    int flags;

    report (REPORT_DEBUG, "opening device >%s<\n", rplay_audio_device);

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
    return (rplay_audio_fd != -1);
}

/*
 * Flush the audio device.
 *
 * Return 0 on success and -1 on error.
 */
int
rplay_audio_flush ()
{
    if (rplay_audio_fd != -1)
    {
	if (ioctl (rplay_audio_fd, AUDIO_DRAIN, 0) < 0)
	{
	    report (REPORT_ERROR, "rplay_audio_init: AUDIO_DRAIN: %s\n", sys_err_str (errno));
	}
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
	if ((n = write (rplay_audio_fd, p, nleft)) < 0)
	{
/*                      report(RPLAY_DEBUG, "rplay_audio_write: %s\n", sys_err_str(errno)); */

	    if (errno == EWOULDBLOCK)
	    {
		return nwritten;
	    }
	    else if (errno != EINTR)
	    {
		report (REPORT_ERROR, "rplay_audio_write: %s\n", sys_err_str (errno));
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
    struct audio_gain ag;
    struct audio_describe ad;

    if (rplay_audio_fd < 0)
    {
	rplay_audio_open ();
    }
    if (rplay_audio_fd < 0)
    {
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_GET_GAINS, &ag) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: AUDIO_GET_GAINS: %s\n", sys_err_str (errno));
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_DESCRIBE, &ad) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: AUDIO_DESCRIBE: %s\n", sys_err_str (errno));
	return -1;
    }

#ifdef DEBUG
    printf ("audio.c: ag.cgain[1].receive_gain: %d\tag.cgain[1].transmit_gain: %d\tag.cgain[1].monitor_gain: %d\n",
	    ag.cgain[1].receive_gain, ag.cgain[1].transmit_gain, ag.cgain[1].monitor_gain);
    printf ("audio.c: ag.cgain[2].receive_gain: %d\tag.cgain[2].transmit_gain: %d\tag.cgain[2].monitor_gain: %d\n",
	    ag.cgain[2].receive_gain, ag.cgain[2].transmit_gain, ag.cgain[2].monitor_gain);
    printf ("audio.c: channel_mask: %d\n", ag.channel_mask);
#endif

    return ((unsigned char) (
	    ((double) ag.cgain[1].transmit_gain - ad.min_transmit_gain)
	    / (ad.max_transmit_gain - ad.min_transmit_gain)
	    * 256));
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
    struct audio_gain ag;
    struct audio_describe ad;

    rplay_audio_volume = 0;

    if (rplay_audio_fd < 0)
    {
	rplay_audio_open ();
    }
    if (rplay_audio_fd < 0)
    {
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_GET_GAINS, &ag) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: AUDIO_GET_GAINS: %s\n", sys_err_str (errno));
	return -1;
    }
    if (ioctl (rplay_audio_fd, AUDIO_DESCRIBE, &ad) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: AUDIO_DESCRIBE: %s\n", sys_err_str (errno));
	return -1;
    }

#ifdef DEBUG
    printf ("ag.cgain[1].receive_gain: %d\tag.cgain[1].transmit_gain: %d\tag.cgain[1].monitor_gain: %d\n",
	    ag.cgain[1].receive_gain, ag.cgain[1].transmit_gain, ag.cgain[1].monitor_gain);
    printf ("ag.cgain[2].receive_gain: %d\tag.cgain[2].transmit_gain: %d\tag.cgain[2].monitor_gain: %d\n",
	    ag.cgain[2].receive_gain, ag.cgain[2].transmit_gain, ag.cgain[2].monitor_gain);
    printf ("channel_mask: %d\n", ag.channel_mask);
#endif

    ag.cgain[1].transmit_gain = ((int) (
	    (((double) volume)
		* (ad.max_transmit_gain - ad.min_transmit_gain)
		/ 256)
	    + ad.min_transmit_gain));

#ifdef DEBUG    
    printf ("ag.cgain[1].receive_gain: %d\tag.cgain[1].transmit_gain: %d\tag.cgain[1].monitor_gain: %d\n",
	    ag.cgain[1].receive_gain, ag.cgain[1].transmit_gain, ag.cgain[1].monitor_gain);
    printf ("ag.cgain[2].receive_gain: %d\tag.cgain[2].transmit_gain: %d\tag.cgain[2].monitor_gain: %d\n",
	    ag.cgain[2].receive_gain, ag.cgain[2].transmit_gain, ag.cgain[2].monitor_gain);
    printf ("channel_mask: %d\n", ag.channel_mask);
#endif    

    if (ioctl (rplay_audio_fd, AUDIO_SET_GAINS, ag) < 0)
    {
	report (REPORT_ERROR, "rplay_audio_get_volume: AUDIO_SET_GAINS: %s\n", sys_err_str (errno));
	return -1;
    }

    rplay_audio_volume = rplay_audio_get_volume ();

    return rplay_audio_volume;
#endif /* not FAKE_VOLUME */
}
