/* $Id: rptp.c,v 1.5 1999/03/10 07:58:13 boyns Exp $ */

/*
 * Copyright (C) 1993-99 Mark R. Boyns <boyns@doit.org>
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
#include "version.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif /* HAVE_LIBREADLINE */
#include "rplay.h"
#include "getopt.h"

typedef struct
{
    char *name;
    int min_args;
    int max_args;
    char *usage;
#ifdef __STDC__
    void (*func) (int argc, char **argv);
#else
    void (*func) ();
#endif
}
COMMAND;

#ifdef __STDC__
void command_open(int argc, char **argv);
void command_close(int argc, char **argv);
void command_play(int argc, char **argv);
void command_help(int argc, char **argv);
void command_list(int argc, char **argv);
void command_quit(int argc, char **argv);
void command_put(int argc, char **argv);
void command_get(int argc, char **argv);
void command_unknown(int argc, char **argv);
void command_set(int argc, char **argv);
void command_status(int argc, char **argv);
void command_generic(int argc, char **argv);
void command_volume(int argc, char **argv);
void command_skip(int argc, char **argv);
void command_set(int argc, char **argv);
void command_monitor(int argc, char **argv);
void argv_to_command(char **argv);
int connected();
void done(int exit_value);
void usage();
void do_application();
void do_error(char *repsonse);
#else
void command_open( /* int argc, char **argv */ );
void command_close( /* int argc, char **argv */ );
void command_play( /* int argc, char **argv */ );
void command_help( /* int argc, char **argv */ );
void command_list( /* int argc, char **argv */ );
void command_quit( /* int argc, char **argv */ );
void command_put( /* int argc, char **argv */ );
void command_get( /* int argc, char **argv */ );
void command_unknown( /* int argc, char **argv */ );
void command_status( /* int argc, char **argv */ );
void command_generic( /* int argc, char **argv */ );
void command_volume( /* int argc, char **argv */ );
void command_skip( /* int argc, char **argv */ );
void command_set( /* int argc, char **argv */ );
void command_monitor( /* int argc, char **argv */ );
void argv_to_command( /* char **argv */ );
int connected();
void done( /* int exit_value */ );
void usage();
void do_application();
void do_error( /* char *repsonse */ );
#endif

COMMAND commands[] =
{
    "access", 0, 0, "", command_generic,
    "close", 0, 0, "", command_close,
    "continue", 1, -1, "#id|sound ...", command_play,
    "find", 1, 1, "sound", command_generic,
    "get", 1, 2, "sound [filename]", command_get,
    "help", 0, 1, "[command]", command_help,
    "info", 1, 1, "sound", command_generic,
    "list", 0, 1, "[connections|hosts|servers|spool|sounds]", command_list,
    "monitor", 0, 0, "", command_monitor,
    "open", 1, 2, "hostname [port]", command_open,
    "pause", 1, -1, "#id|sound ...", command_play,
    "play", 1, -1, "[options] sound ...", command_play,
    "put", 1, 1, "sound", command_put,
    "quit", 0, 0, "", command_quit,
    "reset", 0, 0, "", command_generic,
    "set", 1, -1, "name=value", command_set,
    "skip", 0, 2, "[#id] [[+|-]count]", command_skip,
    "status", 0, 0, "", command_status,
    "stop", 1, -1, "#id|sound ...", command_play,
    "version", 0, 0, "", command_generic,
    "volume", 0, 1, "[[+|-]volume]", command_volume,
    "wait", 1, -1, "#id|command|event-list", command_generic,
};

#define NCOMMANDS	(sizeof(commands)/sizeof(COMMAND))

static struct option longopts[] =
{
    {"help", no_argument, NULL, 1},
    {"host", required_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {"prompt", required_argument, NULL, 2},
    {"raw", no_argument, NULL, 'r'},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

int rptp_fd = -1;
char rptp_buf[4096];
char command[RPTP_MAX_LINE];
char response[RPTP_MAX_LINE];
char *prompt = "rptp> ";
int interactive = 1;
int raw = 0;

extern int optind;
extern char *optarg;

#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc, argv)
    int argc;
    char **argv;
#endif
{
    char buf[256];
    int i;
    int port = RPTP_PORT;
    int c;
    char *av[RPTP_MAX_ARGS], *p, *host = NULL;
    int ac, first;
    char numeric_string[128];

    while ((c = getopt_long(argc, argv, "+h:p:rv", longopts, 0)) != -1)
    {
	switch (c)
	{
	case 0:
	    /* getopt has processed a long-named option -- do nothing */
	    break;

	case 1:		/* --help */
	    usage();
	    done(0);

	case 2:		/* --prompt */
	    prompt = optarg;
	    break;

	case 'h':
	    host = optarg;
	    break;

	case 'p':
	    port = atoi(optarg);
	    break;

	case 'r':
	    raw++;
	    break;

	case 'v':
	    printf("rplay %s\n", RPLAY_VERSION);
	    done(0);

	default:
	    fprintf(stderr, "Try `rptp --help' for more information.\n");
	    exit(1);
	}
    }

    if (optind != argc)
    {
	interactive = 0;
    }

    if (host == NULL)
    {
	host = rplay_default_host();
    }

    rptp_fd = rptp_open(host, port, response, sizeof(response));
    if (rptp_fd < 0)
    {
	rptp_perror("open");
    }
    else if (interactive)
    {
	if (raw)
	{
	    printf("%s\n", response);
	}
	else
	{
	    if (strchr(response, '='))
	    {
		rptp_parse(response, 0);
		printf("%s rplayd %s connected\n",
		       rptp_parse(0, "host"),
		       rptp_parse(0, "version"));
	    }
	    else
	    {
		printf("Connected to %s port %d.\n", host, port);
		printf("%s\n", response + 1);
	    }
	}
    }

    do_application();

    do
    {
	if (interactive)
	{
	    if (!raw)
	    {
#ifdef HAVE_LIBREADLINE
		p = readline(prompt);
		if (!p)
		{
		    done(0);
		}
		add_history(p);
		strcpy(buf, p);
#else
		printf(prompt);
		fflush(stdout);
#endif
	    }

#ifndef HAVE_LIBREADLINE
	    if (fgets(buf, sizeof(buf), stdin) == NULL)
	    {
		done(0);
	    }
#endif
	    first = 1;
	    ac = 0;
	    while ((p = strtok(first ? buf : NULL, " \t\r\n")))
	    {
		av[ac++] = p;
		first = 0;
	    }
	    av[ac] = NULL;
	}
	else
	{
	    int i;

	    ac = 0;
	    for (i = optind; i < argc; i++)
	    {
		av[ac++] = argv[i];
	    }
	    av[ac] = NULL;
	}

	if (av[0] == NULL)
	{
	    continue;
	}

	for (i = 0; i < NCOMMANDS; i++)
	{
	    if (strcasecmp(commands[i].name, av[0]) == 0)
	    {
		if ((commands[i].min_args >= 0 && ac - 1 < commands[i].min_args)
		    || (commands[i].max_args >= 0 && ac - 1 > commands[i].max_args))
		{
		    printf("Usage: %s %s\n", commands[i].name, commands[i].usage);
		}
		else
		{
		    (*commands[i].func) (ac, av);
		}
		break;
	    }
	}
	if (i == NCOMMANDS)
	{
	    command_unknown(ac, av);
	}
    }
    while (interactive);

    done(0);
}

#ifdef __STDC__
void
argv_to_command(char **argv)
#else
void
argv_to_command(argv)
    char **argv;
#endif
{
    command[0] = '\0';
    for (; *argv; argv++)
    {
	strcat(command, *argv);
	if (*(argv + 1))
	{
	    strcat(command, " ");
	}
    }
}

int
connected()
{
    if (rptp_fd != -1)
    {
	return 1;
    }
    else
    {
	printf("You're not connected, use `open' first.\n");
	return 0;
    }
}

#ifdef __STDC__
void
command_open(int argc, char **argv)
#else
void
command_open(argc, argv)
    int argc;
    char **argv;
#endif
{
    int port;

    if (rptp_fd != -1)
    {
	printf("You're already connected, use `close' first.\n");
    }
    else
    {
	port = argc == 3 ? atoi(argv[2]) : RPTP_PORT;
	rptp_fd = rptp_open(argv[1], port, response, sizeof(response));
	if (rptp_fd < 0)
	{
	    rptp_perror("open");
	}
	else
	{
	    if (raw)
	    {
		printf("%s\n", response);
	    }
	    else
	    {
		if (strchr(response, '='))
		{
		    printf("%s rplayd %s connected\n",
			   rptp_parse(response, "host"),
			   rptp_parse(0, "version"));
		}
		else
		{
		    printf("Connected to %s port %d.\n", argv[1], port);
		    printf("%s\n", response + 1);
		}
	    }
	    do_application();
	}
    }
}

#ifdef __STDC__
void
command_close(int argc, char **argv)
#else
void
command_close(argc, argv)
    int argc;
    char **argv;
#endif
{
    if (connected())
    {
	rptp_close(rptp_fd);
	rptp_fd = -1;
	if (interactive)
	{
	    printf("Connection closed.\n");
	}
    }
}

#ifdef __STDC__
void
command_play(int argc, char **argv)
#else
void
command_play(argc, argv)
    int argc;
    char **argv;
#endif
{
    argv_to_command(argv);

    if (!connected())
    {
	return;
    }

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }

    if (raw)
    {
	printf("%s\n", response);
    }
}

#ifdef __STDC__
void
command_generic(int argc, char **argv)
#else
void
command_generic(argc, argv)
    int argc;
    char **argv;
#endif
{
    argv_to_command(argv);

    if (!connected())
    {
	return;
    }

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }

    printf("%s\n", raw ? response : response + 1);
}

#ifdef __STDC__
void
command_help(int argc, char **argv)
#else
void
command_help(argc, argv)
    int argc;
    char **argv;
#endif
{
    int i;

    if (argc == 2)
    {
	for (i = 0; i < NCOMMANDS; i++)
	{
	    if (strcasecmp(commands[i].name, argv[1]) == 0)
	    {
		printf("Usage: %s %s\n", commands[i].name, commands[i].usage);
		return;
	    }
	}
	command_unknown(argc, argv);
    }
    else
    {
	printf("access   Display remote access permissions.\n");
	printf("close    Close the current server connection.\n");
	printf("continue Continue paused sounds.\n");
	printf("find     Search for a sound.\n");
	printf("get      Retrieve a sound.\n");
	printf("help     Display help information.\n");
	printf("info     Display sound information.\n");
	printf("list     Display various server information.\n");
	printf("open     Connect to a server.\n");
	printf("pause    Pause sounds that are playing.\n");
	printf("play     Play sounds\n");
	printf("put      Send a sound.\n");
	printf("quit     Terminate the rptp session.\n");
	printf("reset    Tell the server to reset itself.\n");
	printf("set      Change server settings.\n");
	printf("skip     Skip sounds in a sound list.\n");
	printf("status   Display server statistics.\n");
	printf("stop     Stop sounds that are playing.\n");
	printf("version  Display the version of the server.\n");
	printf("volume   Get and set the volume of the audio device.\n");
	printf("wait     Wait for a spool id, volume change, or command execution.\n");
    }
}

#ifdef __STDC__
void
command_list(int argc, char **argv)
#else
void
command_list(argc, argv)
    int argc;
    char **argv;
#endif
{
    int n;

    if (!connected())
    {
	return;
    }

    argv_to_command(argv);

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	if (raw)
	{
	    printf("%s\n", response);
	}
	break;
    }

    for (;;)
    {
	n = rptp_getline(rptp_fd, rptp_buf, sizeof(rptp_buf));
	if (n < 0)
	{
	    rptp_perror("list");
	    command_close(argc, argv);
	    break;
	}
	if (strcmp(rptp_buf, ".") == 0)
	{
	    if (raw)
	    {
		printf("%s\n", rptp_buf);
	    }
	    break;
	}
	printf("%s\n", rptp_buf);
    }
}

#ifdef __STDC__
void
command_quit(int argc, char **argv)
#else
void
command_quit(argc, argv)
    int argc;
    char **argv;
#endif
{
    if (rptp_fd != -1)
    {
	rptp_close(rptp_fd);
	printf("Connection closed.\n");
    }
    done(0);
}

#ifdef __STDC__
void
command_put(int argc, char **argv)
#else
void
command_put(argc, argv)
    int argc;
    char **argv;
#endif
{
    FILE *fp;
    int size, n, nwritten;
    struct stat st;
    char line[RPTP_MAX_LINE];
    int total_size;

    if (!connected())
    {
	return;
    }

    if (stat(argv[1], &st) < 0)
    {
	perror(argv[1]);
	return;
    }
    fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
	perror(argv[1]);
	return;
    }
    size = st.st_size;

    sprintf(line, "put sound=%s size=%d", argv[1], size);

    switch (rptp_command(rptp_fd, line, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }

    if (raw)
    {
	printf("%s\n", response);
    }

    total_size = size;
    while (size > 0)
    {
	n = fread(rptp_buf, 1, sizeof(rptp_buf), fp);
	nwritten = rptp_write(rptp_fd, rptp_buf, n);
	if (nwritten != n)
	{
	    rptp_perror("put");
	    command_close(argc, argv);
	    break;
	}
	size -= nwritten;

	sprintf(line, "\r%s %d/%d %d%%", argv[1],
		total_size - size, total_size,
		(int)(((float)(total_size-size)/total_size)*100));
	write(2, line, strlen(line));
    }
    write(2, "\n", 1);
    fclose(fp);
}

#ifdef __STDC__
void
command_get(int argc, char **argv)
#else
void
command_get(argc, argv)
    int argc;
    char **argv;
#endif
{
    FILE *fp;
    char *filename;
    char *p;
    int size, n, nread, total_size;
    char line[RPTP_MAX_LINE];

    if (!connected())
    {
	return;
    }

    if (argc == 3)
    {
	filename = argv[2];
	argv[2] = NULL;
    }
    else
    {
	filename = argv[1];
    }

    sprintf(line, "get sound=%s", argv[1]);

    switch (rptp_command(rptp_fd, line, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }

    if (raw)
	printf("%s\n", response);

    fp = fopen(filename, "w");
    if (fp == NULL)
    {
	perror(filename);
	return;
    }

    if (strchr(response, '='))
    {
	size = atoi(rptp_parse(response, "size"));
    }
    else
    {
	p = strtok(response + 1, " ");
	size = atoi(strtok(NULL, "\r\n"));
    }

    total_size = size;
    while (size > 0)
    {
	n = MIN(sizeof(rptp_buf), size);
	nread = rptp_read(rptp_fd, rptp_buf, n);
	if (nread != n)
	{
	    rptp_perror("get");
	    break;
	}
	fwrite(rptp_buf, 1, n, fp);
	size -= n;
	
	sprintf(line, "\r%s %d/%d %d%%", filename,
		total_size - size, total_size,
		(int)(((float)(total_size-size)/total_size)*100));
	write(2, line, strlen(line));
    }
    write(2, "\n", 1);
    fclose(fp);
}

#ifdef __STDC__
void
command_unknown(int argc, char **argv)
#else
void
command_unknown(argc, argv)
    int argc;
    char **argv;
#endif
{
    printf("unknown command `%s'.\n", argv[0]);
}

#ifdef __STDC__
void
command_status(int argc, char **argv)
#else
void
command_status(argc, argv)
    int argc;
    char **argv;
#endif
{
    argv_to_command(argv);

    if (!connected())
    {
	return;
    }

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }

    if (raw)
    {
	printf("%s\n", response);
    }
    else
    {
	int first = 1;
	char *name, *value;

	while (name = rptp_parse(first ? response : 0, 0))
	{
	    first = 0;
	    value = rptp_parse(0, name);
	    printf("%s=%s\n", name, value);
	}
    }
}

#ifdef __STDC__
void
command_volume(int argc, char **argv)
#else
void
command_volume(argc, argv)
    int argc;
    char **argv;
#endif
{
    char *volume;

    if (!connected())
    {
	return;
    }

    if (argc == 2)
    {
	sprintf(command, "set volume=%s", argv[1]);
    }
    else
    {
	sprintf(command, "set volume");
    }

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }

    volume = rptp_parse(response, "volume");
    if (volume && *volume)
    {
	printf("volume=%s\n", volume);
    }
    else
    {
	printf("unknown response `%s'\n", response);
    }
}

#ifdef __STDC__
void
command_skip(int argc, char **argv)
#else
void
command_skip(argc, argv)
    int argc;
    char **argv;
#endif
{
    char *value;

    if (!connected())
    {
	return;
    }

    if (strchr(command, '='))
    {
	/* Leave command alone. */
    }
    else if (argc == 3)
    {
	sprintf(command, "skip id=%s count=%s", argv[1], argv[2]);
    }
    else if (argc == 2)
    {
	sprintf(command, "skip id=%s count=1", argv[1]);
    }
    else
    {
	sprintf(command, "skip id=#0 count=1");
    }

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }

    value = rptp_parse(response, "message");
    if (value && *value)
    {
	printf("%s\n", value);
    }
    else
    {
	printf("unknown response `%s'\n", response);
    }
}

#ifdef __STDC__
void
command_set(int argc, char **argv)
#else
void
command_set(argc, argv)
    int argc;
    char **argv;
#endif
{
    char *value;

    if (!connected())
    {
	return;
    }

    argv_to_command(argv);

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }
}

#ifdef __STDC__
void
command_monitor(int argc, char **argv)
#else
void
command_monitor(argc, argv)
    int argc;
    char **argv;
#endif
{
    char *value, *p;
    char buf[8192];
    char line[80];
    int n, size;
    int sample_rate, precision, channels, sample_size;
    int hours, mins, secs, prev_secs;
    
    if (!connected())
    {
	return;
    }

    sprintf(command, "monitor");

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror(argv[0]);
	command_close(argc, argv);
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }

    p = strtok(rptp_parse(response, "audio-info"), ",");
    // input_format

    p = strtok(NULL, ",");
    if (p) sample_rate = atoi(p);

    p = strtok(NULL, ",");
    if (p) precision = atoi(p);

    p = strtok(NULL, ",");
    if (p) channels = atoi(p);

    sample_size = precision/8 * channels;

    fprintf(stderr, "%dhz %dbit %s\n", sample_rate, precision,
	    channels == 2 ? "stereo" : "mono");

    size = hours = mins = secs = prev_secs = 0;
    while ((n = read(rptp_fd, buf, sizeof(buf))) > 0)
    {
	write(1, buf, n);
	size += n;

	secs = size / (sample_rate * sample_size);
	hours = secs / 3600;
	secs = secs % 3600;
	mins = secs / 60;
	secs = secs % 60;

	/* update once per second */
	if (secs != prev_secs)
	{
	    sprintf(line, "\r%02d:%02d:%02d %0.2fM",
		    hours, mins, secs,
		    (float)size/1048576);
	    write(2, line, strlen(line));
	}
	prev_secs = secs;
    }
    write(2, "\n", 1);
    close(rptp_fd);
}

void
do_application()
{
    sprintf(command, "set application=\"rptp %s\"", RPLAY_VERSION);

    switch (rptp_command(rptp_fd, command, response, sizeof(response)))
    {
    case -1:
	rptp_perror("application");
	rptp_close(rptp_fd);
	rptp_fd = -1;
	return;

    case 1:
	do_error(response);
	return;

    case 0:
	break;
    }
}

#ifdef __STDC__
void
do_error(char *response)
#else
void
do_error(response)
    char *response;
#endif
{
    if (raw)
    {
	printf("%s\n", response);
    }
    else
    {
	char *error;

	error = rptp_parse(response, "error");
	if (!error || !*error)
	{
	    error = response;
	}
	printf("%s\n", error);
    }
}

void
usage()
{
    printf("\nrplay %s\n\n", RPLAY_VERSION);
    printf("usage: rptp [options] [command]\n");
    printf("--help\n");
    printf("\tDisplay helpful information.\n");
    printf("\n");

    printf("-h HOST, --host=HOST\n");
    printf("\tSpecify the RPTP host, default = %s.\n",
	   rplay_default_host());
    printf("\n");

    printf("-p PORT, --port=PORT\n");
    printf("\tUse PORT instead of the default RPTP port, default = %d.\n", RPTP_PORT);
    printf("\n");

    printf("-r, --raw\n");
    printf("\tEnable raw RPTP mode.\n");
    printf("\n");

    printf("-v, --version\n");
    printf("\tDisplay rplay version information.\n");
}

#ifdef __STDC__
void
done(int exit_value)
#else
void
done(exit_value)
    int exit_value;
#endif
{
    exit(exit_value);
}
