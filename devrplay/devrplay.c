/* $Id: devrplay.c,v 1.1 1999/03/03 06:21:54 boyns Exp $ */

/*
 * Copyright (C) 1999 Mark R. Boyns <boyns@doit.org>
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
 * based on esddsp.c
 * Copyright (C) 1998, 1999 Manish Singh <yosh@gimp.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include "rplay.h"

#ifdef linux

#include <sys/soundcard.h>
#include <dlfcn.h>
#define REAL_LIBC RTLD_NEXT

static int rplay_fd = -1;
static int event_fd = -1;
static int spool_id;
static int dsp_fmt;
static int dsp_speed;
static int dsp_channels;
static int dsp_speed;
static int dsp_blksize;
static int streaming;

int
open(const char *pathname, int flags,...)
{
    static int (*func)(const char *, int, mode_t) = NULL;
    va_list args;
    mode_t mode;
    char response[RPTP_MAX_LINE];

    if (!func)
	func = (int (*)(const char *, int, mode_t)) dlsym(REAL_LIBC, "open");

    va_start(args, flags);
    mode = va_arg(args, mode_t);
    va_end(args);

    if (strcmp(pathname, "/dev/dsp") == 0)
    {
	rplay_fd = rptp_open(rplay_default_host(), RPTP_PORT, response,
			     sizeof(response));
	if (rplay_fd < 0)
	{
	    rptp_perror(rplay_default_host());
	}
	return rplay_fd;
    }
    else
    {
	return (*func)(pathname, flags, mode);
    }
}

static int
dspctl(int fd, int request, void *argp)
{
    int *arg = (int *) argp;

    switch (request)
    {
    case SNDCTL_DSP_SETFMT:
	dsp_fmt = *arg;
	break;

    case SNDCTL_DSP_SPEED:
	dsp_speed = *arg;
	break;

    case SNDCTL_DSP_STEREO:
	dsp_channels = *arg ? 2 : 1;
	break;

    case SNDCTL_DSP_CHANNELS:
	dsp_channels = *arg;
	break;

    case SNDCTL_DSP_GETBLKSIZE:
	*arg = 65535;
	break;

    case SNDCTL_DSP_GETFMTS:
	*arg = 0x38;
	break;

    case SNDCTL_DSP_GETCAPS:
	*arg = 0;
	break;

    case SNDCTL_DSP_GETOSPACE:
	{
	    audio_buf_info *bufinfo = (audio_buf_info *) argp;
	    bufinfo->bytes = 4096;
	}
	break;

    default:
	break;
    }

    if (!spool_id && dsp_fmt && dsp_speed && dsp_channels)
    {
	char response[RPTP_MAX_LINE];
	rptp_putline(rplay_fd,
		     "play input=flow input-format=%s input-sample-rate=%d input-bits=%d input-channels=%d input-byte-order=%s input-offset=0 sound=\"%s\"",
		     dsp_fmt == 16 ? "linear16" : "ulinear8",
		     dsp_speed,
		     dsp_fmt,
		     dsp_channels,
		     "little-endian",
		     "devrplay");
	rptp_getline(rplay_fd, response, sizeof(response));

	spool_id = atoi(1 + rptp_parse(response, "id"));

	rptp_putline(rplay_fd, "put id=#%d size=0", spool_id);
	rptp_getline(rplay_fd, response, sizeof(response));

	streaming = 1;
    }

    return 0;
}

int
ioctl(int fd, int request,...)
{
    static int (*func)(int, int, void *) = NULL;
    va_list args;
    void *argp;

    if (!func)
	func = (int (*)(int, int, void *)) dlsym(REAL_LIBC, "ioctl");
    va_start(args, request);
    argp = va_arg(args, void *);
    va_end(args);

    if (fd != rplay_fd)
	return (*func)(fd, request, argp);
    else if (fd == rplay_fd)
	return dspctl(fd, request, argp);
}

#if 0
ssize_t
write(int fd, const void *buf, size_t count)
{
    static int (*func)(int, const void *, size_t) = NULL;

    if (!func)
	func = (int (*)(int, const void *, size_t)) dlsym(REAL_LIBC, "write");

    return (*func)(fd, buf, count);
}
#endif

int
close(int fd)
{
    static int (*func)(int) = NULL;

    if (!func)
	func = (int (*)(int)) dlsym(REAL_LIBC, "close");

    if (fd == rplay_fd)
    {
	rplay_fd = -1;
	streaming = spool_id = dsp_fmt = dsp_speed =
	    dsp_channels = dsp_speed = dsp_blksize = 0;
    }

    return (*func)(fd);
}

#endif /* linux */
