/* cdrom.h - rplay cdrom interface */

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



#ifndef _cdrom_h
#define _cdrom_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_CDROM

/* The maximum number of CDROM devices.  Also see cdrom.c.
   This is really the size of cdrom_table. */
#define MAX_CDROMS 5

typedef struct
{
    char *name;
    char *device;
    int number_of_tracks;
    int current_track;
}
CDROM_TABLE;

extern CDROM_TABLE cdrom_table[];

#ifdef __STDC_
extern void cdrom_reader (int index, int starting_track, int ending_track, int output_fd);
#else
extern void cdrom_reader (/* int index, int starting_track, int ending_track, int output_fd */);
#endif

#endif /* HAVE_CDROM */

#endif /* _cdrom_h */
