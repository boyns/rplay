/* flow1.c - Flow an audio file to rplayd. */

/* usage: flow1 soundfile */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <rplay.h>

main (int argc, char **argv)
{
    FILE *fp;
    int rptp_fd, size, n, nwritten;
    struct stat st;
    char *id;
    char line[RPTP_MAX_LINE];
    char response[RPTP_MAX_LINE];
    char buf[8000];

    /* First determine how big the audio file is. */
    if (stat (argv[1], &st) < 0)
    {
	perror (argv[1]);
	exit (1);
    }
    size = st.st_size;

    fp = fopen (argv[1], "r");
    if (fp == NULL)
    {
	perror (argv[1]);
	exit (1);
    }

    /* Connect to the audio server. */
    rptp_fd = rptp_open (rplay_default_host (), RPTP_PORT, response, sizeof (response));
    if (rptp_fd < 0)
    {
	rptp_perror (rplay_default_host ());
	exit (1);
    }

    /* Start the flow using `input-storage=none'. */
    sprintf (line, "play input=flow input-storage=none sound=%s", argv[1]);
    switch (rptp_command (rptp_fd, line, response, sizeof (response)))
    {
    case -1:
	rptp_perror (argv[0]);
	exit (1);

    case 1:
	fprintf (stderr, "%s\n", rptp_parse (response, "error"));
	exit (1);

    case 0:
	break;
    }

    /* Save the spool id so `put' can use it later. */
    id = rptp_parse (response, "id");

    /* Read chunks of audio from the file and send them to rplayd.
       rplayd will deal with flow-control. */
    while (size > 0)
    {
	n = fread (buf, 1, sizeof (buf), fp);

	/* Use `put' to send the audio data. */
	sprintf (line, "put id=%s size=%d", id, n);
	switch (rptp_command (rptp_fd, line, response, sizeof (response)))
	{
	case -1:
	    rptp_perror (argv[0]);
	    exit (1);

	case 1:
	    fprintf (stderr, "%s\n", rptp_parse (response, "error"));
	    exit (1);

	case 0:
	    break;
	}

	nwritten = rptp_write (rptp_fd, buf, n);
	if (nwritten != n)
	{
	    rptp_perror ("flow");
	    break;
	}
	size -= nwritten;
    }

    fclose (fp);

    /* Always send `done' when the flow is over. */
    sprintf (line, "done id=%s", id);
    switch (rptp_command (rptp_fd, line, response, sizeof (response)))
    {
    case -1:
	rptp_perror (argv[0]);
	exit (1);

    case 1:
	fprintf (stderr, "%s\n", rptp_parse (response, "error"));
	exit (1);

    case 0:
	break;
    }

    exit (0);
}
