/* $Id: cdrom.c,v 1.5 2002/12/11 05:12:16 boyns Exp $ */

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



/* A lot of the CDDA code is based on read_cdda 1.01 by Jim Mintha
   (mintha@geog.ubc.ca) which seems to have borrowed code from Workman
   sources written by Steven Grimm (koreth@hyperion.com). */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_CDROM

#include <sys/types.h>
#include <sys/errno.h>
#include <unistd.h>
#ifdef ultrix
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/ioctl.h>
#include "rplayd.h"
#include "cdrom.h"

CDROM_TABLE cdrom_table[MAX_CDROMS] =
{
#if defined (sun) && defined (SVR4)	/* Solaris 2.x */
    {"cdrom:", "/vol/dev/aliases/cdrom0", 0, 0},
    {"cdrom0:", "/vol/dev/aliases/cdrom0", 0, 0},
    {"cdrom1:", "/vol/dev/aliases/cdrom1", 0, 0},
    {"cdrom2:", "/vol/dev/aliases/cdrom2", 0, 0},
    {"cdrom3:", "/vol/dev/aliases/cdrom3", 0, 0},
#else				/* not solaris */
#if defined (linux)
    {"cdrom:", "/dev/cdrom", 0, 0},
    {"cdrom0:", "/dev/cdrom0", 0, 0},
    {"cdrom1:", "/dev/cdrom1", 0, 0},
    {"cdrom2:", "/dev/cdrom2", 0, 0},
    {"cdrom3:", "/dev/cdrom3", 0, 0},
#endif				/* linux */
#endif
};

typedef struct
{
    char *device;
    int starting_track;
    int ending_track;
    int num_blocks;
    int fd;
    char *buffer;
    int buffer_length;
    int starting_block;
    int current_block;
    int ending_block;
}
CDROM_INFO;

/* Internal prototypes. */
#ifdef HAVE_CDDA
#ifdef __STDC__
static void cdda_cdrom_reader(CDROM_TABLE * cdt, int starting_track, int ending_track, int output_fd);
#else
static void cdda_cdrom_reader( /* CDROM_TABLE *cdt, int starting_track, int ending_track, int output_fd */ );
#endif
#endif /* HAVE_CDDA */

#ifdef linux
#ifdef __STDC__
static void linux_cdrom_reader(CDROM_TABLE * cdt, int starting_track, int ending_track, int output_fd);
#else
static void linux_cdrom_reader( /* CDROM_TABLE *cdt, int starting_track, int ending_track, int output_fd */ );
#endif
#endif /* linux */

/* starting_track and ending_track can be 0 which means first and last */
#ifdef __STDC__
void
cdrom_reader(int index, int starting_track, int ending_track, int output_fd)
#else
void
cdrom_reader(index, starting_track, ending_track, output_fd)
    int index;
    int starting_track;
    int ending_track;
    int output_fd;
#endif
{
    CDROM_TABLE *cdt;

    cdt = &cdrom_table[index];

    report(REPORT_DEBUG, "cdrom_reader: %s%s tracks %d..%d\n",
	   cdt->name, cdt->device, starting_track, ending_track);

    rplay_audio_close();	/* prevent device busy problems */

#ifdef HAVE_CDDA
    cdda_cdrom_reader(cdt, starting_track, ending_track, output_fd);
#else
#ifdef linux
    linux_cdrom_reader(cdt, starting_track, ending_track, output_fd);
#endif /* linux */
#endif /* HAVE_CDDA */

    /* Add more cdrom readers here. */

    exit(0);
}


/******************************************************************************/

#ifdef HAVE_CDDA

/* A nicer interface to reading from the CDDA interface using the
   CDROM_INFO structure and the cdda_open, cdda_read, and cdda_close
   routines. */

#define CDDA_BLOCK_SIZE 2368
#define CDDA_BLOCK_IGNORE 16

static int cdda_open(CDROM_INFO * info);
static int cdda_read(CDROM_INFO * info);
static int cdda_close(CDROM_INFO * info);
static int internal_cdda_init(CDROM_INFO * info);
static int internal_cdda_read(CDROM_INFO * info);
static int internal_cdda_toc(CDROM_INFO * info);

#ifdef __STDC__
static void
cdda_cdrom_reader(CDROM_TABLE * cdt, int starting_track, int ending_track, int output_fd)
#else
static void
cdda_cdrom_reader(cdt, starting_track, ending_track, output_fd)
    CDROM_TABLE *cdt;
    int starting_track;
    int ending_track;
    int output_fd;
#endif
{
    CDROM_INFO info;
    int n, i;
    char *p;

    info.device = cdt->device;
    info.starting_track = starting_track;
    info.ending_track = ending_track;
    info.num_blocks = 30;
    info.fd = -1;
    info.buffer = NULL;

    if (cdda_open(&info) < 0)
    {
	return;
    }

    for (;;)
    {
	n = cdda_read(&info);
	if (n <= 0)
	{
	    break;
	}

	p = info.buffer;
	for (i = 0; i < info.num_blocks; i++)
	{
	    write(output_fd, p, CDDA_BLOCK_SIZE - CDDA_BLOCK_IGNORE);
	    p += CDDA_BLOCK_SIZE;
	}
    }

    cdda_close(&info);
}

#ifdef __STDC__
static int
cdda_open(CDROM_INFO * info)
#else
static int
cdda_open(info)
    CDROM_INFO *info;
#endif
{
    if (info->fd < 0)
    {
	if (internal_cdda_init(info) < 0)
	{
	    return -1;
	}
    }

    return internal_cdda_toc(info);
}

#ifdef __STDC__
static int
cdda_read(CDROM_INFO * info)
#else
static int
cdda_read(info)
    CDROM_INFO *info;
#endif
{
    int n;

    n = internal_cdda_read(info);
    if (n < 0)
    {
	return -1;
    }

    return n;
}

#ifdef __STDC__
static int
cdda_close(CDROM_INFO * info)
#else
static int
cdda_close(info)
    CDROM_INFO *info;
#endif
{
    close(info->fd);
    info->fd = -1;
    if (info->buffer)
    {
	free((char *) info->buffer);
    }
    info->buffer = NULL;
    return 0;
}

/* End of nicer CDDA interface. */

#include <sys/cdio.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/scsi/impl/uscsi.h>
#include <sys/time.h>
#include <ustat.h>

#define CDDABLKSIZE 2368
#define SAMPLES_PER_BLK 588

#ifdef __STDC__
static int
internal_cdda_init(CDROM_INFO * info)
#else
static int
internal_cdda_init(info)
    CDROM_INFO *info;
#endif
{
    struct cdrom_cdda cdda;

    info->fd = open(info->device, 0);
    if (info->fd < 0)
    {
	perror(info->device);
	return -1;
    }

    info->buffer = (char *) malloc(info->num_blocks * CDDABLKSIZE + CDDABLKSIZE);
    if (info->buffer == NULL)
    {
	fprintf(stderr, "malloc: out of memory\n");
	return -1;
    }

    info->buffer_length = info->num_blocks * CDDABLKSIZE;

    cdda.cdda_addr = 200;
    cdda.cdda_length = 1;
    cdda.cdda_data = info->buffer;
    cdda.cdda_subcode = CDROM_DA_SUBQ;

    if (ioctl(info->fd, CDROMCDDA, &cdda) < 0)
    {
	perror("ioctl: CDROMCDDA");
	free((char *) info->buffer);
	info->buffer = NULL;
	close(info->fd);
	return -1;
    }

    return 0;
}

#ifdef __STDC__
static int
internal_cdda_read(CDROM_INFO * info)
#else
static int
internal_cdda_read(info)
    CDROM_INFO *info;
#endif
{
    struct cdrom_cdda cdda;
    int blk;
    unsigned char *q;

    if (info->current_block >= info->ending_block)
    {
	return 0;
    }

    cdda.cdda_addr = info->current_block;
    if (info->current_block + info->num_blocks > info->ending_block)
    {
	cdda.cdda_length = info->ending_block - info->current_block;
    }
    else
    {
	cdda.cdda_length = info->num_blocks;
    }

    cdda.cdda_data = info->buffer;
    cdda.cdda_subcode = CDROM_DA_SUBQ;

    if (ioctl(info->fd, CDROMCDDA, &cdda) < 0)
    {
	if (errno == ENXIO)
	{
	    perror("ioctl: CDROMCDDA: CD ejected");
	    return -1;
	}

	if (info->current_block + info->num_blocks > info->ending_block)
	{
	    return 0;
	}

	if (ioctl(info->fd, CDROMCDDA, &cdda) < 0)
	{
	    if (ioctl(info->fd, CDROMCDDA, &cdda) < 0)
	    {
		if (ioctl(info->fd, CDROMCDDA, &cdda) < 0)
		{
		    perror("ioctl: CDROMCDDA");
		    return -1;
		}
	    }
	}
    }

    info->current_block += cdda.cdda_length;

    return cdda.cdda_length * CDDABLKSIZE;
}

#ifdef __STDC__
static int
internal_cdda_toc(CDROM_INFO * info)
#else
static int
internal_cdda_toc(info)
    CDROM_INFO *info;
#endif
{
    struct cdrom_tochdr hdr;
    struct cdrom_tocentry entry;
    int i;

    if (ioctl(info->fd, CDROMREADTOCHDR, &hdr) < 0)
    {
	perror("ioctl: CDROMREADTOCHDR");
	return -1;
    }

    info->starting_block = 0;
    info->ending_block = 0;

    if (info->starting_track == 0)
    {
	info->starting_track = hdr.cdth_trk0;
    }
    if (info->ending_track == 0)
    {
	info->ending_track = hdr.cdth_trk1;
    }

    for (i = 1; i <= hdr.cdth_trk1; i++)
    {
	entry.cdte_track = i;
	entry.cdte_format = CDROM_MSF;

	if (ioctl(info->fd, CDROMREADTOCENTRY, &entry) < 0)
	{
	    perror("ioctl: CDROMREADTOCENTRY");
	    return -1;
	}

	if (i == info->starting_track)
	{
	    info->starting_block = entry.cdte_addr.msf.minute * 60 * 75 +
		entry.cdte_addr.msf.second * 75 + entry.cdte_addr.msf.frame;
	}

	if (i == info->ending_track + 1)
	{
	    info->ending_block = entry.cdte_addr.msf.minute * 60 * 75 +
		entry.cdte_addr.msf.second * 75 + entry.cdte_addr.msf.frame;
	}
    }

    entry.cdte_track = CDROM_LEADOUT;
    entry.cdte_format = CDROM_MSF;

    if (ioctl(info->fd, CDROMREADTOCENTRY, &entry) < 0)
    {
	perror("ioctl: CDROMREADTOCENTRY");
	return -1;
    }

    if (info->ending_block == 0)
    {
	info->ending_block = entry.cdte_addr.msf.minute * 60 * 75 +
	    entry.cdte_addr.msf.second * 75 + entry.cdte_addr.msf.frame;
    }

    /* Move back two seconds - don't know why but works */
    info->starting_block -= 150;
    info->ending_block -= 150;

    info->current_block = info->starting_block;

    return 0;
}

#else

/******************************************************************************/

#if defined (linux)

#include <linux/cdrom.h>

#ifdef __STDC__
static int
linux_read_toc(CDROM_INFO * info)
#else
static int
linux_read_toc(info)
    CDROM_INFO *info;
#endif
{
    struct cdrom_tochdr hdr;
    struct cdrom_tocentry entry;
    int i;

    if (ioctl(info->fd, CDROMREADTOCHDR, &hdr) < 0)
    {
	perror("ioctl: CDROMREADTOCHDR");
	return -1;
    }

    info->starting_block = 0;
    info->ending_block = 0;

    if (info->starting_track == 0)
    {
	info->starting_track = hdr.cdth_trk0;
    }
    if (info->ending_track == 0)
    {
	info->ending_track = hdr.cdth_trk1;
    }

    for (i = 1; i <= hdr.cdth_trk1; i++)
    {
	entry.cdte_track = i;
	entry.cdte_format = CDROM_MSF;

	if (ioctl(info->fd, CDROMREADTOCENTRY, &entry) < 0)
	{
	    perror("ioctl: CDROMREADTOCENTRY");
	    return -1;
	}

	if (i == info->starting_track)
	{
	    info->starting_block = entry.cdte_addr.msf.minute * 60 * 75 +
		entry.cdte_addr.msf.second * 75 + entry.cdte_addr.msf.frame;
	}

	if (i == info->ending_track + 1)
	{
	    info->ending_block = entry.cdte_addr.msf.minute * 60 * 75 +
		entry.cdte_addr.msf.second * 75 + entry.cdte_addr.msf.frame;
	}
    }

    entry.cdte_track = CDROM_LEADOUT;
    entry.cdte_format = CDROM_MSF;

    if (ioctl(info->fd, CDROMREADTOCENTRY, &entry) < 0)
    {
	perror("ioctl: CDROMREADTOCENTRY");
	return -1;
    }

    if (info->ending_block == 0)
    {
	info->ending_block = entry.cdte_addr.msf.minute * 60 * 75 +
	    entry.cdte_addr.msf.second * 75 + entry.cdte_addr.msf.frame;
    }

    /* Move back two seconds - don't know why but works */
    info->starting_block -= 150;
    info->ending_block -= 150;

    info->current_block = info->starting_block;

    return 0;
}

#ifdef __STDC__
static void
linux_cdrom_reader(CDROM_TABLE * cdt, int starting_track, int ending_track, int output_fd)
#else
static void
linux_cdrom_reader(cdt, starting_track, ending_track, output_fd)
    CDROM_TABLE *cdt;
    int starting_track;
    int ending_track;
    int output_fd;
#endif
{
    CDROM_INFO info;
    int i;

    info.device = cdt->device;
    info.starting_track = starting_track;
    info.ending_track = ending_track;
    info.num_blocks = 30;
    info.fd = -1;
    info.buffer = NULL;

    info.fd = open(info.device, 0);
    if (info.fd < 0)
    {
	perror(info.device);
	return;
    }

    info.buffer = (char *) malloc(info.num_blocks * CD_FRAMESIZE_RAW + CD_FRAMESIZE_RAW);
    if (info.buffer == NULL)
    {
	fprintf(stderr, "malloc: out of memory\n");
	return;
    }

    linux_read_toc(&info);

/*     printf ("start track=%d block=%d\n", info.starting_track, info.starting_block); */
/*     printf ("end track=%d block=%d\n", info.ending_track, info.ending_block); */

    while (info.current_block < info.ending_block)
    {
	int frames;
	int max_retries = 5;

	if (info.current_block + info.num_blocks > info.ending_block)
	{
	    frames = info.ending_block - info.current_block;
	}
	else
	{
	    frames = info.num_blocks;
	}

	for (i = 0; i < max_retries; i++)
	{
	    struct cdrom_read_audio audio;

	    audio.addr_format = CDROM_LBA;
	    audio.addr.lba = info.current_block;
	    audio.nframes = frames;
	    audio.buf = info.buffer;

	    if (ioctl(info.fd, CDROMREADAUDIO, &audio) >= 0)
	    {
		break;
	    }
	}
	if (i == max_retries)
	{
	    perror(info.device);
	    break;
	}

	if (write(output_fd, info.buffer, CD_FRAMESIZE_RAW * frames) < 0)
	{
	    break;
	}

	info.current_block += frames;
    }

    free(info.buffer);
    close(info.fd);
}

#endif /* linux */
#endif /* HAVE_CDDA */
#endif /* HAVE_CDROM */
