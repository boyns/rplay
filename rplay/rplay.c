/* $Id: rplay.c,v 1.2 1998/08/13 06:13:39 boyns Exp $ */

/*
 * Copyright (C) 1993-98 Mark R. Boyns <boyns@doit.org>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <unistd.h>
#include "rplay.h"
#include "getopt.h"

#undef DEBUG

#define OPTIONS "+spcN:P:h:rv:n:R:b:i:"

struct option longopts[] =
{
    {"continue", no_argument, NULL, 'c'},
    {"count", required_argument, NULL, 'n'},
    {"help", no_argument, NULL, 3},
    {"host", required_argument, NULL, 'h'},
    {"hosts", required_argument, NULL, 'h'},
    {"info", required_argument, NULL, 'i'},
    {"info-amd", no_argument, NULL, 8},
    {"info-ulaw", no_argument, NULL, 8},
    {"info-cs4231", no_argument, NULL, 10},
    {"info-dbri", no_argument, NULL, 9},
    {"info-gsm", no_argument, NULL, 13},
    {"buffer-size", required_argument, NULL, 'b'},
    {"list-count", required_argument, NULL, 'N'},
    {"list-name", required_argument, NULL, 7},
    {"pause", no_argument, NULL, 'p'},
    {"port", required_argument, NULL, 4},
    {"priority", required_argument, NULL, 'P'},
    {"random", no_argument, NULL, 'r'},
    {"reset", no_argument, NULL, 5},
    {"rplay", no_argument, NULL, 11},
    {"RPLAY", no_argument, NULL, 11},
    {"rptp", no_argument, NULL, 12},
    {"RPTP", no_argument, NULL, 12},
    {"sample-rate", required_argument, NULL, 'R'},
    {"stop", no_argument, NULL, 's'},
    {"version", no_argument, NULL, 2},
    {"volume", required_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

#define PROTOCOL_GUESS 0
#define PROTOCOL_RPLAY 1
#define PROTOCOL_RPTP 2
#define BUFFER_SIZE 8196

#ifdef __STDC__
typedef int (*PLAY_FUNC) (int, char *, int, RPLAY *, int);
#else
typedef int (*PLAY_FUNC) ();
#endif

void doit (), play (), usage (), interrupt ();
int server_has_sound ();
char *info2str ();
int play_with_flow (), play_with_play ();

RPLAY *rp;
char cwd[MAXPATHLEN];
int interrupted = 0;
int which_protocol = PROTOCOL_GUESS;
int which_port = -1;
int buffer_size = BUFFER_SIZE;

#ifdef __STDC__
main (int argc, char **argv)
#else
main (argc, argv)
    int argc;
    char **argv;
#endif
{
    int i, n, c, command, volume, val;
    int list_count, count, priority, do_random;
    unsigned long sample_rate;
    char *hosts, *name, *list_name, *sound_info = "";
    int optind_val = optind;
    char buf[RPTP_MAX_LINE];
    extern char *optarg;
    extern int optind, opterr;

    if (argc < 2)
    {
	usage ();
    }

    signal (SIGPIPE, SIG_IGN);
    
    if (getcwd (cwd, sizeof (cwd)) == NULL)
    {
	cwd[0] = '\0';
    }
    
    command = RPLAY_PLAY;
    list_count = RPLAY_DEFAULT_LIST_COUNT;
    list_name = NULL;
    priority = RPLAY_DEFAULT_PRIORITY;
    do_random = 0;
    hosts = rplay_default_host ();

    name = NULL;
    sample_rate = RPLAY_DEFAULT_SAMPLE_RATE;
    volume = RPLAY_DEFAULT_VOLUME;
    count = RPLAY_DEFAULT_COUNT;

    /* First scan the args to see what the command is. */
    opterr = 0; /* disable getopt errors */
    while ((c = getopt_long (argc, argv, OPTIONS, longopts, 0)) != -1)
    {
	switch (c)
	{
	case 2:		/* --version */
	    printf ("rplay %s\n", RPLAY_VERSION);
	    exit (0);

	case 3:		/* --help */
	    usage ();

	case 5:		/* --reset */
	    if (command != RPLAY_PLAY)
	    {
		usage ();
	    }
	    command = RPLAY_RESET;
	    break;

	case 's':
	    if (command != RPLAY_PLAY)
	    {
		usage ();
	    }
	    command = RPLAY_STOP;
	    break;

	case 'p':
	    if (command != RPLAY_PLAY)
	    {
		usage ();
	    }
	    command = RPLAY_PAUSE;
	    break;

	case 'c':
	    if (command != RPLAY_PLAY)
	    {
		usage ();
	    }
	    command = RPLAY_CONTINUE;
	    break;
	}
    }
    opterr = 1;			/* enable getopt errors */
    optind = optind_val;	/* reset optind */

    /* Create the RPLAY object. */
    rp = rplay_create (command);
    if (rp == NULL)
    {
	rplay_perror ("rplay_create");
	exit (1);
    }

    /* Build a list of sounds to be played along with their
       attributes. */
    while (argc > 1)
    {
	while ((c = getopt_long (argc, argv, OPTIONS, longopts, 0)) != -1)
	{
	    switch (c)
	    {
	    case 0:
		/* getopt has processed a long-named option -- do nothing */
		break;

	    case 1:
		break;
		
	    case 2:		/* --version */
		printf ("rplay %s\n", RPLAY_VERSION);
		exit (0);

	    case 3:		/* --help */
		usage ();

	    case 4:		/* --port */
		which_port = atoi (optarg);
		break;

	    case 5:		/* --reset */
	    case 's':
	    case 'p':
	    case 'c':
		break;

	    case 7:		/* --list-name */
		list_name = optarg;
		break;

	    case 8:		/* --info-amd, --info-ulaw */
		/* Sun's amd audio device. */
		sound_info = "ulaw,8000,8,1,big-endian,0";
		break;

	    case 9:		/* --info-dbri */
		/* Sun's dbri audio device. */
		sound_info = "linear16,11025,16,1,big-endian,0";
		break;

	    case 10:		/* --info-cs4231 */
		/* Sun's CS4231 audio device. */
		sound_info = "linear16,11025,16,1,big-endian,0";
		break;

	    case 11:		/* --rplay */
		which_protocol = PROTOCOL_RPLAY;
		break;

	    case 12:		/* --rptp */
		which_protocol = PROTOCOL_RPTP;
		break;

	    case 13:		/* --info-gsm */
		/* GSM encoded u-law files */
		sound_info = "gsm,8000";
		break;
		
	    case 'N':
		list_count = atoi (optarg);
		break;

	    case 'P':
		priority = atoi (optarg);
		break;

	    case 'r':
		do_random++;
		break;

	    case 'h':
		hosts = optarg;
		break;

	    case 'v':
		volume = atoi (optarg);
		break;

	    case 'n':
		count = atoi (optarg);
		break;

	    case 'R':
		sample_rate = atoi (optarg);
		break;

	    case 'i':
		sound_info = optarg;
		break;

	    case 'b':
		buffer_size = atoi (optarg);
		break;

	    default:
		fprintf (stderr, "Try `rplay --help' for more information.\n");
		exit (1);
	    }
	}

	if (argc == optind)
	{
	    if (command == RPLAY_PLAY)
	    {
		usage ();
	    }
	    name = "#0";
	}
	/* Convert relative file names to absolute. */
	else if ((*(argv[optind]) != '/') && (strchr (argv[optind], '/')))
	{
	    if (*cwd)
	    {
		name = (char *) malloc (strlen (cwd) + strlen (argv[optind]) + 2);
		strcpy (name, cwd);
		strcat (name, "/");
		if (strncmp (argv[optind], "./", 2) == 0)
		{
		    strcat (name, argv[optind]+2);
		}
		else
		{
		    strcat (name, argv[optind]);
		}
	    }
	    else
	    {
		name = argv[optind];
	    }
	}
	else
	{
	    name = argv[optind];
	}

	argv += optind;
	argc -= optind;
	optind = optind_val;

	if (rplay_set (rp, RPLAY_LIST_COUNT, list_count, NULL) < 0)
	{
	    rplay_perror ("rplay_set");
	    exit (1);
	}

	if (rplay_set (rp, RPLAY_PRIORITY, priority, NULL) < 0)
	{
	    rplay_perror ("rplay_set");
	    exit (1);
	}

	if (list_name && rplay_set (rp, RPLAY_LIST_NAME, list_name, NULL) < 0)
	{
	    rplay_perror ("rplay_set");
	    exit (1);
	}
	
	val = rplay_set (rp, RPLAY_APPEND,
			 RPLAY_SOUND, name,
			 RPLAY_VOLUME, volume,
			 RPLAY_COUNT, count,
			 RPLAY_SAMPLE_RATE, sample_rate,
			 RPLAY_CLIENT_DATA, sound_info,
			 NULL);
	if (val < 0)
	{
	    rplay_perror ("rplay_set");
	    exit (1);
	}
    }

    /* Pick the random sound. */
    if (do_random)
    {
	val = rplay_set (rp, RPLAY_RANDOM_SOUND, NULL);
	if (val < 0)
	{
	    rplay_perror ("rplay_set");
	    exit (1);
	}
    }

    doit (hosts);
    
    exit (0);
}

#ifdef __STDC__
void
doit (char *hostp)
#else
void
doit (hostp)
    char *hostp;
#endif
{
    PLAY_FUNC *play_table;
    char *host;

    play_table = (PLAY_FUNC *) malloc (rplay_get (rp, RPLAY_NSOUNDS) * sizeof (PLAY_FUNC));
    if (play_table == NULL)
    {
	fprintf (stderr, "rplay: out of memory\n");
	exit (1);
    }
    
    /* Cycle through each colon-separated host. */
    do
    {
	int protocol, port;
	int rplay_fd = -1;
	
	host = hostp;
	hostp = strchr (host, ':');
	if (hostp != NULL)
	{
	    *hostp++ = '\0';
	}

	/* Determine which protocol to use. */

	if (rplay_get (rp, RPLAY_COMMAND) != RPLAY_PLAY)
	{
	    protocol = PROTOCOL_RPLAY;
	}
	else if (strcmp ((char *) rplay_get (rp, RPLAY_SOUND, 0), "-") == 0)
	{
	    protocol = PROTOCOL_RPTP;
	    play_table[0] = play_with_flow;
	}
	else if (which_protocol == PROTOCOL_GUESS)
	{
	    char response[RPTP_MAX_LINE];
	    rplay_fd = rptp_open (host, RPTP_PORT, response, sizeof (response));
	    if (rplay_fd < 0)
	    {
		protocol = PROTOCOL_RPLAY;
	    }
	    else
	    {
		int i, n, sounds_not_found = 0;
		char *sound_name;
		
		/* Count the number of sounds the server doesn't have */
		n = (int) rplay_get (rp, RPLAY_NSOUNDS);
		for (i = 0; i < n; i++)
		{
		    struct stat st;
			
		    sound_name = (char *) rplay_get (rp, RPLAY_SOUND, i);
		    if (stat (sound_name, &st) < 0)
		    {
			st.st_size = 0;
		    }
			
		    if (!server_has_sound (rplay_fd, sound_name, (int)st.st_size))
		    {
			if (st.st_size)
			{
			    play_table[i] = play_with_flow;
			    sounds_not_found++;
			}
			else
			{
			    /* Assume the server can get the sound
                               from another server. */
			    play_table[i] = play_with_play;
			}
		    }
		    else
		    {
			play_table[i] = play_with_play;
		    }
		}

		/* XXX is this right??? should they be switched?? */
		if (sounds_not_found > 0)
		{
		    protocol = PROTOCOL_RPTP;
		}
		else
		{
		    protocol = PROTOCOL_RPLAY;
		}
	    }
	}

	/* Allow the protocol choice to be overridden. */
	if (which_protocol != PROTOCOL_GUESS
	    && rplay_get (rp, RPLAY_COMMAND) == RPLAY_PLAY)
	{
	    int i, n;
	    
	    protocol = which_protocol;
	    n = (int) rplay_get (rp, RPLAY_NSOUNDS);
	    for (i = 0; i < n; i++)
	    {
		play_table[i] = (protocol == PROTOCOL_RPTP) ? play_with_flow : play_with_play;
	    }
	}
	
	port = which_port;
	if (port == -1)
	{
	    switch (protocol)
	    {
	    case PROTOCOL_RPLAY:
		port = RPLAY_PORT;
		break;
	    
	    case PROTOCOL_RPTP:
		port = RPTP_PORT;
		break;
	    }
	}

	switch (protocol)
	{
	case PROTOCOL_RPTP:
	    play (rplay_fd, host, port, rp, play_table);
	    break;

	case PROTOCOL_RPLAY:
	    if (rplay_fd != -1)
	    {
		rptp_close (rplay_fd);
	    }
	    rplay_fd = rplay_open_port (host, port);
	    if (rplay_fd < 0)
	    {
		rplay_perror (host);
		exit (1);
	    }
	    if (rplay (rplay_fd, rp) < 0)
	    {
		rplay_perror (host);
		exit (1);
	    }
	    rplay_close (rplay_fd);
	    break;

	default:
	    fprintf (stderr, "unknown protocol\n");
	    exit (1);
	}
    }
    while (hostp != NULL);
}


#ifdef __STDC__
int
server_has_sound (int rplay_fd, char *sound_name, int size)
#else
int
server_has_sound (rplay_fd, sound_name, size)
    int rplay_fd;
    char *sound_name;
    int size;
#endif    
{
    char command[RPTP_MAX_LINE];
    char response[RPTP_MAX_LINE];
    char *p;
    int try;

    for (try = 0; try < 2; try++)
    {
	if (try == 0)
	{
	    sprintf (command, "info sound=%s", sound_name);
	}
	else
	{
	    sprintf (command, "info sound=%s/%s", cwd, sound_name);
	}
		
	if (rptp_command (rplay_fd, command, response, sizeof (response)) < 0)
	{
	    return 0;
	}

	p = rptp_parse (response, "size");
	if (p && (size == 0 || atoi (p) == size)) /* They're the same */
	{
	    return 1;
	}
    }

    return 0;
}

#ifdef __STDC__
void
play (int rplay_fd, char *host, int port, RPLAY *rp, PLAY_FUNC *play_table)
#else
void
play (rplay_fd, rp, play_table)
    int rplay_fd;
    RPLAY *rp;
    PLAY_TABLE *play_table;
#endif	
{
    int i, n;
    char response[RPTP_MAX_LINE];

    n = rplay_get (rp, RPLAY_NSOUNDS);
    for (i = 0; i < n; i++)
    {
	if (rplay_fd < 0)
	{
	    rplay_fd = rptp_open (host, port, response, sizeof (response));
	}
	if (rplay_fd < 0)
	{
	    rptp_perror (host);
	    continue;
	}
	if ((*play_table[i])(rplay_fd, host, port, rp, i) < 0)
	{
	    rptp_close (rplay_fd);
	    rplay_fd = -1;
	}
    }
}

#ifdef __STDC__
int
play_with_play (int rplay_fd, char *host, int port, RPLAY *rp, int index)
#else
int
play_with_play (rplay_fd, host, port, rp, index)
    int rplay_fd;
    char *host;
    int port;
    RPLAY *rp;
    int index;
#endif
{
    int n, spool_id;
    char response[RPTP_MAX_LINE];

    /* Enable `done' notification. */
    rptp_putline (rplay_fd, "set notify=done");
    rptp_getline (rplay_fd, response, sizeof (response));

    /* Play the sound. */
    rptp_putline (rplay_fd, "play priority=%d sample-rate=%d volume=%d sound=\"%s\"",
		  (int) rplay_get (rp, RPLAY_PRIORITY, index),
		  (int) rplay_get (rp, RPLAY_SAMPLE_RATE, index),
		  (int) rplay_get (rp, RPLAY_VOLUME, index),
		  (int) rplay_get (rp, RPLAY_SOUND, index));
    n = rptp_getline (rplay_fd, response, sizeof (response));
    if (n < 0 || response[0] != RPTP_OK)
    {
	fprintf (stderr, "rplay: can't play `%s'\n",
		 (int) rplay_get (rp, RPLAY_SOUND, index));
	return -1;
    }
    
    /* Grab the spool id. */
    spool_id = atoi (1 + rptp_parse (response, "id"));
    
    /* Wait for the sound to finish playing. */
    signal (SIGINT, interrupt);
    interrupted = 0;
    for (;;)
    {
	n = rptp_getline (rplay_fd, response, sizeof (response));
	if (interrupted)
	{
	    break;
	}
	if (n < 0)
	{
	    return -1;
	}
	else if (response[0] != RPTP_NOTIFY)
	{
	    fprintf (stderr, "rplay: %s\n", response+1);
	    break;
	}
	else if (atoi (1 + rptp_parse (response, "id")) == spool_id)
	{
	    break;
	}
    }
    signal (SIGINT, SIG_DFL);

    /* Eat any events and turn off notification. */
    rptp_putline (rplay_fd, "set notify=none");
    for (;;)
    {
	n = rptp_getline (rplay_fd, response, sizeof (response));
	if (n < 0)
	{
	    return -1;
	}
	else if (response[0] == RPTP_OK || response[0] == RPTP_ERROR)
	{
	    break;
	}
    }

    return 0;
}

#ifdef __STDC__
int
play_with_flow (int rplay_fd, char *host, int port, RPLAY *rp, int index)
#else
int
play_with_flow (rplay_fd, host, port, rp, index)
    int rplay_fd;
    char *host;
    int port;
    RPLAY *rp;
    int index;
#endif    
{
    char *sound_name = (char *) rplay_get (rp, RPLAY_SOUND, index);
    char *sound_info = (char *) rplay_get (rp, RPLAY_CLIENT_DATA, index);
    char command[RPTP_MAX_LINE];
    char response[RPTP_MAX_LINE];
    char *buffer;
    int spool_id, n, fd, flow_fd;

    if (strcmp (sound_name, "-") == 0)
    {
	fd = 0;
    }
    else
    {
	fd = open (sound_name, O_RDONLY|O_NDELAY, 0);
	if (fd < 0)
	{
	    fprintf (stderr, "rplay: sound `%s' not found\n", sound_name);
	    return 0;
	}
    }

    /* Open a connect for the flow. */
    flow_fd = rptp_open (host, port, response, sizeof (response));
    if (flow_fd < 0)
    {
	rptp_perror (host);
	return -1;
    }
    
    /* Enable `done' notification. */
    rptp_putline (rplay_fd, "set notify=done");
    rptp_getline (rplay_fd, response, sizeof (response));

    /* Send the flow play command. */
    rptp_putline (flow_fd, "play input=flow %s priority=%d sample-rate=%d volume=%d sound=\"%s\"",
		  info2str (sound_info),
		  (int) rplay_get (rp, RPLAY_PRIORITY, index),
		  (int) rplay_get (rp, RPLAY_SAMPLE_RATE, index),
		  (int) rplay_get (rp, RPLAY_VOLUME, index),
		  strcmp (sound_name, "-") ? sound_name : "stdin");
    n = rptp_getline (flow_fd, response, sizeof (response));
    if (n < 0 || response[0] != RPTP_OK)
    {
	fprintf (stderr, "rplay: can't play `%s'\n",
		 (int) rplay_get (rp, RPLAY_SOUND, index));
	return -1;
    }

    /* Grab the spool id. */
    spool_id = atoi (1 + rptp_parse (response, "id"));

    /* Send the put command. */
    rptp_putline (flow_fd, "put id=#%d size=0", spool_id);
    n = rptp_getline (flow_fd, response, sizeof (response));
    if (n < 0 || response[0] != RPTP_OK)
    {
	fprintf (stderr, "rplay: can't play `%s'\n",
		 (int) rplay_get (rp, RPLAY_SOUND, index));
	return -1;
    }
    
    /* Allocate the buffer. */
    buffer = (char *) malloc (buffer_size);
    if (buffer == NULL)
    {
	fprintf (stderr, "rplay: out of memory\n");
	exit (1);
    }
    
    signal (SIGINT, interrupt);
    interrupted = 0;
    for (;;)
    {
	n = read (fd, buffer, buffer_size);
	if (interrupted)
	{
	    break;
	}
	if (n <= 0)
	{
	    break;
	}
	if (rptp_write (flow_fd, buffer, n) != n)
	{
	    break;
	}
    }

    close (fd);
    sleep (1);
    rptp_close (flow_fd);

    /* Wait for the sound to finish. */
    for (;;)
    {
	n = rptp_getline (rplay_fd, response, sizeof (response));
	if (interrupted)
	{
	    break;
	}
	if (n < 0)
	{
	    return -1;
	}
	else if (response[0] != RPTP_NOTIFY)
	{
	    fprintf (stderr, "rplay: %s\n", response+1);
	    break;
	}
	else if (atoi (1 + rptp_parse (response, "id")) == spool_id)
	{
	    break;
	}
    }
    
    signal (SIGINT, SIG_DFL);

    /* Eat any events and turn off notification. */
    rptp_putline (rplay_fd, "set notify=none");
    for (;;)
    {
	n = rptp_getline (rplay_fd, response, sizeof (response));
	if (n < 0)
	{
	    return -1;
	}
	else if (response[0] == RPTP_OK || response[0] == RPTP_ERROR)
	{
	    break;
	}
    }

    return 0;
}

#ifdef __STDC__
char *
info2str (char *sound_info)
#else
char *
info2str (sound_info)
    char *sound_info;
#endif    
{
    static char str[1024];

    str[0] = '\0';
    if (sound_info && *sound_info)
    {
	char buf[1024];
	char *p;
	
	strcpy (buf, sound_info);
	/* Example: ulaw,8000,8,1,big-endian,offset */
	p = strtok (buf, ", ");
	if (p) sprintf (str + strlen(str), "input-format=%s ", p);
	p = strtok (NULL, ", ");
	if (p) sprintf (str + strlen(str), "input-sample-rate=%s ", p);
	p = strtok (NULL, ", ");
	if (p) sprintf (str + strlen(str), "input-bits=%s ", p);
	p = strtok (NULL, ", ");
	if (p) sprintf (str + strlen(str), "input-channels=%s ", p);
	p = strtok (NULL, ", ");
	if (p) sprintf (str + strlen(str), "input-byte-order=%s ", p);
	p = strtok (NULL, ", ");
	if (p) sprintf (str + strlen(str), "input-offset=%s ", p);
    }
    
    return str;
}

void
interrupt ()
{
    interrupted++;
}

void
usage ()
{
    printf ("\nrplay %s\n\n", RPLAY_VERSION);
    printf ("usage: rplay [options] [sound ... ]\n\n");

    printf ("-b BYTES, --buffer-size=BYTES\n");
    printf ("\tUse of a buffer size of BYTES when playing sounds using RPTP flows.\n");
    printf ("\tThe default is 8K.\n");
    printf ("\n");
    
    printf ("-c, --continue\n");
    printf ("\tContinue sounds.\n");
    printf ("\n");

    printf ("-n N, --count=N\n");
    printf ("\tNumber of times to play the sound, default = %d.\n",
	    RPLAY_DEFAULT_COUNT);
    printf ("\n");

    printf ("-N N, --list-count=N\n");
    printf ("\tNumber of times to play all the sounds, default = %d.\n",
	    RPLAY_DEFAULT_LIST_COUNT);
    printf ("\n");

    printf ("--list-name=NAME\n");
    printf ("\tName this list NAME.  rplayd appends sounds with the same\n");
    printf ("\tNAME into the same sound list -- it plays them sequentially.\n");
    printf ("\n");

    printf ("--help\n");
    printf ("\tDisplay helpful information.\n");
    printf ("\n");

    printf ("-h HOST, --host=HOST, --hosts=HOST\n");
    printf ("\tSpecify the rplay host, default = %s.\n", rplay_default_host ());
    printf ("\n");

    printf ("-i INFO, --info=INFO\n");
    printf ("\tAudio information for a sound file.  This option is intended\n");
    printf ("\tto be used when sounds are read from standard input.\n");
    printf ("\tINFO must be of the form:\n");
    printf ("\t    `format,sample-rate,bits,channels,byte-order,offset'\n");
    printf ("\tExamples: ulaw,8000,8,1,big-endian,0\n");
    printf ("\t          gsm,8000\n");
    printf ("\tShorthand info is provided for Sun's audio devices using the\n");
    printf ("\tfollowing options: --info-amd, --info-dbri, --info-cs4231.\n");
    printf ("\tThere's also: --info-ulaw and --info-gsm.\n");
    printf ("\n");

    printf ("-p, --pause\n");
    printf ("\tPause sounds.\n");
    printf ("\n");

    printf ("--port=PORT\n");
    printf ("\tUse PORT instead of the default RPLAY/UDP or RPTP/TCP port.\n");
    printf ("\n");

    printf ("-P N, --priority=N\n");
    printf ("\tPlay sounds at priority N (%d <= N <= %d), default = %d.\n",
	    RPLAY_MIN_PRIORITY,
	    RPLAY_MAX_PRIORITY,
	    RPLAY_DEFAULT_PRIORITY);
    printf ("\n");

    printf ("-r, --random\n");
    printf ("\tRandomly choose one of the given sounds.\n");
    printf ("\n");

    printf ("--reset\n");
    printf ("\tTell the server to reset itself.\n");
    printf ("\n");

    printf ("--rplay, --RPLAY\n");
    printf ("\tForce the use of the RPLAY protocol.\n");
    printf ("\
\tThe default protocol to be used is determined by checking whether or not\n\
\tthe server has local access to the specified sounds.  RPLAY is used when\n\
\tsounds are accessible, otherwise RPTP and possibly flows are used.\n\
\tRPLAY will also be used when sound accessibility cannot be determined.\n");
    printf ("\n");

    printf ("--rptp, --RPTP\n");
    printf ("\tForce the use of the RPTP protocol.\n");
    printf ("\tSee `--rplay' for more information about protocols.\n");
    printf ("\n");

    printf ("-R N, --sample-rate=N\n");
    printf ("\tPlay sounds at sample rate N, default = %d.\n",
	    RPLAY_DEFAULT_SAMPLE_RATE);
    printf ("\n");

    printf ("-s, --stop\n");
    printf ("\tStop sounds.\n");
    printf ("\n");

    printf ("--version\n");
    printf ("\tPrint the rplay version and exit.\n");
    printf ("\n");

    printf ("-v N, --volume=N\n");
    printf ("\tPlay sounds at volume N (%d <= N <= %d), default = %d.\n",
	    RPLAY_MIN_VOLUME,
	    RPLAY_MAX_VOLUME,
	    RPLAY_DEFAULT_VOLUME);

    exit (1);
}
