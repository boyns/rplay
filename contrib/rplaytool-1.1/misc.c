/* $Id: misc.c,v 1.2 1998/08/13 06:13:19 boyns Exp $ */

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

#include "misc.h"
#include <sys/stat.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <xview/scrollbar.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "rplaytool_ui.h"
#include "rptp.h"

#undef MIN
#undef MAX

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

/* Windows */
extern rplaytool_w_objects *Rplaytool_w;
extern rplaytool_s_objects *Rplaytool_s;
extern rplaytool_f_objects *Rplaytool_f;


PLAYLIST playlist[NUMBER_OF_PLAYLISTS];
int curr_playlist = 0;
int client_id = 0;
extern int initial_delay;

static unsigned short directory_bits[] = 
{
#include "icons/directory.icon"
};
static unsigned short sound_bits[] = 
{
#include "icons/sound.icon"
};
static unsigned short play_bits[] = 
{
#include "icons/play.icon"
};
static unsigned short play1_bits[] = 
{
#include "icons/play1.icon"
};
static unsigned short play2_bits[] = 
{
#include "icons/play2.icon"
};
static unsigned short play3_bits[] = 
{
#include "icons/play3.icon"
};
static unsigned short play4_bits[] = 
{
#include "icons/play4.icon"
};
static unsigned short pause_bits[] = 
{
#include "icons/pause.icon"
};
static unsigned short pause1_bits[] = 
{
#include "icons/pause1.icon"
};
static unsigned short pause2_bits[] = 
{
#include "icons/pause2.icon"
};
static unsigned short pause3_bits[] = 
{
#include "icons/pause3.icon"
};
static unsigned short pause4_bits[] = 
{
#include "icons/pause4.icon"
};
static unsigned short arrow_bits[] = 
{
#include "icons/arrow.icon"
};
static unsigned short none_bits[] = 
{
#include "icons/none.icon"
};
static unsigned short file_bits[] = 
{
#include "icons/file.icon"
};
static unsigned short watch_bits[] = 
{
#include "icons/watch.icon"
};

SOME_ICON icons[NUMBER_OF_ICONS] =
{
    { NULL, play_bits },
    { NULL, play1_bits },
    { NULL, play2_bits },
    { NULL, play3_bits },
    { NULL, play4_bits },
    { NULL, pause_bits },
    { NULL, pause1_bits },
    { NULL, pause2_bits },
    { NULL, pause3_bits },
    { NULL, pause4_bits },
    { NULL, directory_bits },
    { NULL, sound_bits },
    { NULL, arrow_bits },
    { NULL, none_bits },
    { NULL, file_bits },
    { NULL, watch_bits },
};

#ifdef __STDC__
int
notice (Xv_opaque panel, char *button1, char *button2, char *fmt, ...)
#else
    int
notice (va_alist)
    va_dcl
#endif
{
    char message[1024];
    Xv_notice nw;
    int choice = 0;
    va_list args;

#ifdef __STDC__
    va_start (args, fmt);
#else
    char *fmt, *button1, *button2;
    Xv_opaque panel;
    va_start (args);
    panel = va_arg (args, Xv_opaque);
    button1 = va_arg (args, char *);
    button2 = va_arg (args, char *);
    fmt = va_arg (args, char *);
#endif

    vsprintf (message, fmt, args);

    if (button1 && button2)
    {
	nw = xv_create (panel, NOTICE,
			NOTICE_MESSAGE_STRINGS, message, NULL,
			NOTICE_BUTTON, button1, 1,
			NOTICE_BUTTON, button2, 2,
			NOTICE_STATUS, &choice,
			XV_SHOW, TRUE,
			NULL);
    }
    else if (button1)
    {
	nw = xv_create (panel, NOTICE,
			NOTICE_MESSAGE_STRINGS, message, NULL,
			NOTICE_BUTTON, button1, 1,
			NOTICE_STATUS, &choice,
			XV_SHOW, TRUE,
			NULL);
    }
    else
    {
	nw = xv_create (panel, NOTICE,
			NOTICE_MESSAGE_STRINGS, message, NULL,
			XV_SHOW, TRUE,
			NULL);
    }

    xv_destroy_safe (nw);

    return choice;
}

void
sort_list (list)
    Xv_opaque list;
{
    xv_set(list, PANEL_INACTIVE, TRUE, NULL);
    xv_set(list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
    xv_set(list, PANEL_INACTIVE, FALSE, NULL);
}

void
busy (frame, flag)
     Xv_opaque frame;
     int flag;
{
    xv_set (frame, FRAME_BUSY, flag, NULL);
}

static INFO *
new_info ()
{
    INFO *info = (INFO *) malloc (sizeof (INFO));

    info->filename[0] = '\0';
    info->name[0] = '\0';
    info->volume = RPLAY_DEFAULT_VOLUME;
    info->sample_rate = RPLAY_DEFAULT_SAMPLE_RATE;
    info->delay = initial_delay; /* -1 or seconds */

    return info;
}

/* Initialize a playlist. */
void
do_playlist_init (list)
    int list;
{
    playlist[list].id = -1;
    playlist[list].size = 0;
    playlist[list].curr = -1;
    playlist[list].auto_play = 1;
    playlist[list].filename[0] = '\0';
    playlist[list].mtime = 0;
    playlist[list].delay = -1;
}

/* Reset a playlist. */
void
do_playlist_reset (list)
    int list;
{
    playlist[list].id = -1;
    playlist[list].size = 0;
    playlist[list].curr = -1;
    playlist[list].auto_play = 1;
    /* playlist[list].filename[0] = '\0'; */
    /* playlist[list].mtime = 0; */
    /* playlist[list].delay = -1; */
}

void
playlist_update_icons (list)
    int list;
{
    int i;

    xv_set (playlist[list].list, XV_SHOW, FALSE, NULL);
    
    for (i = 0; i < playlist[list].size; i++)
    {
	xv_set (playlist[list].list,
		PANEL_LIST_GLYPH, i, icons[ICON_NONE].image, NULL);
    }

    /* Delaying */
    if (playlist[list].delay > 0)
    {
	xv_set (playlist[list].list,
		PANEL_LIST_GLYPH, playlist[list].curr, icons[ICON_WATCH].image,
		NULL);
    }
    /* Playing */
    else if (playlist[list].curr >= 0 && playlist[list].id > 0)
    {
	xv_set (playlist[list].list,
		PANEL_LIST_GLYPH, playlist[list].curr, icons[ICON_SOUND].image, NULL);
    }
    /* Not playing */
    else if (playlist[list].curr >= 0 && playlist[list].curr < playlist[list].size)
    {
	xv_set (playlist[list].list,
		PANEL_LIST_GLYPH, playlist[list].curr, icons[ICON_ARROW].image, NULL);
    }
    else
    {
	xv_set (playlist[list].list,
		PANEL_LIST_GLYPH, 0, icons[ICON_ARROW].image, NULL);
    }

    xv_set (playlist[list].list, XV_SHOW, TRUE, NULL);
}


/* Add a sound to list at index. */
void
do_playlist_add (list, index, filename)
    int list;
    int index;
    char *filename;
{
    INFO *info;
    char *p;

    info = new_info ();
    strcpy (info->filename, filename);
    p = strrchr (info->filename, '/');
    if (p)
    {
	strcpy (info->name, p+1);
    }
    else
    {
	strcpy (info->name, info->filename);
    }

    xv_set (playlist[list].list,
	    PANEL_LIST_INSERT, index,
	    PANEL_LIST_STRING, index, info->name,
	    PANEL_LIST_CLIENT_DATA, index, info,
	    NULL);
    
    playlist[list].size++;
}

/* Append a sound to list .*/
void
do_playlist_append (list, filename)
    int list;
    char *filename;
{
    do_playlist_add (list, playlist[list].size, filename);
    playlist_update_icons (list);
}

void
do_playlist_move (list, from, to)
    int list;
    int from;
    int to;
{
    char from_name[RPTP_MAX_LINE], *from_data;
    char to_name[RPTP_MAX_LINE], *to_data;

    xv_set (playlist[list].list, XV_SHOW, FALSE, NULL);

    strcpy (from_name, (char *) xv_get (playlist[list].list, PANEL_LIST_STRING, from));
    from_data = (char *) xv_get (playlist[list].list, PANEL_LIST_CLIENT_DATA, from);

    strcpy (to_name, (char *) xv_get (playlist[list].list, PANEL_LIST_STRING, to));
    to_data = (char *) xv_get (playlist[list].list, PANEL_LIST_CLIENT_DATA, to);

    xv_set (playlist[list].list,
	    PANEL_LIST_STRING, from, to_name,
	    PANEL_LIST_CLIENT_DATA, from, to_data,
	    NULL);

    xv_set (playlist[list].list,
	    PANEL_LIST_STRING, to, from_name,
	    PANEL_LIST_CLIENT_DATA, to, from_data,
	    NULL);

    playlist_update_icons (list);
    
    xv_set (playlist[list].list, XV_SHOW, TRUE, NULL);
}
		 
/* Delete all entries from a playlist. */
void
do_playlist_delete_all (list)
    int list;
{
    int i, n;

    n = (int) xv_get (playlist[list].list, PANEL_LIST_NROWS);
    
    for (i = 0; i < n; i++)
    {
	do_playlist_delete (list, 0);
    }

    do_playlist_reset (list);
}

/* Delete a single entry from a playlist. */
void
do_playlist_delete (list, index)
    int list;
    int index;
{
    char *p;

    p = (char *) xv_get (playlist[list].list, PANEL_LIST_CLIENT_DATA, index);
    if (p)
    {
	free ((char *) p);
    }
    
    xv_set (playlist[list].list, PANEL_LIST_DELETE, index, NULL);

    /* Update the size. */
    playlist[curr_playlist].size--;

    /* Stop it if it's playing. */
    if (playlist[list].curr != -1 && index == playlist[list].curr)
    {
	do_playlist_stop (list);
	playlist[list].curr--;
    }
    /* Update curr. */
    else if (playlist[list].curr != -1 && index < playlist[list].curr)
    {
	playlist[list].curr--;
    }
}

/* Stop the sound a playlist is playing. */
void
do_playlist_stop (list)
    int list;
{
    if (playlist[list].curr != -1 && playlist[list].id > 0)
    {
	char client_data[RPTP_MAX_LINE];

	sprintf (client_data, "%d %d", client_id, list);
	do_rptp_stop (playlist[list].id, client_data);

	playlist[curr_playlist].id = -1;
    }

    if (playlist[curr_playlist].delay > 0)
    {
	playlist[curr_playlist].delay = -1;
    }

    playlist_update_icons (list);
}

void
do_playlist_load (list, fp)
    int list;
    FILE *fp;
{
    char buf[BUFSIZ];
    char *value;
    INFO *info;
    struct stat st;
    
    while (fgets (buf, sizeof (buf), fp))
    {
        value = rptp_parse (buf, 0);
        do_playlist_append (list, value);

	info = (INFO *) xv_get (playlist[list].list, PANEL_LIST_CLIENT_DATA, playlist[list].size - 1);

	value = rptp_parse (0, "volume");
	if (value && *value)
	{
	    info->volume = atoi (value);
	}

	value = rptp_parse (0, "sample-rate");
	if (value && *value)
	{
	    info->sample_rate = atoi (value);
	}

	value = rptp_parse (0, "name");
	if (value && *value)
	{
	    strcpy (info->name, value);
	    xv_set (playlist[list].list,
	            PANEL_LIST_STRING, playlist[list].size - 1, info->name,
	            NULL);
        }

	value = rptp_parse (0, "delay");
	if (value && *value)
	{
	    info->delay = atoi (value);
	}
    }

    if (fstat (fileno (fp), &st) > 0)
    {
	playlist[list].mtime = st.st_mtime;
    }

    playlist_update_icons (list);
}

void
do_playlist_save (list, fp)
    int list;
    FILE *fp;
{
    INFO *info;
    int i;

    for (i = 0; i < playlist[list].size; i++)
    {
	info = (INFO *) xv_get (playlist[list].list, PANEL_LIST_CLIENT_DATA, i);

	fprintf (fp, "%s --name=\"%s\"", info->filename, info->name);

	if (info->volume != RPLAY_DEFAULT_VOLUME)
	{
	    fprintf (fp, " --volume=%d", info->volume);
	}

	if (info->sample_rate != RPLAY_DEFAULT_SAMPLE_RATE)
	{
	    fprintf (fp, " --sample-rate=%d", info->sample_rate);
	}

	if (info->delay > 0)
	{
	    fprintf (fp, " --delay=%d", info->delay);
	}
	
	fprintf (fp, "\n");
    }
}

/* Scan all the playlist filenames to see if they have been modified.
   If so, update the playlist with the new file contents. */
void
playlist_scan ()
{
    int i, n;
    struct stat st;
    FILE *fp;
    char buf[BUFSIZ];
    char message[1024];
    char filename[MAXPATHLEN];
    INFO *info;
    char was_playing[MAXPATHLEN];
    int was_playing_id;
    int was_playing_index;
    
    for (i = 0; i < NUMBER_OF_PLAYLISTS; i++)
    {
	if (playlist[i].filename[0] != '\0')
	{
	    stat (playlist[i].filename, &st);
	    if (st.st_mtime > playlist[i].mtime)
	    {
		busy (Rplaytool_s->s, TRUE);
		
		fp = fopen (playlist[i].filename, "r");
		if (!fp)
		{
		    fprintf (stderr, "Can't open `%s'\n", playlist[i].filename);
		    busy (Rplaytool_s->s, FALSE);
		    continue;
		}

		/* Save the sound that was playing. */
		if (playlist[i].curr != -1)
		{
		    info = (INFO *) xv_get (playlist[i].list, PANEL_LIST_CLIENT_DATA, playlist[i].curr);
		    strcpy (was_playing, info->filename);
		    was_playing_id = playlist[i].id;
		    was_playing_index = playlist[i].curr;
		}
		else
		{
		    was_playing[0] = '\0';
		    was_playing_id = 0;
		    was_playing_index = 0;
		}
		
		/* Delete all sounds from this list. */
		playlist[i].curr = -1; /* hack to prevent the stopping of the playing sound */
		playlist[i].id = -1;
		do_playlist_delete_all (i);

		/* Load the new list. */
		do_playlist_load (i, fp);
		fclose (fp);

		if (was_playing[0])
		{
		    int new_position = -1;
		    
		    /* Try to find the sound that was playing in the new list. */

		    /* First try the original position. */
		    n = (int) xv_get (playlist[i].list, PANEL_LIST_NROWS);
		    if (was_playing_index < n)
		    {
			info = (INFO *) xv_get (playlist[i].list, PANEL_LIST_CLIENT_DATA, was_playing_index);
		    }
		    else
		    {
			info = NULL;
		    }
		    if (info && strcmp (was_playing, info->filename) == 0)
		    {
			new_position = was_playing_index;
		    }
		    /* Find closest match. */
		    else
		    {
			int closest_before = -1;
			int closest_after = -1;
			int match_found = 0;

			for (n = 0; n < playlist[i].size; n++)
			{
			    info = (INFO *) xv_get (playlist[i].list, PANEL_LIST_CLIENT_DATA, n);
			    if (strcmp (was_playing, info->filename) == 0)
			    {
				match_found++;
				
				if (n < was_playing_index)
				{
				    closest_before = n;
				}
				else if (n > was_playing_index)
				{
				    closest_after = n;
				    break;
				}
			    }
			}

#if 0
			printf ("match_found=%d prev=%d next=%d was=%d\n",
				match_found, closest_before, closest_after, was_playing_index);
#endif			
			
			if (match_found)
			{
			    if (closest_before >= 0 && closest_after >= 0)
			    {
				int distance_before;
				int distance_after;

				distance_before = was_playing_index - closest_before;
				distance_after = closest_after - was_playing_index;
			    
				/* If matches are equally distance, use the previous one.
				   Otherwise, use the closest one. */
				new_position = distance_before <= distance_after
				    ? closest_before : closest_after;
			    }
			    else if (closest_before >= 0)
			    {
				new_position = closest_before;
			    }
			    else
			    {
				new_position = closest_after;
			    }
			}
		    }
		    
		    if (new_position == -1)
		    {
			do_rptp_stop (was_playing_id, "");
			playlist[i].curr = 0;
			play_playlist (i, TRUE);
		    }
		    else
		    {
			playlist[i].curr = new_position;
			playlist[i].id = was_playing_id;
		    }
		}
		
		playlist[i].mtime = st.st_mtime;
		playlist_update_icons (i);
		busy (Rplaytool_s->s, FALSE);
	    }
	}
    }
}

void
play_playlist (list, use_delay)
    int list;
    int use_delay;
{
    char client_data[32];
    Scrollbar scrollbar;
    INFO *info;
    
    if ((int) xv_get (playlist[list].list, PANEL_LIST_NROWS) > 0
	&& playlist[list].curr >= 0
	&& playlist[list].curr < playlist[list].size)
    {
	int rows, view;

	info = (INFO *) xv_get (playlist[list].list,
				PANEL_LIST_CLIENT_DATA, playlist[list].curr);

	/* See if the play should be delayed a while. */
	if (use_delay && info->delay > 0)
	{
	    /* It's already being delayed, interrupt it. */
	    if (playlist[list].delay > 0)
	    {
		playlist[list].delay = -1;
	    }
	    else
	    {
		playlist[list].delay = info->delay;
		playlist_update_icons (list);
		return;
	    }
	}
	
	/* Set `client-data' to the client_id and soundlist number. */
	sprintf (client_data, "%d %d", client_id, list);

	/* Play the sound */
	do_rptp_play_info (info, client_data);

	busy (Rplaytool_s->s, TRUE);
	
	/* Auto-scroll */
	rows = (int) xv_get (playlist[list].list, PANEL_LIST_DISPLAY_ROWS);
    	scrollbar = (Scrollbar) xv_get (playlist[list].list, PANEL_LIST_SCROLLBAR);
    	view = (int) xv_get (scrollbar, SCROLLBAR_VIEW_START);
    	if (playlist[list].curr >= view + rows)
    	{
	    xv_set (scrollbar, SCROLLBAR_VIEW_START, playlist[list].curr, NULL);
    	}
	else if (playlist[list].curr < view)
	{
	    view = playlist[list].curr - rows;
	    if (view < 0)
	    {
		view = 0;
	    }
	    xv_set (scrollbar, SCROLLBAR_VIEW_START, view, NULL);
	}
    }
    else if (playlist[list].curr >= playlist[list].size
	     || playlist[list].curr < 0)
    {
	playlist[list].curr = 0; /* rewind */
    }
    
    playlist_update_icons (list);
}
