/*
 * Copyright (C) 2002 Mark R. Boyns <boyns@doit.org>
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
#include "rplay.h"

#include <signal.h>

static void event_callback(int fd, int event, char *line);
static void next_sound();
static void stop_and_exit();

static char **sounds;
static int rptp_fd;
static int current_id = -1;

#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc, argv)
    int argc;
    char **argv;
#endif
{
    char buf[RPTP_MAX_LINE];

    signal(SIGINT, stop_and_exit);
    signal(SIGHUP, stop_and_exit);
    signal(SIGTERM, stop_and_exit);

    rptp_fd = rptp_open(rplay_default_host(), RPTP_PORT, buf, sizeof(buf));
    if (rptp_fd < 0)
    {
        rptp_perror(rplay_default_host());
        exit(1);
    }

    rptp_async_notify(rptp_fd,
                      RPTP_EVENT_OK|RPTP_EVENT_ERROR|
                      RPTP_EVENT_PLAY|RPTP_EVENT_PAUSE|RPTP_EVENT_DONE|RPTP_EVENT_CONTINUE,
                      event_callback);

    sounds = ++argv;
    next_sound();

    rptp_main_loop();
}

static void
#ifdef __STDC__
event_callback(int fd, int event, char *line)
#else
event_callback(fd, event, line)
     int fd;
     int event;
     char *line;
#endif
{
    switch (event)
    {
    case RPTP_EVENT_OK:
        if (rptp_parse(line, "id"))
        {
            current_id = atoi(1 + rptp_parse(0, "id"));
        }
        break;

    case RPTP_EVENT_ERROR:
        printf("error: %s\n", rptp_parse(line, "error"));
        next_sound();
        break;

    case RPTP_EVENT_PLAY:
        break;

    case RPTP_EVENT_PAUSE:
        break;

    case RPTP_EVENT_DONE:
        if (rptp_parse(line, "id"))
        {
            int id;

            id = atoi(1 + rptp_parse(0, "id"));
            if (id == current_id)
            {
                current_id = -1;
                next_sound();
            }
        }
        break;

    case RPTP_EVENT_CONTINUE:
        break;
    }
}

static void next_sound()
{
    char buf[RPTP_MAX_LINE];

    if (*sounds)
    {
        sprintf(buf, "play sound=\"%s\"", *sounds++);
        rptp_async_putline(rptp_fd, NULL, buf);
    }
    else
    {
        rptp_stop_main_loop(0);
    }
}

void stop_and_exit()
{
    char buf[64];
    char response[RPTP_MAX_LINE];

    rptp_stop_main_loop(0);

    if (current_id != -1)
    {
        sprintf(buf, "stop id=\"#%d\"", current_id);
        current_id = -1;
        rptp_putline(rptp_fd, buf);
        rptp_getline(rptp_fd, response, sizeof(response));
    }
}
