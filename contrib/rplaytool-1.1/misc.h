/* $Id: misc.h,v 1.2 1998/08/13 06:13:20 boyns Exp $ */

/* 
 * Copyright (C) 1995 Mark Boyns <boyns@sdsu.edu>
 *
 * This file is part of rplaytool.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _misc_h
#define _misc_h

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>

#define STATE_NONE     0
#define STATE_PLAY     1
#define STATE_STOP     2
#define STATE_PAUSE    3
#define STATE_CONTINUE 4

/* Icons */
typedef struct
{
    Server_image image;
    unsigned short *bits;
}
SOME_ICON;
#define ICON_PLAY      0
#define ICON_PLAY1     1
#define ICON_PLAY2     2
#define ICON_PLAY3     3
#define ICON_PLAY4     4
#define ICON_PAUSE     5
#define ICON_PAUSE1    6
#define ICON_PAUSE2    7
#define ICON_PAUSE3    8
#define ICON_PAUSE4    9
#define ICON_DIRECTORY 10
#define ICON_SOUND     11
#define ICON_ARROW     12
#define ICON_NONE      13
#define ICON_FILE      14
#define ICON_WATCH     15
#define NUMBER_OF_ICONS 16

typedef struct
{
    char filename[MAXPATHLEN];
    char name[MAXPATHLEN];
    int volume;
    int sample_rate;
    int delay;
}
INFO;

typedef struct
{
    int id;
    int size;
    int curr;
    Xv_opaque list;
    int auto_play;
    time_t mtime;
    char filename[MAXPATHLEN];
    int delay;
}
PLAYLIST;

#define NUMBER_OF_PLAYLISTS 4

extern PLAYLIST playlist[NUMBER_OF_PLAYLISTS];
extern int curr_playlist;
extern SOME_ICON icons[NUMBER_OF_ICONS];
extern int client_id;

#ifdef __STDC__
extern int notice (Xv_opaque panel, char *button1, char *button2, char *fmt, ...);
#else
extern int notice (/* Xv_opaque frame, char *button1, char *button2, char *fmt, ... */ );
#endif
extern void sort_list (/* Xv_opaque list */);
extern void busy (/* Xv_opaque frame, int flag */);
extern void do_playlist_init (/* int list */);
extern void do_playlist_reset (/* int list */);
extern void do_playlist_add (/* list, index, sound_dir, sound_name */);
extern void do_playlist_append (/* list, sound_dir, sound_name */);
extern void do_playlist_move (/* list, from, to */);
extern void do_playlist_delete_all (/* int list */);
extern void do_playlist_delete (/* int list, int index */);
extern void do_playlist_stop (/* int list */);
extern void playlist_scan ();
extern void do_playlist_load (/* int list, FILE *fp */);
extern void do_playlist_save (/* int list, FILE *fp */);
extern void play_playlist (/* int list */);
extern void playlist_update_icons (/* int list */);

#endif /* _misc_h */
