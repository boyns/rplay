/* do_rptp.c */

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

#include "rptp.h"
#include "misc.h"

int rptp_fd = -1;
char *rptp_host = 0;

static void
callback (fd, action)
    int fd;
    int action;
{
    switch (action)
    {
    case RPTP_ASYNC_ENABLE:
	enable_notify_write (fd);
	break;
	
    case RPTP_ASYNC_DISABLE:
	disable_notify_write (fd);
	break;
    }
}

int
do_rptp_open (host)
    char *host;
{
    char rptp_buf[RPTP_MAX_LINE];
    extern void process_input ();
    
    if (rptp_fd != -1)
    {
        rptp_close (rptp_fd);
    }

    rptp_fd = rptp_open (host, RPTP_PORT, rptp_buf, sizeof (rptp_buf));

    rptp_async_register (rptp_fd, RPTP_ASYNC_WRITE, callback);
#if 0    
    rptp_async_notify (rptp_fd,
		       RPTP_EVENT_PLAY|RPTP_EVENT_PAUSE|RPTP_EVENT_CONTINUE|RPTP_EVENT_DONE
		       |RPTP_EVENT_STATE|RPTP_EVENT_OK|RPTP_EVENT_ERROR|RPTP_EVENT_CLOSE
		       |RPTP_EVENT_OTHER,
		       process_input);
#else
    rptp_async_notify (rptp_fd, RPTP_EVENT_ALL, process_input);
#endif    
    
    if (rptp_host)
    {
        free (rptp_host);
    }
    rptp_host = (char *) strdup (host);

    return 0;
}


int
do_rptp_pause (id, client_data)
    int id;
    char *client_data;
{
    rptp_async_putline (rptp_fd, NULL, "pause id=#%d client-data=\"%d %s\"",
			id, STATE_PAUSE, client_data);

    return 0;
}


int
do_rptp_cont (id, client_data)
    int id;
    char *client_data;
{
    rptp_async_putline (rptp_fd, NULL, "continue id=#%d client-data=\"%d %s\"",
			id, STATE_CONTINUE, client_data);

    return 0;
}


int
do_rptp_stop (id, client_data)
    int id;
    char *client_data;
{
    rptp_async_putline (rptp_fd, NULL, "stop id=#%d client-data=\"%d %s\"",
			id, STATE_STOP, client_data);

    return 0;
}


int
do_rptp_volume (volume)
    int volume;
{
    rptp_async_putline (rptp_fd, NULL, "set volume=%d", volume);

    return 0;
}


int
do_rptp_sounds ()
{
    rptp_async_putline (rptp_fd, NULL, "list sounds");

    return 0;
}


int
do_rptp_play (sound, client_data, args)
    char *sound;
    char *client_data;
    char *args;
{
    if (client_data)
    {
        rptp_async_putline (rptp_fd, NULL, "play client-data=\"%d %s\" %s sound=\"%s\"",
			    STATE_PLAY, client_data,
			    args ? args : "",
			    sound);
    }
    else
    {
        rptp_async_putline (rptp_fd, NULL, "play %s sound=%s",
			    args ? args : "",
			    sound);
    }

    return 0;
}


int
do_rptp_play_info (info, client_data)
    INFO *info;
    char *client_data;
{
    char buf[RPTP_MAX_LINE];

    buf[0] = '\0';
    
    if (info->sample_rate != RPLAY_DEFAULT_SAMPLE_RATE)
    {
	sprintf (buf + strlen (buf), "sample-rate=%d ", info->sample_rate);
    }

    if (info->volume != RPLAY_DEFAULT_VOLUME)
    {
	sprintf (buf + strlen (buf), "volume=%d ", info->volume);
    }

    return do_rptp_play (info->filename, client_data, buf);
}
