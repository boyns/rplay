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

#define NBOXES 8

typedef struct
{
    FL_OBJECT *obj;
    int level;
    FL_PD_COL color;
} BOX;

FL_FORM *form;
FL_OBJECT *w, *input;

BOX left[] = 
{
    { 0, 0, FL_GREEN },
    { 0, 32, FL_GREEN },
    { 0, 64, FL_GREEN },
    { 0, 96, FL_GREEN },
    { 0, 128, FL_YELLOW },
    { 0, 160, FL_YELLOW },
    { 0, 192, FL_RED },
    { 0, 224, FL_RED }
};
BOX right[] =
{
    { 0, 0, FL_GREEN },
    { 0, 32, FL_GREEN },
    { 0, 64, FL_GREEN },
    { 0, 96, FL_GREEN },
    { 0, 128, FL_YELLOW },
    { 0, 160, FL_YELLOW },
    { 0, 192, FL_RED },
    { 0, 224, FL_RED }
};

int stereo = 1;
float scale = 1.0;

void do_form ();
void do_rptp ();
void event_callback (int fd, int event, char *line);

main (int argc, char **argv)
{
#if 1
    fl_flip_yorigin ();
    fl_initialize (&argc, argv, "VU", 0, 0);
#else
    fl_initialize (argv[0], "VU", 0, 0, &argc, argv);
#endif
    do_form ();
    do_rptp ();
    rptp_main_loop ();
}

void
do_form ()
{
    int i, width, x, y;

    if (stereo)
    {
	width = 200;
    }
    else
    {
	width = 100;
    }
    
    form = fl_bgn_form (FL_NO_BOX, width, 260);
    w = fl_add_box (FL_UP_BOX, 0, 0, width, 260, "");
    input = fl_add_input (FL_HIDDEN_INPUT, 0, 0, width, 260, "");
    fl_set_input_return (input, FL_RETURN_ALWAYS);
    fl_set_input (input, "");
    
    x = 10;
    y = 10;
    for (i = 0; i < NBOXES; i++, y += 30)
    {
	left[i].obj = fl_add_box (FL_DOWN_BOX, x, y, 80, 30, "");
	if (stereo)
	{
	    right[i].obj = fl_add_box (FL_DOWN_BOX, 100+x, y, 80, 30, "");
	}
    }

    fl_end_form ();
    fl_show_form (form, FL_FREE_SIZE, FL_FULLBORDER, "VU");
    fl_check_forms ();
}

void
do_rptp ()
{
    char buf[RPTP_MAX_LINE];
    int fd;

    fd = rptp_open (rplay_default_host (), RPTP_PORT, buf, sizeof (buf));
    rptp_async_notify (fd, RPTP_EVENT_LEVEL|RPTP_EVENT_CLOSE, event_callback);
}

void 
event_callback (int fd, int event, char *line)
{
    FL_OBJECT *obj;
    int left_level, right_level, i;

    switch (event)
    {
    case RPTP_EVENT_LEVEL:
	left_level = atoi (rptp_parse (line, "left"));
	right_level = atoi (rptp_parse (0, "right"));
	
	for (i = 0; i < NBOXES; i++)
	{
	    if (left_level > left[i].level)
	    {
		fl_set_object_color (left[i].obj, left[i].color, FL_COL1);
	    }
	    else
	    {
		fl_set_object_color (left[i].obj, FL_COL1, FL_COL1);
	    }
	    if (stereo)
	    {
		if (right_level > right[i].level)
		{
		    fl_set_object_color (right[i].obj, right[i].color, FL_COL1);
		}
		else
		{
		    fl_set_object_color (right[i].obj, FL_COL1, FL_COL1);
		}
	    }
	}
	break;

    case RPTP_EVENT_CLOSE:
	exit (1);
    }

    obj = fl_check_forms ();
    if (obj == input)
    {
	float new_scale = scale;
	char *key = fl_get_input (obj);
	if (*key == '<')
	{
	    new_scale *= 0.8;
	}
	else if (*key == '>')
	{
	    new_scale *= (1/0.8);
	}
	if (new_scale != scale)
	{
	    fl_scale_form (form, new_scale/scale, new_scale/scale);
	    scale = new_scale;
	}
	fl_set_input (input, "");
    }
}

    
