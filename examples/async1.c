/* $Id: async1.c,v 1.3 1998/11/07 21:15:11 boyns Exp $ */

#include <rplay.h>

static void event_callback(int fd, int event, char *line);

main(int argc, char **argv)
{
    char buf[RPTP_MAX_LINE];
    int fd;

    fd = rptp_open(rplay_default_host(), RPTP_PORT, buf, sizeof(buf));

    rptp_async_notify(fd,
		      RPTP_EVENT_PLAY | RPTP_EVENT_PAUSE | RPTP_EVENT_DONE | RPTP_EVENT_CONTINUE,
		      event_callback);

    rptp_main_loop();

    exit(0);
}

static void
event_callback(int fd, int event, char *line)
{
    switch (event)
    {
    case RPTP_EVENT_PLAY:
	printf("%s is playing\n", rptp_parse(line, "sound"));
	break;

    case RPTP_EVENT_PAUSE:
	printf("%s is paused\n", rptp_parse(line, "sound"));
	break;

    case RPTP_EVENT_DONE:
	printf("%s is done\n", rptp_parse(line, "sound"));
	break;

    case RPTP_EVENT_CONTINUE:
	printf("%s continue\n", rptp_parse(line, "sound"));
	break;
    }
}
