/* async2.c - A *very* simple RPTP client using async. */

#include <rplay.h>
#include <stdio.h>

static void process_input (int fd);
static void event_callback (int fd, int event, char *line);

static int rptp_fd;

main (int argc, char **argv)
{
    char buf[RPTP_MAX_LINE];

    rptp_fd = rptp_open (rplay_default_host (), RPTP_PORT, buf, sizeof (buf));

    rptp_async_register (0, RPTP_ASYNC_READ, process_input);
    rptp_async_notify (rptp_fd, RPTP_EVENT_ALL, event_callback);

    rptp_main_loop ();

    exit (0);
}

static void
process_input (int fd)
{
    char buf[BUFSIZ];

    fgets (buf, sizeof (buf), stdin);
    buf[strlen (buf) - 1] = '\0';

    rptp_async_putline (rptp_fd, NULL, buf);
}

static void 
event_callback (int fd, int event, char *line)
{
    rptp_parse (line, 0);

    switch (event)
    {
    case RPTP_EVENT_OK:
        break;

    case RPTP_EVENT_ERROR:
        printf ("Error: %s\n", rptp_parse (0, "error"));
        break;

    case RPTP_EVENT_PLAY:
	printf ("[%s]  Play      %s\n", rptp_parse (0, "id"), rptp_parse (0, "sound"));
	break;

    case RPTP_EVENT_PAUSE:
	printf ("[%s]  Pause     %s\n", rptp_parse (0, "id"), rptp_parse (0, "sound"));
	break;

    case RPTP_EVENT_DONE:
	printf ("[%s]  Done      %s\n", rptp_parse (0, "id"), rptp_parse (0, "sound"));
	break;

    case RPTP_EVENT_CONTINUE:
	printf ("[%s]  Continue  %s\n", rptp_parse (0, "id"), rptp_parse (0, "sound"));
	break;
    }
}
