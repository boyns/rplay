/* $Id: rplayd.c,v 1.7 1998/11/10 15:29:55 boyns Exp $ */

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
#include "version.h"
#include "rplay.h"
#include <sys/param.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#ifdef ultrix
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/uio.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <sys/errno.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <pwd.h>
#include <grp.h>
#include "rplayd.h"
#include "connection.h"
#include "spool.h"
#include "sound.h"
#ifdef AUTH
#include "host.h"
#endif /* AUTH */
#include "server.h"
#include "misc.h"
#include "cache.h"
#include "timer.h"
#include "strdup.h"
#include "getopt.h"
#include "tilde.h"
#ifdef HAVE_CDROM
#include "cdrom.h"
#endif /* HAVE_CDROM */
#ifdef HAVE_HELPERS
#include "helper.h"
#endif /* HAVE_HELPERS */

/* Make sure MAXHOSTNAMELEN is defined. */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

/* Global variables */
fd_set read_mask;		/* Mask of RPTP clients that are reading.  */
fd_set write_mask;		/* Mask of RPTP clients that are writing.  */
int debug = 0;			/* Is debugging enable? (--debug) */
int rptp_timeout = RPTP_CONNECTION_TIMEOUT;	/* --connection-timeout */
char hostname[MAXHOSTNAMELEN];	/* Name of the local system.  */
char *hostaddr;			/* IP address of the local system.  */
int rplay_fd = -1;		/* Socket used for RPLAY packets.  */
int rptp_fd = -1;		/* Socket used for RPTP connections.  */
#ifdef OTHER_RPLAY_PORTS
int other_rplay_fd = -1;	/* Another RPLAY socket.  */
int other_rptp_fd = -1;		/* Another RPTP socket.  */
#endif
int curr_rate;			/* Current audio rate. */
#ifndef HAVE_OSS
int curr_bufsize;		/* Current audio bufsize. */
#endif /* !HAVE_OSS */
int rplayd_pid;			/* Process ID of rplayd.  */
time_t starttime;		/* The time in seconds that rplayd started. */
#ifdef AUTH
int auth_enabled = 1;		/* Host authentication.  (--auth, --no-auth) */
#endif /* AUTH */
int rplay_priority_threshold = RPLAY_DEFAULT_PRIORITY;	/* Default priority level allowed. */

/* Global audio-specific variables */
char *rplay_audio_device = RPLAY_AUDIO_DEVICE;	/* Which audio device to use. */
#ifndef HAVE_OSS
int rplay_audio_bufsize = 0;	/* --audio-bufsize */
#endif /* !HAVE_OSS */
int rplay_audio_rate = 0;	/* --audio-rate */
int rplay_audio_sample_rate = 0;	/* --audio-sample-rate */
int rplay_audio_channels = 0;	/* --audio-channels */
int rplay_audio_format = 0;	/* --audio-format */
int rplay_audio_precision = 0;	/* --audio-precison */
int rplay_audio_port = 0;	/* --audio_port */
int audio_enabled = 1;		/* --no-audio */
int rplay_audio_volume = RPLAY_DEFAULT_VOLUME;	/* Current audio volume.  */
int rplay_audio_match = 0;
RPLAY_AUDIO_TABLE *rplay_audio_table = NULL;

/* Optional audio parameters specified on the command line.  */
int optional_sample_rate = 0;
int optional_precision = 0;
int optional_channels = 0;
int optional_format = 0;
int optional_port = 0;

/* Default audio parameters.  */
int default_sample_rate = 0;
int default_precision = 0;
int default_channels = 0;
int default_format = 0;
int default_port = 0;

#ifdef HAVE_OSS
int rplay_audio_fragsize = 0;
int optional_fragsize = 0;
int default_fragsize = 0;
#endif /* HAVE_OSS */

/* The audio buffer and its current and maximum size.  */
int rplay_audio_size = 0;
char *rplay_audio_buf = NULL;
int max_rplay_audio_bufsize = 0;

/* Audio levels. */
int rplay_audio_left_level = 0;
int rplay_audio_right_level = 0;

int rplay_audio_timeout = RPLAY_AUDIO_TIMEOUT;
static int rplay_audio_flush_timeout = RPLAY_AUDIO_FLUSH_TIMEOUT;
static int rplayd_timeout = RPLAYD_TIMEOUT;	/* seconds */
static int sound_cleanup_timeout = 10;	/* seconds */
static int buffer_cleanup_timeout = 10;		/* seconds */
static int spool_cleanup_timeout = 0;	/* seconds -- disabled for now */
static int need_to_reset = 0;
static char *progname;
static char *sounds_file = NULL;
#ifdef AUTH
static char *hosts_file = NULL;
#endif /* AUTH */
static char *servers_file = NULL;
static char *cache_dir = NULL;
static char *log_file = NULL;
#ifdef HAVE_HELPERS
static char *helpers_file = NULL;
#endif /* HAVE_HELPERS */
static int logging = 0;
static int report_level;
static int log_fd = -1;
static int forward_fd = -1;
static char *forward = NULL;
static int rplay_port = 0;
static int rptp_port = 0;
#ifdef OTHER_RPLAY_PORTS
static int other_rplay_port = 0;
static int other_rptp_port = 0;
#endif /* OTHER_RPLAY_PORTS */
static int audio_test_mode = 0;	/* enable audio test mode */
static int do_fork = 1;
static int inetd = 0;		/* Was rplayd started by inetd? */

#ifndef RPLAYD_USER
#define RPLAYD_USER ""
#endif
#ifndef RPLAYD_GROUP
#define RPLAYD_GROUP ""
#endif
static char *run_as_user = RPLAYD_USER;
static char *run_as_group = RPLAYD_GROUP;

extern char *optarg;		/* getopt_long */
extern int optind;		/* getopt_long */

/*
 * Long options for getopt_long.
 */
static struct option longopts[] =
{
    {"audio-device", required_argument, NULL, 'A'},
#ifndef HAVE_OSS
    {"audio-bufsize", required_argument, NULL, 'b'},
#endif				/* !HAVE_OSS */
    {"audio-bits", required_argument, NULL, 'B'},
    {"audio-channels", required_argument, NULL, 1},
    {"audio-close", required_argument, NULL, 'c'},
    {"audio-flush", required_argument, NULL, 'F'},
    {"audio-format", required_argument, NULL, 3},
#ifdef HAVE_OSS
    {"audio-fragsize", required_argument, NULL, 18},
#endif				/* HAVE_OSS */
    {"audio-info", required_argument, NULL, 'i'},
    {"audio-info-ulaw", no_argument, NULL, 13},
    {"audio-match", no_argument, &rplay_audio_match, 1},
    {"audio-port", required_argument, NULL, 10},
    {"audio-rate", required_argument, NULL, 'r'},
    {"audio-sample-rate", required_argument, NULL, 'R'},
    {"audio-test", no_argument, &audio_test_mode, 1},
#ifdef AUTH
    {"auth", no_argument, &auth_enabled, 1},
#endif
    {"cache-directory", required_argument, NULL, 'D'},
    {"cache-remove", no_argument, NULL, 12},
    {"cache-size", required_argument, NULL, 's'},
#ifdef HAVE_CDROM
    {"cdrom0", required_argument, NULL, 14},
    {"cdrom1", required_argument, NULL, 15},
    {"cdrom2", required_argument, NULL, 16},
    {"cdrom3", required_argument, NULL, 17},
#endif				/* HAVE_CDROM */
    {"conf", required_argument, NULL, 'C'},
    {"connection-timeout", required_argument, NULL, 'T'},
    {"debug", no_argument, NULL, 'd'},
    {"fork", no_argument, &do_fork, 1},
    {"forward", required_argument, NULL, 'f'},
    {"group", required_argument, NULL, 21},
    {"help", no_argument, NULL, 2},
#ifdef HAVE_HELPERS
    {"helpers", required_argument, NULL, 20},
#endif				/* HAVE_HELPERS */
#ifdef AUTH
    {"hosts", required_argument, NULL, 'H'},
#endif
    {"inetd", no_argument, &inetd, 1},
    {"info", required_argument, NULL, 'i'},
    {"info-ulaw", no_argument, NULL, 13},
    {"log-file", required_argument, NULL, 'L'},
    {"log-level", required_argument, NULL, 'l'},
    {"memory-cache-size", required_argument, NULL, 4},
    {"memory-cache-sound-size", required_argument, NULL, 5},
    {"no-audio", no_argument, NULL, 'N'},
#ifdef AUTH
    {"no-auth", no_argument, &auth_enabled, 0},
#endif
    {"no-fork", no_argument, &do_fork, 0},
    {"no-inetd", no_argument, NULL, 'n'},
    {"option-file", required_argument, NULL, 11},
    {"options-file", required_argument, NULL, 11},
#ifdef OTHER_RPLAY_PORTS
    {"other-rplay-port", required_argument, NULL, 9},
    {"other-rptp-port", required_argument, NULL, 8},
#endif
    {"port", required_argument, NULL, 6},
    {"rplay-port", required_argument, NULL, 6},
    {"rptp-port", required_argument, NULL, 7},
    {"servers", required_argument, NULL, 'S'},
    {"timeout", required_argument, NULL, 't'},
    {"user", required_argument, NULL, 22},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

#ifdef sun
#ifdef __STDC__
extern int gethostname(char *name, int namelen);
#else
extern int gethostname( /* char *name, int namelen */ );
#endif
#endif

extern char *ctime();
#ifdef __STDC__
static void do_option_file(char *option_file);
static void do_option(int option_value);
#else
static void do_option_file( /* char *option_file */ );
static void do_option( /* int option_value */ );
#endif
static void doit();
static void handle_signals();
static void handle_sighup();
static void handle_sigint();
static void handle_sigchld();
static void reset();
static void audio_test();

#if defined(HAVE_CDROM) || defined(HAVE_HELPERS)
#ifdef __STDC__
void rplayd_pipe_read( /* fd_set *rfds */ );
#else
void rplayd_pipe_read( /* fd_set *rfds */ );
#endif
#endif

#ifdef AUTH
#define OPTIONS "A:C:D:F:S:b:B:c:df:l:L:Nnr:R:s:t:T:vi:H:"
#else
#define OPTIONS "A:C:D:F:S:b:B:c:df:l:L:Nnr:R:s:t:T:vi:"
#endif

#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc, argv)
    int argc;
    char **argv;
#endif
{
    int c;
    struct hostent *hp;
    struct in_addr addr;
    char *rplaydrc;

    starttime = time(0);

    progname = argv[0];

    servers_file = tilde_expand(RPLAY_SERVERS);
#ifdef HAVE_HELPERS
    helpers_file = tilde_expand(RPLAY_HELPERS);
#endif /* HAVE_HELPERS */
    sounds_file = tilde_expand(RPLAY_CONF);
#ifdef AUTH
    hosts_file = tilde_expand(RPLAY_HOSTS);
#endif /* AUTH */
    cache_dir = tilde_expand(RPLAY_CACHE);
    log_file = tilde_expand(RPLAY_LOG);

#ifdef RPLAYD_ALWAYS_LOG
    report_level = RPLAYD_LOG_LEVEL;
    if (report_level != REPORT_NONE)
    {
	logging++;
    }
#else
    report_level = REPORT_ERROR;
#endif

    /* Parse RPLAYDRC. */
    rplaydrc = tilde_expand(RPLAYDRC);
    if (rplaydrc)
    {
	struct stat st;

	if (stat(rplaydrc, &st) == 0 && S_ISREG(st.st_mode))
	{
	    do_option_file(rplaydrc);
	}

	free(rplaydrc);
    }

    /* Parse command line options. */
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, 0)) != -1)
    {
	do_option(c);
    }

    if (audio_test_mode)
    {
	report_level = 0;	/* don't want debug output */
	audio_test();
	done(0);
    }

    if (run_as_group && *run_as_group)
    {
	struct group *gr = getgrnam(run_as_group);
	if (!gr)
	{
	    fprintf(stderr, "Unknown group `%s'.\n", run_as_group);
	    done(1);
	}
	if (setgid(gr->gr_gid) < 0)
	{
	    report(REPORT_ERROR, "setgid: %s\n", sys_err_str(errno));
	    done(1);
	}
	report(REPORT_DEBUG, "running as group %s (%d)\n", run_as_group, gr->gr_gid);
    }
    if (run_as_user && *run_as_user)
    {
	struct passwd *pw = getpwnam(run_as_user);
	if (!pw)
	{
	    fprintf(stderr, "Unknown user `%s'.\n", run_as_user);
	    done(1);
	}
	if (setuid(pw->pw_uid) < 0)
	{
	    report(REPORT_ERROR, "setuid: %s\n", sys_err_str(errno));
	    done(1);
	}
	report(REPORT_DEBUG, "running as user %s (%d)\n", run_as_user, pw->pw_uid);
    }

    if (debug)
    {
	logging = 0;
	do_fork = 0;
    }

    /* Fork */
    if (do_fork)
    {
	int pid = fork();
	if (pid != 0)
	{
	    exit(0);
	}
    }

    if (gethostname(hostname, sizeof(hostname)) < 0)
    {
	report(REPORT_ERROR, "gethostname: %s\n", sys_err_str(errno));
	done(1);
    }

    hp = gethostbyname(hostname);
    if (hp == NULL)
    {
	report(REPORT_ERROR, "gethostbyname: cannot resolve hostname: %s\n", hostname);
	done(1);
    }
    memcpy((char *) &addr, (char *) hp->h_addr, hp->h_length);
    hostaddr = strdup(inet_ntoa(addr));

    rplayd_pid = getpid();

    handle_signals();

    /* Call all the initialization routines. */
    rplayd_init();
#ifdef HAVE_HELPERS
    helper_read(helpers_file);
#endif /* HAVE_HELPERS */
    sound_read(sounds_file);
    cache_init(cache_dir);
    cache_read();
#ifdef AUTH
    host_read(hosts_file);
#endif /* AUTH */
    server_read(servers_file);
    spool_init();
    timer_init();
    if (rplayd_audio_init() < 0)
    {
	/* Disable audio.  */
	audio_enabled = 0;
	report(REPORT_DEBUG, "audio disabled\n");
    }
    else
    {
	/* Save the initial audio configuration.  */
	default_sample_rate = rplay_audio_sample_rate;
	default_precision = rplay_audio_precision;
	default_channels = rplay_audio_channels;
	default_format = rplay_audio_format;
	default_port = rplay_audio_port;

	/* Close audio device to avoid hogging it. */
	rplay_audio_close();
    }

#ifdef AUTH
    report(REPORT_DEBUG, "authentication %s\n", auth_enabled ? "enabled" : "disabled");
#else
    report(REPORT_DEBUG, "no authentication\n");
#endif

    if (forward)
    {
	report(REPORT_DEBUG, "forwarding sounds to %s\n", forward);
    }

    report(REPORT_DEBUG, "%s rplayd %s ready.\n", hostname, RPLAY_VERSION);

    doit();
}

#ifdef __STDC__
static void
do_option_file(char *option_file)
#else
static void
do_option_file(option_file)
    char *option_file;
#endif
{
    int first, c;
    char buf[BUFSIZ], *p;
    FILE *fp;
    int saved_optind;
    char *argv[1024];		/* that should be enough */
    int argc = 0;

    fp = fopen(option_file, "r");
    if (fp == NULL)
    {
	report(REPORT_ERROR, "cannot open `%s'.\n", option_file);
	return;
    }

    argv[argc++] = progname;

    tilde_additional_prefixes = (char **) xmalloc(2 * sizeof(char *));
    tilde_additional_prefixes[0] = "=~";
    tilde_additional_prefixes[1] = (char *) NULL;

    while (fgets(buf, sizeof(buf), fp))
    {
	switch (buf[0])
	{
	case '#':
	case ' ':
	case '\n':
	case '\t':
	    continue;
	}

	first = 1;
	while (p = (char *) strtok(first ? buf : NULL, " \t\n"))
	{
	    first = 0;
	    argv[argc++] = tilde_expand(p);
	}
    }

    fclose(fp);
    free((char *) tilde_additional_prefixes);
    tilde_additional_prefixes = NULL;
    argv[argc] = NULL;

    saved_optind = optind;	/* save optind */
    optind = 1;			/* reset optind */
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, 0)) != -1)
    {
	do_option(c);
    }
    optind = saved_optind;	/* restore optind */
}


#ifdef __STDC__
static void
do_option(int option_value)
#else
static void
do_option(option_value)
    int option_value;
#endif
{
    static int within_file;
    int i;
    char *p;

    switch (option_value)
    {
    case 0:
	/* getopt has processed a long-named option -- do nothing */
	break;

    case 1:
	/* --audio-channels */

	optional_channels = atoi(optarg);
	if (optional_channels != 1 && optional_channels != 2)
	{
	    usage();
	    done(1);
	}
	break;

    case 2:
	/* --help */
	usage();
	done(1);

    case 3:
	/* --audio-format */
	optional_format = string_to_audio_format(optarg);
	if (optional_format < 0)
	{
	    usage();
	    done(1);
	}
	break;

    case 4:
	/* --memory-cache-size */
	sound_cache_max_size = atoi(optarg);
	break;

    case 5:
	/* --memory-cache-sound-size */
	sound_cache_max_sound_size = atoi(optarg);
	break;

    case 6:
	/* --port or --rplay-port */
	rplay_port = atoi(optarg);
	break;

    case 7:
	/* --rptp-port */
	rptp_port = atoi(optarg);
	break;

#ifdef OTHER_RPLAY_PORTS
    case 8:
	/* --other-rplay-port */
	other_rplay_port = atoi(optarg);
	break;

    case 9:
	/* --other-rptp-port */
	other_rptp_port = atoi(optarg);
	break;
#endif /* OTHER_RPTP_PORTS */

    case 10:
	/* --audio-port */
	if (strstr(optarg, "none"))
	{
	    SET_BIT(optional_port, RPLAY_AUDIO_PORT_NONE);
	}
	if (strstr(optarg, "speaker"))
	{
	    SET_BIT(optional_port, RPLAY_AUDIO_PORT_SPEAKER);
	}
	if (strstr(optarg, "headphone"))
	{
	    SET_BIT(optional_port, RPLAY_AUDIO_PORT_HEADPHONE);
	}
	if (strstr(optarg, "lineout"))
	{
	    SET_BIT(optional_port, RPLAY_AUDIO_PORT_LINEOUT);
	}
	break;

    case 11:
	do_option_file(optarg);
	break;

    case 12:			/* --cache-remove */
	cache_remove++;
	break;

    case 13:			/* --audio-info-ulaw, --info-ulaw */
	optional_format = RPLAY_FORMAT_ULAW;
	optional_sample_rate = 8000;
	optional_precision = 8;
	optional_channels = 1;
	break;

#ifdef HAVE_CDROM
    case 14:			/* cdrom0 */
	for (i = 0; i < MAX_CDROMS; i++)
	{
	    if (strncmp(cdrom_table[i].name, "cdrom0", 6) == 0)
	    {
		cdrom_table[i].device = optarg;
		break;
	    }
	}
	break;

    case 15:			/* cdrom1 */
	for (i = 0; i < MAX_CDROMS; i++)
	{
	    if (strncmp(cdrom_table[i].name, "cdrom1", 6) == 0)
	    {
		cdrom_table[i].device = optarg;
		break;
	    }
	}
	break;

    case 16:			/* cdrom2 */
	for (i = 0; i < MAX_CDROMS; i++)
	{
	    if (strncmp(cdrom_table[i].name, "cdrom2", 6) == 0)
	    {
		cdrom_table[i].device = optarg;
		break;
	    }
	}
	break;

    case 17:			/* cdrom3 */
	for (i = 0; i < MAX_CDROMS; i++)
	{
	    if (strncmp(cdrom_table[i].name, "cdrom3", 6) == 0)
	    {
		cdrom_table[i].device = optarg;
		break;
	    }
	}
	break;
#endif /* HAVE_CDROM */

#ifdef HAVE_OSS
    case 18:			/* audio-fragsize */
	optional_fragsize = atoi(optarg);
	break;
#endif /* HAVE_OSS */

    case 19:			/* fork */
	do_fork++;
	break;

    case 20:			/* helpers */
	helpers_file = optarg;
	break;

    case 21:			/* group */
	run_as_group = optarg;
	break;

    case 22:			/* user */
	run_as_user = optarg;
	break;

    case 'A':
	rplay_audio_device = optarg;
	break;

#ifndef HAVE_OSS
    case 'b':
	rplay_audio_bufsize = atoi(optarg);
	break;
#endif /* !HAVE_OSS */

    case 'B':
	optional_precision = atoi(optarg);
	if (optional_precision != 8 && optional_precision != 16)
	{
	    usage();
	    done(1);
	}
	break;

    case 'c':
	rplay_audio_timeout = atoi(optarg);
	break;

    case 'C':
	sounds_file = optarg;
	break;

    case 'd':
	debug++;
	report_level = REPORT_DEBUG;
	break;

    case 'D':
	cache_dir = optarg;
	break;

    case 'f':
	forward = optarg;
	break;

    case 'F':
	rplay_audio_flush_timeout = atoi(optarg);
	break;

#ifdef AUTH
    case 'H':
	hosts_file = optarg;
	break;
#endif /* AUTH */

    case 'i':			/* --audio-info, --info */
	/* Example: ulaw,8000,8,1 */
	p = strtok(optarg, ", ");
	if (p)
	    optional_format = string_to_audio_format(p);
	p = strtok(NULL, ",");
	if (p)
	    optional_sample_rate = atoi(p);
	p = strtok(NULL, ",");
	if (p)
	    optional_precision = atoi(p);
	p = strtok(NULL, ",");
	if (p)
	    optional_channels = atoi(p);
	break;

    case 'l':
	logging++;
	report_level = atoi(optarg);
	break;

    case 'L':
	log_file = optarg;
	break;

    case 'n':
	inetd = 0;
	break;

    case 'N':
	audio_enabled = 0;
	break;

    case 'r':
	rplay_audio_rate = atoi(optarg);
	break;

    case 'R':
	optional_sample_rate = atoi(optarg);
	break;

    case 's':
	cache_max_size = atoi(optarg);
	break;

    case 'S':
	servers_file = optarg;
	break;

    case 't':
	rplayd_timeout = atoi(optarg);
	break;

    case 'T':
	rptp_timeout = atoi(optarg);
	break;

    case 'v':
	printf("rplay %s\n", RPLAY_VERSION);
	done(0);
	break;

    default:
	fprintf(stderr, "Try `%s --help' for more information.\n", progname);
	done(1);
    }
}

/*
 * The main loop of rplayd.
 */
static void
doit()
{
    int nfds, icount = 0;
    int idle = 1;
    struct timeval select_timeout;
    fd_set rfds, wfds;
    SPOOL *sp;
    /* Initialize the fd_sets before starting.  */
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&read_mask);
    FD_ZERO(&write_mask);
    FD_SET(rplay_fd, &read_mask);
    FD_SET(rptp_fd, &read_mask);
#ifdef OTHER_RPLAY_PORTS
    FD_SET(other_rplay_fd, &read_mask);
    FD_SET(other_rptp_fd, &read_mask);
#endif

    for (;;)
    {
	if (need_to_reset)
	{
	    reset();
	    need_to_reset = 0;
	}

	if (audio_enabled)
	{
	    if (timer_enabled)
	    {
		if (!spool_nplaying)
		{
		    timer_stop();

		    rplay_audio_size = 0;
		    if (rplay_audio_flush_timeout == -1)
		    {
			report(REPORT_DEBUG, "flushing %s\n", rplay_audio_device);
			rplay_audio_flush();
		    }

		    /* Notify the final zero levels. */
		    rplay_audio_left_level = 0;
		    rplay_audio_right_level = 0;
		    if (connection_want_level_notify)
		    {
			connection_notify(0, NOTIFY_LEVEL, 1);	/* force notification */
		    }
		}
		else
		{
		    /* keep trying to open */
		    if (!rplay_audio_isopen())
		    {
			rplay_audio_open();
		    }

		    if (connection_level_notify)
		    {
			connection_notify(0, NOTIFY_LEVEL, 0);
			connection_level_notify = 0;
		    }

		    for (sp = spool; sp; sp = sp->next)
		    {
			if (sp->notify_position)
			{
			    connection_notify(0, NOTIFY_POSITION, sp);
			    sp->notify_position = 0;
			}
		    }
		}
	    }
	    else if (spool_nplaying)
	    {
		if (!rplay_audio_isopen())
		{
		    report(REPORT_DEBUG, "opening %s\n", rplay_audio_device);
		    if (rplay_audio_open() < 0)
		    {
			report(REPORT_ERROR, "rplay_audio_open: %s: %s (will keep trying)\n",
			       rplay_audio_device, sys_err_str(errno));
		    }
		}
		/* start the time even if the audio device isn't open */
		timer_start(curr_rate);
	    }

	    if (spool_needs_update)
	    {
		spool_update();
	    }
	}

	if (connections)
	{
	    connection_check_timeout();
	}

	rfds = read_mask;
	wfds = write_mask;

	if (idle)
	{
	    report(REPORT_DEBUG, "entering idle mode\n");
	    select_timeout.tv_sec = rplayd_timeout;
	    select_timeout.tv_usec = 0;
#ifdef __hpux
	    nfds = select(FD_SETSIZE, (int *) &rfds, (int *) &wfds, 0,
			  rplayd_timeout ? &select_timeout : 0);
#else
	    nfds = select(FD_SETSIZE, &rfds, &wfds, 0,
			  rplayd_timeout ? &select_timeout : 0);
#endif
	    if (nfds == 0)
	    {
		done(0);
	    }

#ifdef AUTO_REREAD
#ifdef HAVE_HELPERS
	    helper_stat(helpers_file);
#endif /* HAVE_HELPERS */
	    sound_stat(sounds_file);
#ifdef AUTH
	    host_stat(hosts_file);
#endif /* AUTH */
	    server_stat(servers_file);
#endif /* AUTO_REREAD */

	    idle = 0;
	    icount = 0;
	}
	else
	{
	    select_timeout.tv_sec = 1;
	    select_timeout.tv_usec = 0;
#ifdef __hpux
	    nfds = select(FD_SETSIZE, (int *) &rfds, (int *) &wfds, 0,
			  &select_timeout);
#else
	    nfds = select(FD_SETSIZE, &rfds, &wfds, 0,
			  &select_timeout);
#endif
	}

	switch (nfds)
	{
	case -1:
	    if (errno != EINTR && errno != EAGAIN)
	    {
		report(REPORT_ERROR, "select: %s\n", sys_err_str(errno));
		done(1);
	    }
	    break;

	case 0:
	    icount++;

	    /*
	     * Timeouts that occur when no sounds are playing.
	     */
	    if (!spool_nplaying && !rplay_audio_size)
	    {
		if (audio_enabled && rplay_audio_timeout && icount == rplay_audio_timeout
		    && rplay_audio_isopen())
		{
		    report(REPORT_DEBUG, "closing %s\n", rplay_audio_device);
		    rplay_audio_close();
		}
		if (audio_enabled && rplay_audio_flush_timeout > 0
		    && icount == rplay_audio_flush_timeout && rplay_audio_isopen())
		{
		    report(REPORT_DEBUG, "flushing %s\n", rplay_audio_device);
		    rplay_audio_flush();
		}
		if (sound_cleanup_timeout && icount == sound_cleanup_timeout)
		{
		    sound_cleanup();
		}
		if (buffer_cleanup_timeout && icount == buffer_cleanup_timeout)
		{
		    buffer_cleanup();
		}
		if (spool_cleanup_timeout && icount == spool_cleanup_timeout)
		{
		    spool_cleanup();
		}

		/*
		 * See if rplayd should enter idle mode.
		 * Idle mode should only be entered when *nothing*
		 * is happening.
		 */
		if (icount >= rplay_audio_timeout
		    && connection_idle()
		    && icount >= rplay_audio_flush_timeout
		    && icount >= sound_cleanup_timeout
		    && icount >= buffer_cleanup_timeout
		    && icount >= spool_cleanup_timeout)
		{
		    idle = 1;
		}
	    }
	    break;

	default:
	    if (FD_ISSET(rplay_fd, &rfds))
	    {
		rplayd_read(rplay_fd);
	    }
#ifdef OTHER_RPLAY_PORTS
	    if (FD_ISSET(other_rplay_fd, &rfds))
	    {
		rplayd_read(other_rplay_fd);
	    }
#endif
	    connection_update(&rfds, &wfds);
#if defined(HAVE_CDROM) || defined(HAVE_HELPERS)
	    rplayd_pipe_read(&rfds);
#endif
	    icount = 0;
	    break;
	}
    }
}


/*
 * Set up the sockets to receive RPLAY packets and RPTP connections.
 */
void
rplayd_init()
{
    struct sockaddr_in s;
    struct servent *sp;

    if (inetd)
    {
	rplay_fd = 0;
#ifdef OTHER_RPLAY_PORTS
	/* Try to figure out which port is being used by inetd.  */
	sp = getservbyname("rplay", "udp");
	if (sp == NULL)
	{
	    report(REPORT_ERROR, "can't find rplay/udp service\n");
	    done(1);
	}

	if (ntohs(sp->s_port) == RPLAY_PORT)
	{
	    other_rplay_fd = udp_socket(other_rplay_port ? other_rplay_port : OLD_RPLAY_PORT);
	}
	else
	{
	    other_rplay_fd = udp_socket(other_rplay_port ? other_rplay_port : RPLAY_PORT);
	}
#endif
    }
    else
    {
	rplay_fd = udp_socket(rplay_port ? rplay_port : RPLAY_PORT);
#ifdef OTHER_RPLAY_PORTS
	other_rplay_fd = udp_socket(other_rplay_port ? other_rplay_port : OLD_RPLAY_PORT);
#endif
    }

    /*
     * Make the RPLAY sockets non-blocking.
     */
    fd_nonblock(rplay_fd);
#ifdef OTHER_RPLAY_PORTS
    fd_nonblock(other_rplay_fd);
#endif

    if (forward)
    {
	unsigned long addr = inet_addr(forward);

	memset((char *) &s, 0, sizeof(s));
	s.sin_family = AF_INET;
	s.sin_port = htons(RPLAY_PORT);

	if (addr == 0xffffffff)
	{
	    struct hostent *hp = gethostbyname(forward);
	    if (hp == NULL)
	    {
		report(REPORT_ERROR, "gethostbyname: %s unknown host\n", forward);
		done(1);
	    }
	    memcpy((char *) &s.sin_addr.s_addr, (char *) hp->h_addr, hp->h_length);
	}
	else
	{
	    memcpy((char *) &s.sin_addr.s_addr, (char *) &addr, sizeof(addr));
	}

	forward_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (forward_fd < 0)
	{
	    report(REPORT_ERROR, "socket: %s\n", sys_err_str(errno));
	    done(1);
	}

	if (connect(forward_fd, (struct sockaddr *) &s, sizeof(s)) < 0)
	{
	    report(REPORT_ERROR, "connect: %s\n", sys_err_str(errno));
	    done(1);
	}
    }

    rptp_fd = tcp_socket(rptp_port ? rptp_port : RPTP_PORT);
#ifdef OTHER_RPLAY_PORTS
    other_rptp_fd = tcp_socket(other_rptp_port ? other_rptp_port : OLD_RPTP_PORT);
#endif

    /*
     * Make the RPTP sockets non-blocking.
     */
    fd_nonblock(rptp_fd);
#ifdef OTHER_RPLAY_PORTS
    fd_nonblock(other_rptp_fd);
#endif
}

int
rplayd_audio_init()
{
    int sample_size;

    if (!audio_enabled)
    {
	return -1;
    }

    if (optional_sample_rate && optional_sample_rate > MAX_SAMPLE_RATE)
    {
	optional_sample_rate = MAX_SAMPLE_RATE;
    }

    /* Call rplay_audio_init() which sets all the rplay_audio variables.  */
    rplay_audio_table = NULL;	/* will be set by rplay_audio_init() */
    if (rplay_audio_init() < 0)
    {
	return -1;
    }

    /* XXX */
    optional_sample_rate = rplay_audio_sample_rate;
    optional_precision = rplay_audio_precision;
    optional_channels = rplay_audio_channels;
    optional_format = rplay_audio_format;
    optional_port = rplay_audio_port;

    sample_size = (rplay_audio_precision >> 3) * rplay_audio_channels;

    if (rplay_audio_match || rplay_audio_rate == 0)
    {
	rplay_audio_rate = RPLAY_AUDIO_RATE;
    }
    curr_rate = rplay_audio_rate;

#ifndef HAVE_OSS
    /* Click prevention:  Make sure curr_rate won't split up samples.
       If it will, it's easiest right now to modify the rate and not
       the sample_rate.  */

    if (rplay_audio_sample_rate % curr_rate)
    {
	while (rplay_audio_sample_rate % curr_rate)
	{
	    curr_rate--;	/* Slow it down. */
	}

	report(REPORT_DEBUG, "changed curr_rate from %d to %d\n",
	       rplay_audio_rate, curr_rate);
    }

    if (rplay_audio_match || rplay_audio_bufsize == 0)
    {
	rplay_audio_bufsize = rplay_audio_sample_rate / curr_rate * sample_size;
    }
    curr_bufsize = rplay_audio_bufsize;

    /* More click prevention:  Make sure curr_bufsize won't split up samples.
       If it will, curr_bufsize should be increased, not decreased.  */

    if (curr_bufsize % sample_size)
    {
	while (curr_bufsize % sample_size)
	{
	    curr_bufsize++;	/* Add a little. */
	}

	report(REPORT_DEBUG, "changed curr_bufsize from %d to %d\n",
	       rplay_audio_bufsize, curr_bufsize);
    }
#endif /* !HAVE_OSS */

    /*
     * Create the audio buffer which holds at most 1 second of 16-bit
     * stereo audio data sampled at MAX_SAMPLE_RATE Hz.
     */
    max_rplay_audio_bufsize = MAX_SAMPLE_RATE * 2 /* 16-bit */  * 2 /* stereo */ ;
    if (rplay_audio_buf)
    {
	free((char *) rplay_audio_buf);
    }
    rplay_audio_buf = (char *) malloc(max_rplay_audio_bufsize);
    if (rplay_audio_buf == NULL)
    {
	report(REPORT_ERROR, "cannot allocate rplay_audio_buf, size=%d\n",
	       max_rplay_audio_bufsize);
	done(1);
    }

    /*
     * Update the volume of the audio device.
     */
    rplay_audio_volume = rplay_audio_get_volume();
    if (rplay_audio_volume < 0)
    {
	rplay_audio_volume = 0;
    }

    report(REPORT_DEBUG, "\
audio: bits=%d sample-rate=%d channels=%d format=%s volume=%d port-mask=%0x\n",
       rplay_audio_precision, rplay_audio_sample_rate, rplay_audio_channels,
	   audio_format_to_string(rplay_audio_format), rplay_audio_volume, rplay_audio_port);

#ifdef HAVE_OSS
    report(REPORT_DEBUG, "timer: rate=%d, min bytes/sec=%d\n", curr_rate,
	   (rplay_audio_sample_rate / curr_rate * sample_size));
#else
    report(REPORT_DEBUG, "timer: rate=%d, bufsize=%d\n", curr_rate, curr_bufsize);
#endif

    return 0;
}

/* Attempt to match the audio device configuration with the sound
   in `sp'.   Currenly only the sample-rate is matched. */
#ifdef __STDC__
void
rplayd_audio_match(SPOOL *match_sp)
#else
void
rplayd_audio_match(match_sp)
    SPOOL *match_sp;
#endif
{
    if (rplay_audio_match
	&& spool_nplaying == 0	/* Only is nothing else is playing. */
	&& match_sp->sample_rate != rplay_audio_sample_rate)
    {
	RPLAY_AUDIO_TABLE *t;
	SPOOL *sp;
	int prev_sample_rate;

	/* Find the closest matching table entry. */
	optional_sample_rate = match_sp->sample_rate;
	prev_sample_rate = rplay_audio_table ? rplay_audio_table->sample_rate : 0;
	for (t = rplay_audio_table; t; t++)
	{
	    if (t->sample_rate == 0
		|| optional_sample_rate < prev_sample_rate)
	    {
		optional_sample_rate = prev_sample_rate;
		break;
	    }
	    else if (optional_sample_rate >= prev_sample_rate
		     && optional_sample_rate <= t->sample_rate)
	    {
		if ((t->sample_rate - optional_sample_rate)
		    > (optional_sample_rate - prev_sample_rate))
		{
		    optional_sample_rate = prev_sample_rate;
		}
		else
		{
		    optional_sample_rate = t->sample_rate;
		}
		break;
	    }
	    prev_sample_rate = t->sample_rate;
	}

	report(REPORT_DEBUG, "matched %d with %d\n", match_sp->sample_rate,
	       optional_sample_rate);

	if (rplayd_audio_init() < 0)
	{
	    /* That sample rate didn't work -- Use the default instead.  */
	    optional_sample_rate = default_sample_rate;
	    rplayd_audio_init();
	}
	if (timer_enabled)
	{
	    timer_restart(curr_rate);
	}

	/* Update the spool entries with the new sample-rate. */
	for (sp = spool; sp; sp = sp->next)
	{
	    sp->sample_factor = (double) sp->sample_rate / (double) rplay_audio_sample_rate;
	}
    }
}

/*
 * Write nbytes of audio data from the audio buffer to the audio device.
 */
#ifdef __STDC__
void
rplayd_write(int nbytes)
#else
void
rplayd_write(nbytes)
    int nbytes;
#endif
{
    int nwritten = 0;

    /* Insert `nbytes' of audio data from the spool into `rplay_audio_buf'.  */
    spool_process(rplay_audio_buf, nbytes);

    /*
     * Write the audio data if the audio device is open.
     * Otherwise forget about the data.
     */
    if (rplay_audio_size && rplay_audio_isopen())
    {
	nwritten = rplay_audio_write(rplay_audio_buf, rplay_audio_size);
#if 0
	report(REPORT_DEBUG, "audio_write: %d -> %d\n", rplay_audio_size, nwritten);
#endif
	/* Flush after every write?  */
	if (rplay_audio_flush_timeout == -2)
	{
	    report(REPORT_DEBUG, "flushing %s\n", rplay_audio_device);
	    rplay_audio_flush();
	}
    }
    rplay_audio_size = 0;

    /* Flush when the audio stream has ended.  */
    if (spool_nplaying == 0 && rplay_audio_size == 0)
    {
	if (rplay_audio_flush_timeout == -1)
	{
	    report(REPORT_DEBUG, "flushing %s\n", rplay_audio_device);
	    rplay_audio_flush();
	}
    }
}

/*
 * Read RPLAY packets from the UDP socket.
 */
#ifdef __STDC__
void
rplayd_read(int fd)
#else
void
rplayd_read(fd)
    int fd;
#endif
{
    static char recv_buf[MAX_PACKET];
    char *packet;
    struct sockaddr_in f;
    int j, n, flen;
    RPLAY *rp;

    for (j = 0; j < SPOOL_SIZE; j++)
    {
	flen = sizeof(f);

	n = recvfrom(fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *) &f, &flen);
	if (n <= 0)
	{
	    continue;
	}

	if (forward)
	{
	    write(forward_fd, recv_buf, n);
	    continue;
	}

#ifdef AUTH
	if (!host_access(f, HOST_EXECUTE))
	{
	    if (!host_access(f, HOST_READ) && !host_access(f, HOST_WRITE))
	    {
		report(REPORT_NOTICE, "%s permission denied\n", inet_ntoa(f.sin_addr));
	    }
	    continue;
	}
#endif /* AUTH */

	packet = recv_buf;

	switch (*packet)
	{
#ifdef OLD_RPLAY
	case OLD_RPLAY_PLAY:
	case OLD_RPLAY_STOP:
	case OLD_RPLAY_PAUSE:
	case OLD_RPLAY_CONTINUE:
	    packet = rplay_convert(recv_buf);
#endif /* OLD_RPLAY */

	case RPLAY_PACKET_ID:
	    rp = rplay_unpack(packet);
	    if (rp == NULL)
	    {
		report(REPORT_ERROR, "rplay_unpack: %s\n", rplay_errlist[rplay_errno]);
		break;
	    }

	    switch (rp->command)
	    {
	    case RPLAY_PING:
#ifdef DEBUG
		report(REPORT_DEBUG, "received a ping packet\n");
#endif
		rplay_destroy(rp);
		break;

	    case RPLAY_PLAY:
		report(REPORT_INFO, "%s play %s\n",
		       inet_ntoa(f.sin_addr),
		       (char *) rplay_get(rp, RPLAY_SOUND, 0));
		rplayd_play(rp, f);
		break;

	    case RPLAY_STOP:
		report(REPORT_INFO, "%s stop %s\n",
		       inet_ntoa(f.sin_addr),
		       (char *) rplay_get(rp, RPLAY_SOUND, 0));
		rplayd_stop(rp, f);
		break;

	    case RPLAY_PAUSE:
		report(REPORT_INFO, "%s pause %s\n",
		       inet_ntoa(f.sin_addr),
		       (char *) rplay_get(rp, RPLAY_SOUND, 0));
		rplayd_pause(rp, f);
		break;

	    case RPLAY_CONTINUE:
		report(REPORT_INFO, "%s continue %s\n",
		       inet_ntoa(f.sin_addr),
		       (char *) rplay_get(rp, RPLAY_SOUND, 0));
		rplayd_continue(rp, f);
		break;

	    case RPLAY_RESET:
		report(REPORT_INFO, "received a reset packet\n");
		need_reset();
		rplay_destroy(rp);
		break;

	    case RPLAY_DONE:
		report(REPORT_INFO, "%s done %s\n",
		       inet_ntoa(f.sin_addr),
		       (char *) rplay_get(rp, RPLAY_SOUND, 0));
		rplayd_done(rp, f);
		break;

	    case RPLAY_PUT:
		report(REPORT_INFO, "%s put id=%d sequence=%d size=%d\n",
		       inet_ntoa(f.sin_addr),
		       rp->id,
		       rp->sequence,
		       rp->data_size);
		rplayd_put(rp, f);
		break;
	    }
	    break;

	default:
	    report(REPORT_ERROR, "unknown RPLAY packet received from %s\n",
		   inet_ntoa(f.sin_addr));
	    break;
	}
    }
}

#if defined(HAVE_CDROM) || defined(HAVE_HELPERS)
/* Read data from piped devices and insert it into the spool entry.
   This is done using rplayd main select loop for efficiency. */
#ifdef __STDC__
void
rplayd_pipe_read(fd_set * rfds)
#else
void
rplayd_pipe_read(rfds)
    fd_set *rfds;
#endif
{
    SPOOL *sp, *sp_next;
    extern BUFFER *sound_pipe_read(SINDEX *si);

    for (sp = spool; sp; sp = sp_next)
    {
	sp_next = sp->next;
	if (!sp->si)
	{
	    continue;
	}

#ifdef HAVE_CDROM
	if (sp->si->sound->type == SOUND_CDROM && FD_ISSET(sp->si->fd, rfds))
	{
	    BUFFER *b = sound_pipe_read(sp->si);
	    if (b)
	    {
		spool_flow_insert(sp, b);
	    }
	}
#endif

#ifdef HAVE_HELPERS
	if (sp->si->sound->needs_helper && FD_ISSET(sp->si->fd, rfds))
	{
	    BUFFER *b = sound_pipe_read(sp->si);
	    if (b)
	    {
		spool_flow_insert(sp, b);
	    }
	}
#endif

    }
}
#endif

#ifdef __STDC__
int
rplayd_play(RPLAY *rp, struct sockaddr_in sin)
#else
int
rplayd_play(rp, sin)
    RPLAY *rp;
    struct sockaddr_in sin;
#endif
{
    SPOOL *sp = NULL;
    int i, n;
    int start_sound, end_sound, merged_lists;
    RPLAY_ATTRS *attrs;

    if (!audio_enabled)
    {
	return -1;
    }

    /* If a list_name is specified, try to find a matching list. */
    if (*rp->list_name)
    {
	for (sp = spool; sp; sp = sp->next)
	{
	    if (*sp->rp->list_name
		&& strcmp(rp->list_name, sp->rp->list_name) == 0)
	    {
		break;
	    }
	}
	if (sp)
	{
	    *(sp->rp->attrsp) = rp->attrs;	/* Append the new attrs. */
	    sp->rp->attrsp = rp->attrsp;
	    start_sound = sp->rp->nsounds;
	    sp->rp->nsounds += rp->nsounds;
	    end_sound = sp->rp->nsounds;
	    merged_lists = 1;
	}
    }

    /* Find a spool entry. */
    if (!sp)
    {
	/* Check the priority level.  rplayd will ignore all sounds
	   with priorities less than rplay_priority_threshold. */
	if (rp->priority < rplay_priority_threshold)
	{
	    report(REPORT_DEBUG, "sound at priority %d ignored\n",
		   rp->priority);
	    rplay_destroy(rp);
	    return -1;
	}

	sp = spool_next(rp->priority);
	if (!sp)
	{
	    report(REPORT_DEBUG, "spool full\n");
	    rplay_destroy(rp);
	    return -1;
	}
	start_sound = 0;
	end_sound = rp->nsounds;
	merged_lists = 0;
    }

    /* Gotta have attributes! */
    if (!rp->attrs)
    {
	report(REPORT_DEBUG, "`rp' missing attributes\n");
	rplay_destroy(rp);
	spool_destroy(sp);
	return -1;
    }

    /* Add the sounds to the spool entry. */
    n = 0;
    for (i = start_sound, attrs = rp->attrs;
	 i < end_sound && attrs;
	 i++, attrs = attrs->next)
    {
	if (attrs->rptp_server)
	{
	    SERVER *s;

	    s = (SERVER *) malloc(sizeof(SERVER));
	    s->next = NULL;
	    s->sin.sin_family = AF_INET;
	    s->sin.sin_port = htons(attrs->rptp_server_port);
	    s->sin.sin_addr.s_addr = inet_addr(attrs->rptp_server);
	    sp->sound[i] = sound_lookup(attrs->sound,
		      attrs->rptp_search ? SOUND_FIND : SOUND_DONT_FIND, s);
	}
	else
	{
	    sp->sound[i] = sound_lookup(attrs->sound,
		   attrs->rptp_search ? SOUND_FIND : SOUND_DONT_FIND, NULL);
	}

	if (sp->sound[i] == NULL)
	{
	    rplay_destroy(rp);
	    spool_destroy(sp);
	    return -1;
	}

	if (sp->sound[i]->status != SOUND_READY)
	{
	    n++;
	}
    }

    /* Set-up the spool entry only if it's a new sound list. */
    if (!merged_lists)
    {
	sp->rp = rp;
	sp->curr_attrs = rp->attrs;
	sp->curr_sound = 0;
	sp->curr_count = sp->curr_attrs->count;
	sp->list_count = rp->count;
	sp->sin = sin;

	if (n)
	{
	    sp->state = SPOOL_WAIT;
	}
	else
	{
	    spool_play(sp);
	}
    }

    return sp->id;
}

#ifdef __STDC__
int
rplayd_stop(RPLAY *rp, struct sockaddr_in sin)
#else
int
rplayd_stop(rp, sin)
    RPLAY *rp;
    struct sockaddr_in sin;
#endif
{
    int n;

    if (!audio_enabled)
    {
	rplay_destroy(rp);
	return -1;
    }

    n = spool_match(rp, spool_stop, sin);
    rplay_destroy(rp);

    return n > 0 ? 0 : -1;
}

#ifdef __STDC__
int
rplayd_pause(RPLAY *rp, struct sockaddr_in sin)
#else
int
rplayd_pause(rp, sin)
    RPLAY *rp;
    struct sockaddr_in sin;
#endif
{
    int n;

    if (!audio_enabled)
    {
	rplay_destroy(rp);
	return -1;
    }

    n = spool_match(rp, spool_pause, sin);
    rplay_destroy(rp);

    return n > 0 ? 0 : -1;
}

#ifdef __STDC__
int
rplayd_continue(RPLAY *rp, struct sockaddr_in sin)
#else
int
rplayd_continue(rp, sin)
    RPLAY *rp;
    struct sockaddr_in sin;
#endif
{
    int n;

    if (!audio_enabled)
    {
	rplay_destroy(rp);
	return -1;
    }

    n = spool_match(rp, spool_continue, sin);
    rplay_destroy(rp);

    return n > 0 ? 0 : -1;
}

#ifdef __STDC__
int
rplayd_done(RPLAY *rp, struct sockaddr_in sin)
#else
int
rplayd_done(rp, sin)
    RPLAY *rp;
    struct sockaddr_in sin;
#endif
{
    int n;

    if (!audio_enabled)
    {
	rplay_destroy(rp);
	return -1;
    }

    n = spool_match(rp, spool_done, sin);
    rplay_destroy(rp);

    return n > 0 ? 0 : -1;
}

#ifdef __STDC__
int
rplayd_put(RPLAY *rp, struct sockaddr_in sin)
#else
int
rplayd_put(rp, sin)
    RPLAY *rp;
    struct sockaddr_in sin;
#endif
{
    BUFFER *b;
    SPOOL *sp;

    if (!audio_enabled)
    {
	rplay_destroy(rp);
	return -1;
    }

    /* Find the spool entry. */
    sp = spool_find(rp->id);
    if (!sp)
    {
	report(REPORT_DEBUG, "put: `%d' no such spool id\n", rp->id);
	rplay_destroy(rp);
	return -1;
    }

    /* Create a buffer and insert the flow data. */
    b = buffer_create();
    b->nbytes = rp->data_size;
    memcpy(b->buf, rp->data, b->nbytes);
    spool_flow_insert(sp, b);

    rplay_destroy(rp);

    return 0;
}

BUFFER *
rplayd_status()
{
    BUFFER *b;

    b = buffer_create();

    b->buf[0] = RPTP_OK;
    b->buf[1] = '\0';

    SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), "host=%s", hostname);
    SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " version=%s", RPLAY_VERSION);
    SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " uptime=%s", time2string(time(0) - starttime));

    if (audio_enabled)
    {
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-bits=%d", rplay_audio_precision);
#ifndef HAVE_OSS
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-bufsize=%d", rplay_audio_bufsize);
#endif /* !HAVE_OSS */
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-byte-order=%s",
		 byte_order_to_string(RPLAY_AUDIO_BYTE_ORDER));
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-channels=%d", rplay_audio_channels);
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-device=%s", rplay_audio_device);
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-format=%s",
		 audio_format_to_string(rplay_audio_format));
#ifdef HAVE_OSS
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-fragsize=%d", rplay_audio_fragsize);
#endif /* HAVE_OSS */
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-port=%s",
		 audio_port_to_string(rplay_audio_port));
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-rate=%d", rplay_audio_rate);
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-sample-rate=%d", rplay_audio_sample_rate);
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " volume=%d", rplay_audio_volume);
#ifndef HAVE_OSS
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " curr-bufsize=%d", curr_bufsize);
#endif /* !HAVE_OSS */
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " curr-rate=%d", curr_rate);
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " priority-threshold=%d",
		 rplay_priority_threshold);
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-close=%d",
		 rplay_audio_timeout);
	SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), " audio-device-status=%s",
		 rplay_audio_isopen() ? "open" : "closed");
    }

    SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), "\r\n");

    b->nbytes = strlen(b->buf);

    return b;
}

void
need_reset()
{
    need_to_reset++;
}

/* Reset rplayd's state.  */
static void
reset()
{
    int was_enabled = timer_enabled;

    if (was_enabled)
    {
	timer_stop();
    }
    spool_init();
    connection_cleanup();
#ifdef HAVE_HELPERS
    helper_reread(helpers_file);
#endif /* HAVE_HELPERS */
    sound_reread(sounds_file);
    cache_init(cache_dir);
    cache_read();
#ifdef AUTH
    host_reread(hosts_file);
#endif /* AUTH */
    server_reread(servers_file);
    if (was_enabled)
    {
	timer_start(curr_rate);
    }
}

static void
handle_signals()
{
    /* Ingore SIGPIPE since read will handle closed TCP connections.
       Handle SIGHUP, SIGINT, and SIGCHLD. */
#ifdef HAVE_SIGSET
    sigset(SIGPIPE, SIG_IGN);
    sigset(SIGHUP, handle_sighup);
    sigset(SIGINT, handle_sigint);
    sigset(SIGCHLD, handle_sigchld);
#else
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, handle_sighup);
    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld);
#endif
}


static void
handle_sighup()
{
    report(REPORT_DEBUG, "received SIGHUP signal\n");
    need_reset();
}

static void
handle_sigint()
{
    report(REPORT_DEBUG, "received SIGINT signal\n");
    done(0);
}

static void
handle_sigchld()
{
    pid_t pid;
    SPOOL *sp;

#ifdef linux
    handle_signals();
#endif

    report(REPORT_DEBUG, "received SIGCHLD signal\n");

#ifdef HAVE_WAITPID
    pid = waitpid((pid_t) - 1, NULL, WNOHANG);
#else /* not HAVE_WAITPID */
    pid = wait(NULL);
#endif /* not HAVE_WAITPID */
    if (pid <= 0)
    {
	return;
    }
}

static void
audio_test()
{
    static char *spin = "|\\-/";
    static int spin_pos;

    printf("Testing supported audio configurations (this may take a long time)...\n");
    for (optional_sample_rate = 4000; optional_sample_rate <= 48000; optional_sample_rate += 5)
	for (optional_precision = 8; optional_precision <= 16; optional_precision += 8)
	    for (optional_channels = 1; optional_channels <= 2; optional_channels++)
		for (optional_format = 1; optional_format <= 5; optional_format++)
		{
		    if (!spin[spin_pos])
		    {
			spin_pos = 0;
		    }
		    fprintf(stderr, "%c\r", spin[spin_pos++]);

		    if (rplay_audio_init() >= 0)
		    {
			printf("  { %5d, %-24s, %2d, %d },\n",
			       rplay_audio_sample_rate,
			       rplay_audio_format == RPLAY_FORMAT_ULAW ? "RPLAY_FORMAT_ULAW" :
			       rplay_audio_format == RPLAY_FORMAT_LINEAR_8 ? "RPLAY_FORMAT_LINEAR_8" :
			       rplay_audio_format == RPLAY_FORMAT_ULINEAR_8 ? "RPLAY_FORMAT_ULINEAR_8" :
			       rplay_audio_format == RPLAY_FORMAT_LINEAR_16 ? "RPLAY_FORMAT_LINEAR_16" :
			       rplay_audio_format == RPLAY_FORMAT_ULINEAR_16 ? "RPLAY_FORMAT_ULINEAR_16" : "?",
			       rplay_audio_precision,
			       rplay_audio_channels);
		    }
		}
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
    SPOOL *sp;

/* #ifdef HAVE_CDROM */
/*     for (sp = spool; sp; sp = sp->next) */
/*     { */
/*      if (sp->si && sp->si->pid > 0) */
/*      { */
/*          report (REPORT_DEBUG, "killing process %d\n", sp->si->pid); */
/*          if (kill (sp->si->pid, SIGKILL) < 0) */
/*          { */
/*              report (REPORT_DEBUG, "kill %d: %s\n", sp->si->pid, sys_err_str (errno)); */
/*          } */
/*      } */
/*     } */
/* #endif */
/*     connection_cleanup (); */

    cache_cleanup();
    rplay_audio_close();
    close(rplay_fd);
    close(rptp_fd);
#ifdef OTHER_RPLAY_PORTS
    close(other_rplay_fd);
    close(other_rptp_fd);
#endif
    report(REPORT_DEBUG, "exit(%d)\n", exit_value);
    if (logging && log_fd >= 0)
    {
	close(log_fd);
    }
    exit(exit_value);
}

#ifdef __STDC__
void
report(int level, char *fmt,...)
#else
void
report(va_alist)
    va_dcl
#endif
{
    va_list args;
    char *p;
    time_t now;
    static struct iovec iov[2];
    static char header[256];
    static char message[2048];	/* XXX: largest message */

#ifdef __STDC__
    va_start(args, fmt);
#else
    char *fmt;
    int level;
    va_start(args);
    level = va_arg(args, int);
    fmt = va_arg(args, char *);
#endif

    if (report_level >= level)
    {
	VSNPRINTF(SIZE(message, sizeof(message)), fmt, args);
	iov[1].iov_base = message;
	iov[1].iov_len = strlen(message);

	if (logging)
	{
	    now = time(0);
	    p = ctime(&now);
	    p += 4;
	    p[15] = '\0';

	    if (log_fd < 0)
	    {
#ifndef O_SYNC
#define O_SYNC 0
#endif
		log_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND | O_SYNC, 0644);
		if (log_fd < 0)
		{
		    fprintf(stderr, "%s: cannot open logfile `%s': %s\n",
			    progname, log_file, sys_err_str(errno));
		    /*
		     * Turn off logging to avoid further problems.
		     */
		    logging = 0;
		    log_fd = -1;
		    return;
		}
	    }

	    SNPRINTF(SIZE(header, sizeof(header)), "%s %s rplayd[%d]: ", p, hostname, rplayd_pid);
	    iov[0].iov_base = header;
	    iov[0].iov_len = strlen(header);

	    if (writev(log_fd, iov, 2) < 0)
	    {
		fprintf(stderr, "%s: cannot write to log file `%s': %s\n",
			progname, log_file, sys_err_str(errno));
		logging = 0;
		log_fd = -1;
		fprintf(stderr, "%s: logging disabled\n", progname);
	    }
	}

	if (debug || level == REPORT_ERROR)
	{
	    SNPRINTF(SIZE(header, sizeof(header)), "%s: ", progname);
	    iov[0].iov_base = header;
	    iov[0].iov_len = strlen(header);
#ifndef STDERR_FILENO
#define STDERR_FILENO	2
#endif
	    writev(STDERR_FILENO, iov, 2);
	}

    }

    va_end(args);
}

void
usage()
{
    printf("\n");
    printf("rplay %s\n", RPLAY_VERSION);
    printf("\n");
    printf("usage: %s [options]\n", progname);
    printf("\n");

    printf("-A DEVICE, --audio-device=DEVICE\n");
    printf("\tUse DEVICE for the audio device (%s).\n", rplay_audio_device);
    printf("\n");

#ifndef HAVE_OSS
    printf("-b N, --audio-bufsize=N\n");
    printf("\tAudio buffer size (%d).\n", rplay_audio_bufsize);
    printf("\n");
#endif /* !HAVE_OSS */

    printf("-B N, --audio-bits=N\n");
    printf("\tAudio device bits per sample, 8 or 16.\n");
    printf("\n");

    printf("--audio-channels=N\n");
    printf("\tNumber of audio channels to use, 1 == mono, 2 == stereo.\n");
    printf("\n");

    printf("-c N, --audio-close=N\n");
    printf("\tClose %s after N idle seconds, disabled with 0 (%d).\n",
	   RPLAY_AUDIO_DEVICE, rplay_audio_timeout);
    printf("\n");

    printf("-F N, --audio_flush=N\n");
    printf("\tFlush %s after N idle seconds, disabled with 0 (%d).\n",
	   RPLAY_AUDIO_DEVICE, rplay_audio_flush_timeout);
    printf("\tN = -1 : flush when spool is empty.\n");
    printf("\tN = -2 : flush after each audio write. (not recommended)\n");
    printf("\tN should be <= to the audio close timeout.\n");
    printf("\n");

    printf("--audio-format=FORMAT\n");
    printf("\tTell rplayd to write audio data using FORMAT, where FORMAT\n");
    printf("\tcan be ulaw, linear-8, ulinear-8, linear-16, or ulinear-16.\n");
    printf("\t(linear = signed, ulinear = unsigned)\n");
    printf("\n");

#ifdef HAVE_OSS
    printf("--audio-fragsize=N\n");
    printf("\tAudio fragment size (%d).  The default size is zero which lets\n",
	   rplay_audio_fragsize);
    printf("\tthe audio driver pick the \"best\" size.  The size specified must\n");
    printf("\tbe a power of 2 greater than 16.  Example:  256, 1024, 4096.\n");
    printf("\n");
#endif /* HAVE_OSS */

    printf("--audio-info=INFO, --info=INFO, -i INFO\n");
    printf("\tSpecify complete audio device information with one option.\n");
    printf("\tINFO is of the form: format,sample-rate,bits,channels\n");
    printf("\tExamples: `ulaw,8000,8,1' and `linear-16,44100,16,2'\n");
    printf("\tAlso provided are:\n");
    printf("\t    --audio-info-ulaw, --info-ulaw -> ulaw,8000,8,1\n");
    printf("\n");

    printf("--audio-match\n");
    printf("\tAttempt to match the sample rate of the audio device with\n");
    printf("\tthe sample rate of the current sound when no other sounds\n");
    printf("\tare playing.  If the match fails, --audio-sample-rate is used.\n");
    printf("\tThis option overrides --audio-bufsize.\n");
    printf("\n");

    printf("--audio-port=PORT[,PORT...]\n");
    printf("\tOutput audio to the specified audio port(s).\n");
    printf("\tValid ports are `speaker', `headphone', and `lineout'.\n");
    printf("\tMultiple ports can be specified using `speaker,headphone,lineout'\n");
    printf("\n");

    printf("-r N, --audio-rate=N\n");
    printf("\tWrite the audio buffer N times per second (%d).\n", rplay_audio_rate);
    printf("\n");

    printf("-R N, --audio-sample-rate=N\n");
    printf("\tSample rate of the audio device.\n");
    printf("\n");

#ifdef AUTH
    printf("--auth\n");
    printf("\tEnable host access authentication.\n");
    printf("\n");
#endif

    printf("-D DIR, --cache-directory=DIR\n");
    printf("\tUse DIR for rplay.cache (%s).\n", RPLAY_CACHE);
    printf("\n");

    printf("--cache-remove\n");
    printf("\tRemove the cache directory and all its contents when rplayd exists.\n");
    printf("\n");

    printf("-s N, --cache-size=N\n");
    printf("\tMaximum size in bytes of the rplay cache, disabled with 0 (%d).\n",
	   RPLAY_CACHE_SIZE);
    printf("\n");

#ifdef HAVE_CDROM
    printf("--cdrom0=DEVICE, --cdrom1=DEVICE, --cdrom2=DEVICE, --cdrom3=DEVICE\n");
    printf("\tSpecify the cdrom[0-3] to DEVICE mapping.  For Solaris 2.x the default\n");
    printf("\tmapping is cdrom[0-3] -> /vol/dev/aliases/cdrom[0-3].\n");
    printf("\tLinux uses cdrom[0-3] -> /dev/cdrom[0-3].\n");
    printf("\n");
#endif /* HAVE_CDROM */

    printf("-C FILE, --conf=FILE\n");
    printf("\tUse FILE for rplay.conf (%s).\n", RPLAY_CONF);
    printf("\n");

    printf("-T N, --connection-timeout=N\n");
    printf("\tClose idle RPTP connections after N seconds, disabled with 0 (%d).\n",
	   rptp_timeout);
    printf("\n");

    printf("-d, --debug\n");
    printf("\tEnable debug mode.\n");
    printf("\n");

    printf("-f HOST, --forward=HOST\n");
    printf("\tForward all RPLAY packets to HOST.\n");
    printf("\n");

    printf("--fork\n");
    printf("\tEnable backgrounding rplayd at startup. (%s)\n",
	   do_fork ? "enabled" : "disabled");
    printf("\n");

    printf("--group=GROUP\n");
    printf("\tRun with GROUP privs. (%s)\n", run_as_group);
    printf("\n");

    printf("--help\n");
    printf("\tDisplay helpful information.\n");
    printf("\n");

#ifdef HAVE_HELPERS
    printf("--helpers=FILE\n");
    printf("\tUse FILE for rplay.helpers (%s).\n", RPLAY_HELPERS);
    printf("\n");
#endif /* HAVE_HELPERS */

#ifdef AUTH
    printf("-H FILE, --hosts=FILE\n");
    printf("\tUse FILE for rplay.hosts (%s).\n", RPLAY_HOSTS);
    printf("\n");
#endif /* AUTH */

    printf("--inetd\n");
    printf("\tEnable inetd mode. (%s)\n", inetd ? "enabled" : "disabled");
    printf("\n");

    printf("-L FILE, --log-file=FILE\n");
    printf("\tUse file for rplay.log (%s).\n", RPLAY_LOG);
    printf("\n");

    printf("-l N, --log-level=N\n");
    printf("\tUse logging level N where %d <= n <= %d.\n", REPORT_MIN, REPORT_MAX);
    printf("\n");

    printf("--memory-cache-size=N\n");
    printf("\tMaximum size in bytes of the memory cache, disable caching with 0 (%d).\n",
	   MEMORY_CACHE_SIZE);
    printf("\n");

    printf("--memory-cache-sound-size=N\n");
    printf("\tMaximum size in bytes of a sound that can be cached in memory.\n");
    printf("\tA value of 0 means to try and cache all sounds. (%d)\n",
	   MEMORY_CACHE_SOUND_SIZE);
    printf("\n");

    printf("-N, --no-audio\n");
    printf("\tDisable audio, RPTP file server mode.\n");
    printf("\n");

#ifdef AUTH
    printf("--no-auth\n");
    printf("\tDisable host access authentication.\n");
    printf("\n");
#endif

    printf("-n, --no-inetd\n");
    printf("\tDisable inetd mode. (%s)\n", inetd ? "enabled" : "disabled");
    printf("\n");

    printf("--no-fork\n");
    printf("\tDisable backgrounding rplayd at startup. (%s)\n",
	   do_fork ? "enabled" : "disabled");
    printf("\n");

    printf("--options-file=FILE\n");
    printf("\tRead rplayd options from FILE.\n");
    printf("\n");

    printf("--port=PORT, --rplay-port=PORT\n");
    printf("\tUse PORT as the RPLAY/UDP port. (%d)\n", RPLAY_PORT);
    printf("\t(--other-rplay-port may also be available)\n");
    printf("\n");

    printf("--rptp-port=PORT\n");
    printf("\tUse PORT as the RPTP/TCP port. (%d)\n", RPTP_PORT);
    printf("\t(--other-rptp-port may also be available)\n");
    printf("\n");

    printf("-S FILE, --servers=FILE\n");
    printf("\tUse FILE for rplay.servers (%s).\n", RPLAY_SERVERS);
    printf("\n");

    printf("-t N, --timeout=N\n");
    printf("\tExit after N idle seconds, disabled with 0 (%d).\n", rplayd_timeout);
    printf("\n");

    printf("--user=USER\n");
    printf("\tRun with USER privs. (%s)\n", run_as_user);
    printf("\n");

    printf("-v, --version\n");
    printf("\tPrint the rplay version and exit.\n");
}
