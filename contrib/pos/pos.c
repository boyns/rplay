/* 
 * Copyright (C) 1995 Mark Boyns <boyns@sdsu.edu>
 *
 * This file is part of rplay.
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

#include <X11/forms.h>
#include <rplay.h>

#define TITLE "<silence>"

FL_FORM *form;
FL_OBJECT *window, *slider;

typedef struct 
{
    int id;
    char name[1024];
    double position;
    double seconds;
} 
SOUND;

#define MAX_SOUNDS 12
SOUND sounds[MAX_SOUNDS];

#define SOUND_INIT(i)\
    sounds[i].id = 0; \
    sounds[i].name[0] = '\0'; \
    sounds[i].position = 0; \
    sounds[i].seconds = 0;

double max_seconds = 0.0;

void do_form ();
void do_rptp ();
void event_callback (int fd, int event, char *line);
void set_title (char *title);

main (int argc, char **argv)
{
#if 1
    fl_flip_yorigin ();
    fl_initialize (&argc, argv, "POS", 0, 0);
#else    
    fl_initialize (argv[0], "??", 0, 0, &argc, argv);
#endif    
    do_form ();
    do_rptp ();
    rptp_main_loop ();
}

void
do_form ()
{
    int i;

#define W 300
#define H 50
    
    form = fl_bgn_form (FL_NO_BOX, W, H);
    window = fl_add_box (FL_UP_BOX, 0, 0, W, H, "");
    slider = fl_add_valslider (FL_HOR_FILL_SLIDER, 0, 0, W, H, "");
    /* fl_set_object_align (slider, FL_ALIGN_LEFT); */
    fl_set_object_color (slider, FL_COL1, FL_GREEN);
    fl_set_slider_bounds (slider, 0.0, 1.0);
    fl_set_slider_value (slider, 0.0);
    
    for (i = 0; i < MAX_SOUNDS; i++)
    {
	SOUND_INIT(i);
    }
    
    fl_end_form ();
    fl_show_form (form, FL_FREE_SIZE, FL_FULLBORDER, TITLE);
    fl_check_forms ();
}

void
do_rptp ()
{
    char buf[RPTP_MAX_LINE];
    int fd;

    fd = rptp_open (rplay_default_host (), RPTP_PORT, buf, sizeof (buf));
    rptp_async_notify (fd, RPTP_EVENT_POSITION|RPTP_EVENT_PLAY|RPTP_EVENT_DONE|RPTP_EVENT_CLOSE, event_callback);
    rptp_async_putline (fd, NULL, "set notify-rate=1.0");
}

void 
event_callback (int fd, int event, char *line)
{
    FL_OBJECT *obj;
    double position, seconds;
    char *sec;
    char buf[RPTP_MAX_LINE];
    int i, j, id, max_index;
    
    rptp_parse (line, 0);
    id = atoi (1 + rptp_parse (0, "id"));

    switch (event)
    {
    case RPTP_EVENT_PLAY:
        for (i = 0; i < MAX_SOUNDS; i++)
        {
            if (sounds[i].id == 0)
            {
		sscanf (rptp_parse (0, "seconds"), "%lf", &seconds);
            	sounds[i].id = id;
            	sounds[i].seconds = seconds;
            	strcpy (sounds[i].name, rptp_parse (0, "sound"));

	        if (seconds > max_seconds)
	        {
	            max_seconds = seconds;
		    fl_set_slider_bounds (slider, 0.0, max_seconds);
		    fl_set_slider_value (slider, 0.0);
		    set_title (sounds[i].name);
	        }
            	break;
            }
        }
        break;

    case RPTP_EVENT_DONE:
        for (i = 0; i < MAX_SOUNDS; i++)
        {
            if (sounds[i].id == id && sounds[i].seconds == max_seconds)
	    {
		max_seconds = 0.0;
		max_index = -1;
		for (j = 0; j < MAX_SOUNDS; j++)
		{
		    if (j == i)
		    {
			continue;
		    }
		    if (sounds[j].seconds > max_seconds)
		    {
			max_seconds = sounds[j].seconds;
			max_index = j;
		    }
		}
		if (max_index == -1)
		{
		    fl_set_slider_bounds (slider, 0.0, 1.0);
		    fl_set_slider_value (slider, 0.0);
		    set_title (TITLE);
		}
		else
		{
		    fl_set_slider_bounds (slider, 0.0, sounds[max_index].seconds);
		    fl_set_slider_value (slider, sounds[max_index].position);
		    set_title (sounds[max_index].name);
		}
	        SOUND_INIT(i);
            	break;
            }
        }
        break;

    case RPTP_EVENT_POSITION:
        for (i = 0; i < MAX_SOUNDS; i++)
        {
            if (sounds[i].id == id && sounds[i].seconds == max_seconds)
            {
		sscanf (rptp_parse (0, "position"), "%lf", &position);
	        fl_set_slider_value (slider, position);
	        break;
	    }
        }
	break;

    case RPTP_EVENT_CLOSE:
	exit (1);
    }

    obj = fl_check_forms ();
}

void
set_title (char *title)
{
    XTextProperty   windowName;

    XStringListToTextProperty(&title, 1, &windowName);
    XSetWMName(fl_display, form->window, &windowName);
    XFlush (fl_display);
}
