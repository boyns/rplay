/* $Id: misc.c,v 1.4 1998/11/07 21:15:40 boyns Exp $ */

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
#include "rplayd.h"
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef ultrix
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/param.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <math.h>
#include "sound.h"
#include "timer.h"

#ifdef __STDC__
char *
sys_err_str(int error)
#else
char *
sys_err_str(error)
    int error;
#endif
{
#ifdef HAVE_STRERROR
    return (char *) strerror(error);
#else
    extern char *sys_errlist[];
    return sys_errlist[error];
#endif
}

#ifdef __STDC__
int
udp_socket(int port)
#else
int
udp_socket(port)
    int port;
#endif
{
    int fd;
    struct sockaddr_in s;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
	report(REPORT_ERROR, "socket: %s\n", sys_err_str(errno));
	done(1);
    }

    s.sin_family = AF_INET;
    s.sin_port = htons(port);
    s.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr *) &s, sizeof(s)) < 0)
    {
	report(REPORT_ERROR, "bind: %s\n", sys_err_str(errno));
	done(1);
    }

    return fd;
}

#ifdef __STDC__
int
tcp_socket(int port)
#else
int
tcp_socket(port)
    int port;
#endif
{
    int fd;
    struct sockaddr_in s;
    int on = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
	report(REPORT_ERROR, "socket: %s\n", sys_err_str(errno));
	done(1);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
    {
	report(REPORT_ERROR, "setsockopt: SO_REUSEADDR: %s\n", sys_err_str(errno));
	done(1);
    }

    s.sin_family = AF_INET;
    s.sin_port = htons(port);
    s.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr *) &s, sizeof(s)) < 0)
    {
	report(REPORT_ERROR, "bind: %s\n", sys_err_str(errno));
	done(1);
    }

    if (listen(fd, 5) < 0)
    {
	report(REPORT_ERROR, "listen: %s\n", sys_err_str(errno));
	done(1);
    }

    return fd;
}

/*
 * make a file descriptor non-blocking
 */
#ifdef __STDC__
void
fd_nonblock(int fd)
#else
void
fd_nonblock(fd)
    int fd;
#endif
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
	report(REPORT_ERROR, "fd_nonblock: F_GETFL fcntl: %s\n", sys_err_str(errno));
	done(1);
    }
#ifdef linux
    flags |= O_NONBLOCK;
#else
    flags |= FNDELAY;
#endif
    if (fcntl(fd, F_SETFL, flags) < 0)
    {
	report(REPORT_ERROR, "fd_nonblock: F_SETFL fcntl: %s\n", sys_err_str(errno));
	done(1);
    }
}

/*
 * make a file descriptor blocking
 */
#ifdef __STDC__
void
fd_block(int fd)
#else
void
fd_block(fd)
    int fd;
#endif
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
	report(REPORT_ERROR, "fd_block: F_GETFL fcntl: %s\n", sys_err_str(errno));
	done(1);
    }
#ifdef linux
    flags &= ~O_NONBLOCK;
#else
    flags &= ~FNDELAY;
#endif
    if (fcntl(fd, F_SETFL, flags) < 0)
    {
	report(REPORT_ERROR, "fd_block: F_SETFL fcntl: %s\n", sys_err_str(errno));
	done(1);
    }
}

#ifdef __STDC__
int
modified(char *filename, time_t since)
#else
int
modified(filename, since)
    char *filename;
    time_t since;
#endif
{
    struct stat st;

    if (stat(filename, &st) < 0)
    {
	report(REPORT_ERROR, "%s: %s\n", filename, sys_err_str(errno));
	return 0;
    }

    return ((time_t) st.st_mtime > since) ? 1 : 0;
}

#ifdef __STDC__
char *
time2string(time_t t)
#else
char *
time2string(t)
    time_t t;
#endif
{
    static char buf[64];
    int days, hours, mins, secs;

    days = t / (60 * 60 * 24);
    t %= (60 * 60 * 24);
    hours = t / (60 * 60);
    t %= (60 * 60);
    mins = t / 60;
    t %= 60;
    secs = t;

    buf[0] = '\0';

    if (days > 0)
    {
	SNPRINTF(SIZE(buf, sizeof(buf)), "%d+", days);
    }

    SNPRINTF(SIZE(buf + strlen(buf), sizeof(buf) - strlen(buf)), "%02d:%02d:%02d", hours, mins, secs);

    return buf;
}


#ifdef __STDC__
char *
audio_format_to_string(int format)
#else
char *
audio_format_to_string(format)
    int format;
#endif
{
    switch (format)
    {
    case RPLAY_FORMAT_ULAW:
	return "ulaw";

    case RPLAY_FORMAT_LINEAR_8:
	return "linear-8";

    case RPLAY_FORMAT_ULINEAR_8:
	return "ulinear-8";

    case RPLAY_FORMAT_LINEAR_16:
	return "linear-16";

    case RPLAY_FORMAT_ULINEAR_16:
	return "ulinear-16";

    case RPLAY_FORMAT_G721:
	return "g721";

    case RPLAY_FORMAT_G723_3:
	return "g723-3";

    case RPLAY_FORMAT_G723_5:
	return "g723-5";

    case RPLAY_FORMAT_GSM:
	return "gsm";

    default:
	return "unknown";
    }
}

#ifdef __STDC__
int
string_to_audio_format(char *string)
#else
int
string_to_audio_format(string)
    char *string;
#endif
{
    if (strcmp(string, "ulaw") == 0
	|| strcmp(string, "u_law") == 0
	|| strcmp(string, "u-law") == 0)
    {
	return RPLAY_FORMAT_ULAW;
    }
    else if (strcmp(string, "linear-16") == 0
	     || strcmp(string, "linear_16") == 0
	     || strcmp(string, "linear16") == 0)
    {
	return RPLAY_FORMAT_LINEAR_16;
    }
    else if (strcmp(string, "ulinear-16") == 0
	     || strcmp(string, "ulinear_16") == 0
	     || strcmp(string, "ulinear16") == 0)
    {
	return RPLAY_FORMAT_ULINEAR_16;
    }
    else if (strcmp(string, "linear-8") == 0
	     || strcmp(string, "linear_8") == 0
	     || strcmp(string, "linear8") == 0)
    {
	return RPLAY_FORMAT_LINEAR_8;
    }
    else if (strcmp(string, "ulinear-8") == 0
	     || strcmp(string, "ulinear_8") == 0
	     || strcmp(string, "ulinear8") == 0)
    {
	return RPLAY_FORMAT_ULINEAR_8;
    }
    else if (strcmp(string, "g721") == 0)
    {
	return RPLAY_FORMAT_G721;
    }
    else if (strcmp(string, "g723-3") == 0
	     || strcmp(string, "g723_3") == 0)
    {
	return RPLAY_FORMAT_G723_3;
    }
    else if (strcmp(string, "g723-5") == 0
	     || strcmp(string, "g723_5") == 0)
    {
	return RPLAY_FORMAT_G723_5;
    }
    else if (strcmp(string, "gsm") == 0
	     || strcmp(string, "GSM") == 0)
    {
	return RPLAY_FORMAT_GSM;
    }
    else
    {
	return RPLAY_FORMAT_NONE;
    }
}

#ifdef __STDC__
char *
byte_order_to_string(int byte_order)
#else
char *
byte_order_to_string(byte_order)
    int byte_order;
#endif
{
    switch (byte_order)
    {
    case RPLAY_BIG_ENDIAN:
	return "big-endian";

    case RPLAY_LITTLE_ENDIAN:
	return "little-endian";

    default:
	return "unknown";
    }
}

#ifdef __STDC__
int
string_to_byte_order(char *string)
#else
int
string_to_byte_order(string)
    char *string;
#endif
{
    if (strcmp(string, "big-endian") == 0
	|| strcmp(string, "big") == 0)
    {
	return RPLAY_BIG_ENDIAN;
    }
    else if (strcmp(string, "little-endian") == 0
	     || strcmp(string, "little") == 0)
    {
	return RPLAY_LITTLE_ENDIAN;
    }
    else
    {
	return 0;
    }
}

#ifdef __STDC__
char *
storage_to_string(int storage)
#else
char *
storage_to_string(storage)
    int storage;
#endif
{
    switch (storage)
    {
    case SOUND_STORAGE_NONE:
	return "none";

    case SOUND_STORAGE_DISK:
	return "disk";

    case SOUND_STORAGE_MEMORY:
	return "memory";

    default:
	return "unknown";
    }
}

#ifdef __STDC__
int
string_to_storage(char *string)
#else
int
string_to_storage(string)
    char *string;
#endif
{
    if (strcmp(string, "none") == 0)
    {
	return SOUND_STORAGE_NONE;
    }
    else if (strcmp(string, "disk") == 0)
    {
	return SOUND_STORAGE_DISK;
    }
    else if (strcmp(string, "memory") == 0)
    {
	return SOUND_STORAGE_MEMORY;
    }
    else
    {
	return SOUND_STORAGE_NULL;
    }
}

#ifdef __STDC__
char *
input_to_string(int input)
#else
char *
input_to_string(input)
    int input;
#endif
{
    switch (input)
    {
    case SOUND_FILE:
	return "file";

    case SOUND_FLOW:
	return "flow";

#ifdef HAVE_CDROM
    case SOUND_CDROM:
	return "cdrom";
#endif /* HAVE_CDROM */

    case SOUND_VIRTUAL:
	return "virtual";

    default:
	return "unknown";
    }
}

#ifdef __STDC__
int
string_to_input(char *string)
#else
int
string_to_input(string)
    char *string;
#endif
{
    if (strcmp(string, "file") == 0)
    {
	return SOUND_FILE;
    }
    else if (strcmp(string, "flow") == 0)
    {
	return SOUND_FLOW;
    }
    else
    {
	return 0;
    }
}

#ifdef __STDC__
char *
audio_port_to_string(int port)
#else
char *
audio_port_to_string(port)
    int port;
#endif
{
    static char string[128];
    int n;

    string[0] = '\0';
    if (BIT(port, RPLAY_AUDIO_PORT_NONE))
    {
	strncat(string, "none,", sizeof(string) - strlen(string));
    }
    if (BIT(port, RPLAY_AUDIO_PORT_SPEAKER))
    {
	strncat(string, "speaker,", sizeof(string) - strlen(string));
    }
    if (BIT(port, RPLAY_AUDIO_PORT_HEADPHONE))
    {
	strncat(string, "headphone,", sizeof(string) - strlen(string));
    }
    if (BIT(port, RPLAY_AUDIO_PORT_LINEOUT))
    {
	strncat(string, "lineout,", sizeof(string) - strlen(string));
    }
    string[strlen(string) - 1] = '\0';

    return string;
}

unsigned short
little_short(p)
    char *p;
{
    return (((unsigned long) (((unsigned char *) p)[1])) << 8) |
	((unsigned long) (((unsigned char *) p)[0]));
}

unsigned short
big_short(p)
    char *p;
{
    return (((unsigned long) (((unsigned char *) p)[0])) << 8) |
	((unsigned long) (((unsigned char *) p)[1]));
}

unsigned long
little_long(p)
    char *p;
{
    return (((unsigned long) (((unsigned char *) p)[3])) << 24) |
	(((unsigned long) (((unsigned char *) p)[2])) << 16) |
	(((unsigned long) (((unsigned char *) p)[1])) << 8) |
	((unsigned long) (((unsigned char *) p)[0]));
}

unsigned long
big_long(p)
    char *p;
{
    return (((unsigned long) (((unsigned char *) p)[0])) << 24) |
	(((unsigned long) (((unsigned char *) p)[1])) << 16) |
	(((unsigned long) (((unsigned char *) p)[2])) << 8) |
	((unsigned long) (((unsigned char *) p)[3]));
}

/*
 * C O N V E R T   F R O M   I E E E   E X T E N D E D  
 */

/* 
 * Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
#define HUGE_VAL HUGE
#endif /*HUGE_VAL */

#define UnsignedToFloat(u)         (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

double
ConvertFromIeeeExtended(bytes)
    unsigned char *bytes;	/* LCN */
{
    double f;
    int expon;
    unsigned long hiMant, loMant;

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant = ((unsigned long) (bytes[2] & 0xFF) << 24)
	| ((unsigned long) (bytes[3] & 0xFF) << 16)
	| ((unsigned long) (bytes[4] & 0xFF) << 8)
	| ((unsigned long) (bytes[5] & 0xFF));
    loMant = ((unsigned long) (bytes[6] & 0xFF) << 24)
	| ((unsigned long) (bytes[7] & 0xFF) << 16)
	| ((unsigned long) (bytes[8] & 0xFF) << 8)
	| ((unsigned long) (bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0)
    {
	f = 0;
    }
    else
    {
	if (expon == 0x7FFF)
	{			/* Infinity or NaN */
	    f = HUGE_VAL;
	}
	else
	{
	    expon -= 16383;
	    f = ldexp(UnsignedToFloat(hiMant), expon -= 31);
	    f += ldexp(UnsignedToFloat(loMant), expon -= 32);
	}
    }

    if (bytes[0] & 0x80)
	return -f;
    else
	return f;
}
