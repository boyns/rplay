#include <rplay.h>

static void event_callback (int fd, int event, char *line);

main (int argc, char **argv)
{
    char buf[RPTP_MAX_LINE];
    int fd;

    fd = rptp_open (rplay_default_host (), RPTP_PORT, buf, sizeof (buf));
    rptp_async_notify (fd, RPTP_EVENT_LEVEL|RPTP_EVENT_CLOSE, event_callback);
    rptp_main_loop ();

    exit (0);
}

static void 
event_callback (int fd, int event, char *line)
{
    static int i;
    int left, right;
    
    switch (event)
    {
    case RPTP_EVENT_LEVEL:
	left = atoi (rptp_parse (line, "left"));
	right = atoi (rptp_parse (0, "right"));
	printf ("%3d %3d\n", left, right);
	break;

    case RPTP_EVENT_CLOSE:
	exit (0);
    }
}
