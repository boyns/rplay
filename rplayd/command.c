/* $Id: command.c,v 1.3 1998/11/06 15:16:49 boyns Exp $ */

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
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "rplayd.h"
#include "command.h"
#include "connection.h"
#include "server.h"
#include "host.h"
#include "spool.h"
#include "cache.h"
#include "sound.h"
#include "misc.h"
#include "strdup.h"
#include "getopt.h"
#ifdef HAVE_HELPERS
#include "helper.h"
#endif /* HAVE_HELPERS */

#undef is_true
#define is_true(string)				\
    (strcmp (string, "true") == 0		\
     || strcmp (string, "t") == 0		\
     || strcmp (string, "1") == 0		\
     || strcmp (string, "yes") == 0		\
     || strcmp (string, "y") == 0		\
     || strcmp (string, "on") == 0)		\

typedef struct
{
    char *name;
    int min_args;
    int max_args;
    char *usage;
#ifdef __STDC__
    int (*func) (CONNECTION *c, int argc, char **argv);
#else
    int (*func) ( /* CONNECTION *c, int argc, char **argv */ );
#endif
}
COMMAND;

#ifdef __STDC__
static int do_command (CONNECTION *c, int argc, char **argv);
static int command_quit (CONNECTION *c, int argc, char **argv);
static int command_unknown (CONNECTION *c, int argc, char **argv);
static int command_help (CONNECTION *c, int argc, char **argv);
static int command_get (CONNECTION *c, int argc, char **argv);
static int command_put (CONNECTION *c, int argc, char **argv);
static int command_list (CONNECTION *c, int argc, char **argv);
static int command_find (CONNECTION *c, int argc, char **argv);
static int command_execute (CONNECTION *c, int argc, char **argv);
static int command_access (CONNECTION *c, int argc, char **argv);
static int command_volume (CONNECTION *c, int argc, char **argv);
static int command_info (CONNECTION *c, int argc, char **argv);
static int command_version (CONNECTION *c, int argc, char **argv);
static int command_wait (CONNECTION *c, int argc, char **argv);
static int do_execute (CONNECTION *c, int argc, char **argv);
static int command_status (CONNECTION *c, int argc, char **argv);
static int command_application (CONNECTION *c, int argc, char **argv);
static int command_reset (CONNECTION *c, int argc, char **argv);
static int command_skip (CONNECTION *c, int argc, char **argv);
static int command_set (CONNECTION *c, int argc, char **argv);
static int command_modify (CONNECTION *c, int argc, char **argv);
#else
static int do_command ( /* CONNECTION *c, int argc, char **argv */ );
static int command_quit ( /* CONNECTION *c, int argc, char **argv */ );
static int command_unknown ( /* CONNECTION *c, int argc, char **argv */ );
static int command_help ( /* CONNECTION *c, int argc, char **argv */ );
static int command_get ( /* CONNECTION *c, int argc, char **argv */ );
static int command_put ( /* CONNECTION *c, int argc, char **argv */ );
static int command_list ( /* CONNECTION *c, int argc, char **argv */ );
static int command_find ( /* CONNECTION *c, int argc, char **argv */ );
static int command_execute ( /* CONNECTION *c, int argc, char **argv */ );
static int command_access ( /* CONNECTION *c, int argc, char **argv */ );
static int command_volume ( /* CONNECTION *c, int argc, char **argv */ );
static int command_info ( /* CONNECTION *c, int argc, char **argv */ );
static int command_version ( /* CONNECTION *c, int argc, char **argv */ );
static int command_wait ( /* CONNECTION *c, int argc, char **argv */ );
static int do_execute ( /* CONNECTION *c, int argc, char **argv */ );
static int command_status ( /* CONNECTION *c, int argc, char **argv */ );
static int command_application ( /* CONNECTION *c, int argc, char **argv */ );
static int command_reset ( /* CONNECTION *c, int argc, char **argv */ );
static int command_skip ( /* CONNECTION *c, int argc, char **argv */ );
static int command_set ( /* CONNECTION *c, int argc, char **argv */ );
static int command_modify ( /* CONNECTION *c, int argc, char **argv */ );
#endif

#ifdef DEBUG
extern int command_die ( /* CONNECTION *c, int argc, char **argv */ );
#endif

static COMMAND commands[] =
{
/*      
 *    command         min     max     usage                   function
 */
    {"access", -1, -1, "", command_access},
    {"application", 1, -1, "name", command_application},
    {"continue", 1, -1, "id|sound ...", command_execute},
#ifdef DEBUG
    {"die", -1, -1, "", command_die},
#endif
    {"done", 1, -1, "id|sound ...", command_execute},
    {"find", 1, 1, "sound", command_find},
    {"get", 1, 1, "sound", command_get},
    {"help", -1, -1, "", command_help},
    {"info", 1, 1, "sound", command_info},
#ifdef AUTH
    {"list", 0, 1, "[connections|hosts|servers|spool|sounds]", command_list},
#else
    {"list", 0, 1, "[connections|servers|sounds}", command_list},
#endif				/* AUTH */
    {"modify", 2, -1, "id [count|list-count|priority|sample-rate|volume] ...", command_modify},
    {"pause", 1, -1, "id|sound ...", command_execute},
    {"play", 1, -1, "sound ...", command_execute},
    {"put", 2, -1, "id|sound size", command_put},
    {"quit", 0, 0, "", command_quit},
    {"reset", 0, 0, "", command_reset},
    {"set", 1, -1, "name[=value] ...", command_set},
    {"skip", 1, -1, "id count", command_skip},
    {"status", 0, 0, "", command_status},
    {"stop", 1, -1, "id|sound ...", command_execute},
    {"version", 0, 0, "", command_version},
    {"volume", 0, 1, "[[+|-]volume]", command_volume},
    {"wait", -1, -1, "id|event|RPTP command", command_wait},
};
#define NCOMMANDS	(sizeof(commands)/sizeof(COMMAND))

/* Copy of the current RPTP command.  */
static char command_buffer[RPTP_MAX_LINE];

static char *default_client_data = "";

#ifdef __STDC__
int
command (CONNECTION *c, char *buf)
#else
int
command (c, buf)
    CONNECTION *c;
    char *buf;
#endif
{
    char *argv[RPTP_MAX_ARGS], *p;
    int argc = 0, first = 1;

    report (REPORT_INFO, "%s command=\"%s\"\n", inet_ntoa (c->sin.sin_addr), buf);
    strncpy (command_buffer, buf, sizeof(command_buffer));

    while ((p = strtok (first ? buf : NULL, " \t")))
    {
	argv[argc++] = p;
	first = 0;
	if (argc == RPTP_MAX_ARGS-1)
	{
	    break;
	}
    }
    argv[argc] = NULL;

    if (argc == 0)
    {
	return 0;
    }
    else
    {
	return do_command (c, argc, argv);
    }
}

#ifdef __STDC__
static int
do_command (CONNECTION *c, int argc, char **argv)
#else
static int
do_command (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    int i;

    for (i = 0; i < NCOMMANDS; i++)
    {
	if (strcmp (commands[i].name, argv[0]) == 0)
	{
	    if ((commands[i].min_args >= 0 && argc - 1 < commands[i].min_args)
		|| (commands[i].max_args >= 0 && argc - 1 > commands[i].max_args))
	    {
		connection_reply (c, "%cerror=\"usage: %s %s\"",
		    RPTP_ERROR, commands[i].name, commands[i].usage);
		return 0;
	    }
	    else
	    {
		return (*commands[i].func) (c, argc, argv);
	    }
	}
    }

    return command_unknown (c, argc, argv);
}

#ifdef __STDC__
static int
command_quit (CONNECTION *c, int argc, char **argv)
#else
static int
command_quit (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    /* connection_close (c); */
    return -1;
}

#ifdef __STDC__
static int
command_unknown (CONNECTION *c, int argc, char **argv)
#else
static int
command_unknown (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    char *client_data;
    
    client_data = rptp_parse (command_buffer, "client-data");
    if (!client_data)
    {
	client_data = default_client_data;
    }

    connection_reply (c, "%cerror=\"unknown command `%s'\" command=\"%s\" client-data=\"%s\"",
		      RPTP_ERROR, argv[0], argv[0], client_data);
    
    return 0;
}

#ifdef __STDC__
static int
command_help (CONNECTION *c, int argc, char **argv)
#else
static int
command_help (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    int i;
    char fmt[1024];
    EVENT *e;
    static BUFFER *b;

    if (b == NULL)
    {
	b = buffer_create ();
	b->status = BUFFER_KEEP;
	SNPRINTF (SIZE(b->buf,BUFFER_SIZE), "%cmessage=\"command summary\" command=help\r\n", RPTP_OK);
	b->nbytes += strlen(b->buf);
	for (i = 0; i < NCOMMANDS; i++)
	{
	    SNPRINTF (SIZE(fmt,sizeof(fmt)), "%-8s %s\r\n", commands[i].name, commands[i].usage);
	    SNPRINTF (SIZE(b->buf+b->nbytes,BUFFER_SIZE-b->nbytes), fmt);
	    b->nbytes += strlen(fmt);
	}
	SNPRINTF (SIZE(b->buf+b->nbytes,BUFFER_SIZE-b->nbytes), ".\r\n");
	b->nbytes += 3;
    }

    e = event_create (EVENT_WRITE, b);
    event_insert (c, e);

    return 0;
}

#ifdef __STDC__
static int
command_get (CONNECTION *c, int argc, char **argv)
#else
static int
command_get (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    SOUND *s;
    EVENT *e;
    char *sound_name = NULL;
    int old_style = 0;
    char *client_data = default_client_data;
    
    if (strchr (command_buffer, '='))
    {
	sound_name = rptp_parse (command_buffer, "sound");
	client_data = rptp_parse (0, "client-data");
	if (!client_data)
	{
	    client_data = default_client_data;
	}
    }
    if (!sound_name)
    {
	sound_name = argv[1];
	old_style++;
    }

#ifdef AUTH
    if (!host_access (c->sin, HOST_READ))
    {
	report (REPORT_NOTICE, "%s get %s - read access denied\n",
		inet_ntoa (c->sin.sin_addr),
		argv[1]);
	connection_reply (c, "%cerror=\"access denied\" command=get client-data=\"%s\"",
			  RPTP_ERROR, client_data);
	return 0;
    }
#endif /* AUTH */

    s = sound_lookup (sound_name, SOUND_DONT_FIND, NULL);
    if (s == NULL || s->status != SOUND_READY)
    {
	report (REPORT_NOTICE, "%s get %s - not found\n",
		inet_ntoa (c->sin.sin_addr), sound_name);
	connection_reply (c, "%cerror=\"%s not found\" command=get client-data=\"%s\"",
			  RPTP_ERROR, sound_name, client_data);
    }
    else if (s->type != SOUND_FILE)
    {
	connection_reply (c, "%cerror=\"%s not a file\" command=get client-data=\"%s\"",
			  RPTP_ERROR, s->name, client_data);
    }
    else
    {
	e = event_create (EVENT_WRITE_SOUND, s);
	if (e == NULL)
	{
	    report (REPORT_NOTICE, "%s get %s - cannot open\n",
		    inet_ntoa (c->sin.sin_addr), s->name);
	    connection_reply (c, "%cerror=\"cannot open %s\" command=get client-data=\"%s\"",
			      RPTP_ERROR, s->name, client_data);
	}
	else
	{
	    report (REPORT_NOTICE, "%s get %s %d\n",
		    inet_ntoa (c->sin.sin_addr), s->name, s->size);
	    if (old_style)
	    {
		connection_reply (c, "%c%s %d", RPTP_OK, s->name, s->size);
	    }
	    else
	    {
		connection_reply (c, "%csound=\"%s\" size=%d command=get client-data=\"%s\"",
				  RPTP_OK, s->name, s->size, client_data);
	    }
	    event_insert (c, e);
	}
    }

    return 0;
}

#ifdef __STDC__
static int
command_put (CONNECTION *c, int argc, char **argv)
#else
static int
command_put (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    EVENT *e;
    int sound_size;
    char *name, *sound_name = NULL, *p;
    SOUND *s;
    int fd;
    int old_style = 0;
    int spool_id = 0;
    char *client_data = default_client_data;
    
    if (strchr (command_buffer, '='))
    {
	client_data = rptp_parse (command_buffer, "client-data");
	if (!client_data)
	{
	    client_data = default_client_data;
	}
    }
    else
    {
	old_style++;
    }
    
#ifdef AUTH
    if (!host_access (c->sin, HOST_WRITE))
    {
	report (REPORT_NOTICE, "%s put %s - write access denied\n",
		inet_ntoa (c->sin.sin_addr),
		argv[1]);
	connection_reply (c, "%cerror=\"access denied\" command=put client-data=\"%s\"",
			  RPTP_ERROR, client_data);
	return 0;
    }
#endif /* AUTH */

    if (!old_style)
    {
	char *p;
	
	sound_name = rptp_parse (0, "sound");
	
	p = rptp_parse (0, "size");
	if (!p)
	{
	    connection_reply (c, "%cerror=\"missing `size=<value>'\" command=put client-data=\"%s\"",
			      RPTP_ERROR, client_data);
	    return 0;
	}
	sound_size = atoi (p);

	p = rptp_parse (0, "id");
	if (p && *p && p[0] == '#')
	{
	    spool_id = atoi (p+1);
	}
	else
	{
	    connection_reply (c, "%cerror=\"invalid `id'\" command=put client-data=\"%s\"",
			      RPTP_ERROR, client_data);
	    return 0;
	}
    }
    else /* old-style */
    {
	sound_name = argv[1];
	sound_size = atoi (argv[2]);
	old_style++;
    }

    if (spool_id)
    {
	SPOOL *sp;
#ifdef HAVE_HELPERS	
	HELPER *hp;
#endif	

	sp = spool_find (spool_id);
	if (!sp)
	{
	    connection_reply (c, "%cerror=\"`%d' no such spool id\" command=put client-data=\"%s\"",
			      RPTP_ERROR, spool_id, client_data);
	    return 0;
	}
	else if (sp->sound[sp->curr_sound]->type != SOUND_FLOW)
	{
	    connection_reply (c, "%cerror=\"`%d' spool id is not a flow\" command=put client-data=\"%s\"",
			      RPTP_ERROR, spool_id, client_data);
	    return 0;
	}

	connection_reply (c, "%cid=#%d size=%d command=put client-data=\"%s\"",
			  RPTP_OK, spool_id, sound_size, client_data);

#ifdef HAVE_HELPERS	
	/* XXX - it isn't known yet whether or not this sound
	   will need a helper.  Check here too. */
	hp = helper_lookup (sp->sound[sp->curr_sound]->path);
	if (hp)
	{
	    SOUND *s = sp->sound[sp->curr_sound];
	    e = event_create (EVENT_PIPE_FLOW, spool_id, s);
	    event_insert (c, e);

	    s->status = SOUND_READY;
	    sound_map (s);
	    spool_ready (s);
	}
	else
#endif /* HAVE_HELPERS */	    
	{
	    e = event_create (EVENT_READ_FLOW, spool_id, sound_size);
	    event_insert (c, e);
	}

	return 0;
    }
    else
    {
	/* strip pathnames -- files can only be put in the cache directory */
	p = strrchr (sound_name, '/');
	if (p)
	{
	    sound_name = p + 1;
	}
	name = cache_name (sound_name);

	s = sound_lookup (name, SOUND_DONT_FIND, NULL);
	if (s != NULL)
	{
	    connection_reply (c, "%cerror=\"%s already is in the cache\" command=put client-data=\"%s\"",
			      RPTP_ERROR, sound_name, client_data);
	    return 0;
	}

	if (cache_free (sound_size) < 0)
	{
	    connection_reply (c, "%cerror=\"the cache is full\" command=put client-data=\"%s\"",
			      RPTP_ERROR, client_data);
	}
	else
	{
	    fd = cache_create (name, sound_size);
	    if (fd < 0)
	    {
		connection_reply (c, "%cerror=\"cache error\" command=put client-data=\"%s\"",
				  RPTP_ERROR, client_data);
	    }
	    else
	    {
		report (REPORT_NOTICE, "%s put %s %d\n",
			inet_ntoa (c->sin.sin_addr), sound_name, sound_size);
		s = sound_insert (name, SOUND_NOT_READY, SOUND_FILE);
		if (old_style)
		{
		    connection_reply (c, "%c%s %d", RPTP_OK, sound_name, sound_size);
		}
		else
		{
		    connection_reply (c, "%csound=\"%s\" size=%d command=put client-data=\"%s\"",
				      RPTP_OK, sound_name, sound_size, client_data);
		}
		e = event_create (EVENT_READ_SOUND, fd, buffer_create (), sound_size, s);
		event_insert (c, e);
	    }
	}
    
	return 0;
    }
}

#ifdef __STDC__
static int
command_list (CONNECTION *c, int argc, char **argv)
#else
static int
command_list (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    EVENT *e;
    char *client_data = default_client_data;
    BUFFER *b;
    
    if (strchr (command_buffer, '='))
    {
	client_data = rptp_parse (command_buffer, "client-data");
	if (!client_data)
	{
	    client_data = default_client_data;
	}
    }

    if (argv[1] == NULL || strcmp (argv[1], "sounds") == 0)
    {
	b = sound_list_create ();
	if (b)
	{
	    e = event_create (EVENT_WRITE, b);
	    event_insert (c, e);
	}
	else
	{
	    connection_reply (c, "%cerror=\"no sounds available\" command=list client-data=\"%s\"",
			      RPTP_ERROR, client_data);
	}
    }
    else if (strcmp (argv[1], "connections") == 0)
    {
	b = connection_list_create ();
	if (b)
	{
	    e = event_create (EVENT_WRITE, b);
	    event_insert (c, e);
	}
	else
	{
	    connection_reply (c, "%cerror=\"no connections available\" command=list client-data=\"%s\"",
			      RPTP_ERROR, client_data);
	}
    }
    else if (strcmp (argv[1], "servers") == 0)
    {
	if (server_list)
	{
	    e = event_create (EVENT_WRITE, server_list);
	    event_insert (c, e);
	}
	else
	{
	    connection_reply (c, "%cerror=\"no servers available\" command=list client-data=\"%s\"",
			      RPTP_ERROR, client_data);
	}
    }
    else if (strcmp (argv[1], "spool") == 0)
    {
	b = spool_list_create ();
	if (b)
	{
	    e = event_create (EVENT_WRITE, b);
	    event_insert (c, e);
	}
	else
	{
	    connection_reply (c, "%cerror=\"no spool available\" command=list client-data=\"%s\"",
			      RPTP_ERROR, client_data);
	}
    }
#ifdef AUTH
    else if (strcmp (argv[1], "hosts") == 0)
    {
	b = host_list;
	if (b)
	{
	    e = event_create (EVENT_WRITE, b);
	    event_insert (c, e);
	}
	else
	{
	    connection_reply (c, "%cerror=\"no hosts available\" command=list client-data=\"%s\"",
			      RPTP_ERROR, client_data);
	}
    }
#endif /* AUTH */
    else
    {
	connection_reply (c, "%cerror=\"cannot list `%s'\" command=list client-data=\"%s\"",
			  RPTP_ERROR, argv[1], client_data);
    }

    return 0;
}

#ifdef __STDC__
static int
command_find (CONNECTION *c, int argc, char **argv)
#else
static int
command_find (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    SOUND *s;
    char *sound_name = NULL;
    int old_style = 0;
    char *client_data = default_client_data;
    
    if (strchr (command_buffer, '='))
    {
	sound_name = rptp_parse (command_buffer, "sound");
	client_data = rptp_parse (0, "client-data");
	if (!client_data)
	{
	    client_data = default_client_data;
	}
    }
    if (!sound_name)
    {
	sound_name = argv[1];
	old_style++;
    }

    s = sound_lookup (sound_name, SOUND_DONT_FIND, NULL);
    if (s == NULL || s->status != SOUND_READY)
    {
	report (REPORT_NOTICE, "%s find %s - not found\n",
		inet_ntoa (c->sin.sin_addr), sound_name);
	connection_reply (c, "%cerror=\"%s not found\" command=find client-data=\"%s\"",
			  RPTP_ERROR, sound_name, client_data);
    }
    else
    {
	report (REPORT_NOTICE, "%s find %s %d\n",
		inet_ntoa (c->sin.sin_addr), sound_name, s->size);
	if (old_style)
	{
	    connection_reply (c, "%c%s %d", RPTP_OK, s->name, s->size);
	}
	else
	{
	    connection_reply (c, "%csound=\"%s\" size=%d command=find client-data=\"%s\"",
			      RPTP_OK, s->name, s->size, client_data);
	}
    }

    return 0;
}

#ifdef __STDC__
static int
command_access (CONNECTION *c, int argc, char **argv)
#else
static int
command_access (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    char buf[4];
    char *client_data = default_client_data;
    
    if (strchr (command_buffer, '='))
    {
	client_data = rptp_parse (command_buffer, "client-data");
	if (!client_data)
	{
	    client_data = default_client_data;
	}
    }
    
    buf[0] = '\0';
#ifdef AUTH
    if (host_access (c->sin, HOST_READ))
    {
	strcat (buf, "r");
    }
    if (host_access (c->sin, HOST_WRITE))
    {
	strcat (buf, "w");
    }
    if (host_access (c->sin, HOST_EXECUTE))
    {
	strcat (buf, "x");
    }
#else /* AUTH */
    strcat (buf, "rwx");
#endif /* AUTH */

    connection_reply (c, "%caccess=%s command=access client-data=\"%s\"",
		      RPTP_OK, buf, client_data);

    return 0;
}

#ifdef __STDC__
static int
command_execute (CONNECTION *c, int argc, char **argv)
#else
static int
command_execute (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    SPOOL *sp;
    int id = do_execute (c, argc, argv);

    if (id > 0)
    {
	sp = spool_find (id);
	connection_reply (c, "%cid=#%d sound=\"%s\" command=%s client-data=\"%s\" list-name=\"%s\"",
			  RPTP_OK, id,
			  sp->curr_attrs->sound,
			  argv[0],
			  sp->curr_attrs->client_data,
			  sp->rp->list_name);
    }

    return 0;
}

/*
 * Return -1 for errors, 0 for stop, pause, continue, and return
 * a spool id for play.
 */
#ifdef __STDC__
static int
do_execute (CONNECTION *c, int argc, char **argv)
#else
static int
do_execute (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    int old_style = 0;
    int i, n, command = RPLAY_PLAY, val = 0;
    RPLAY *rp;
    int volume, list_count, count, priority, sample_rate;
    int do_random = 0;
    int do_search = 1; /* search by default */
    int input = SOUND_FILE;
    int input_offset = 0;
    int input_format = 0;
    int input_byte_order = 0;
    int input_sample_rate = 0;
    float input_precision = 0;
    int input_channels = 0;
    int input_storage = SOUND_STORAGE_NONE; /* Don't store flows by default. */
    char *client_data = default_client_data;
    char *list_name = NULL;
    
    if (strcmp (argv[0], "play") == 0)
    {
	command = RPLAY_PLAY;
    }
    else if (strcmp (argv[0], "stop") == 0)
    {
	command = RPLAY_STOP;
    }
    else if (strcmp (argv[0], "pause") == 0)
    {
	command = RPLAY_PAUSE;
    }
    else if (strcmp (argv[0], "continue") == 0)
    {
	command = RPLAY_CONTINUE;
    }
    else if (strcmp (argv[0], "done") == 0)
    {
	command = RPLAY_DONE;
    }

    if (strchr (command_buffer, '='))
    {
	client_data = rptp_parse (command_buffer, "client-data");
	if (!client_data)
	{
	    client_data = default_client_data;
	}
    }
    else
    {
	old_style++;
    }
    
#ifdef AUTH
    if (!host_access (c->sin, HOST_EXECUTE))
    {
	report (REPORT_NOTICE, "%s %s access denied\n", argv[0],
		inet_ntoa (c->sin.sin_addr));
	connection_reply (c, "%cerror=\"access denied\" command=%s client-data=\"%s\"",
			  RPTP_ERROR, argv[0], client_data);
	return -1;
    }
#endif /* AUTH */

    rp = rplay_create (command);
    if (rp == NULL)
    {
	connection_reply (c, "%cerror=\"%s failed\" command=%s client_data=\"%s\"",
			  RPTP_ERROR, argv[0], argv[0], client_data);
	return -1;
    }

    volume = RPLAY_DEFAULT_VOLUME;
    count = RPLAY_DEFAULT_COUNT;
    list_count = RPLAY_DEFAULT_LIST_COUNT;
    priority = RPLAY_DEFAULT_PRIORITY;
    sample_rate = RPLAY_DEFAULT_SAMPLE_RATE;

    if (!old_style)
    {
	char *name, *value;

	rptp_parse (command_buffer, 0);
	while (name = rptp_parse (0, 0))
	{
	    value = rptp_parse (0, name);

	    if (!value || !*value)
	    {
		continue;
	    }
	    else if (strcmp (name, "sound") == 0
		     || (command != RPLAY_PLAY && strcmp (name, "id") == 0))
	    {
		val = rplay_set (rp, RPLAY_APPEND,
				 RPLAY_SOUND, value,
				 RPLAY_VOLUME, volume,
				 RPLAY_COUNT, count,
				 RPLAY_SAMPLE_RATE, sample_rate,
				 RPLAY_RPTP_SEARCH, do_search,
				 RPLAY_CLIENT_DATA, client_data,
				 NULL);
		if (val < 0)
		{
		    connection_reply (c, "%cerror=\"%s failed\" command=%s client-data=\"%s\"",
				      RPTP_ERROR, argv[0], argv[0], client_data);
		    return -1;
		}
	    }
	    else if (strcmp (name, "client-data") == 0)
	    {
		client_data = value;
	    }
	    else if (strcmp (name, "volume") == 0)
	    {
		volume = atoi (value);
	    }
	    else if (strcmp (name, "count") == 0)
	    {
		count = atoi (value);
	    }
	    else if (strcmp (name, "list-count") == 0)
	    {
		list_count = atoi (value);
	    }
	    else if (strcmp (name, "priority") == 0)
	    {
		priority = atoi (value);
	    }
	    else if (strcmp (name, "sample-rate") == 0)
	    {
		sample_rate = atoi (value);
	    }
	    else if (strcmp (name, "search") == 0
		     || strcmp (name, "rptp-search") == 0)
	    {
		do_search = is_true (value);
	    }
	    else if (strcmp (name, "random") == 0)
	    {
		do_random = is_true (value);
	    }
	    else if (strcmp (name, "list-name") == 0)
	    {
		list_name = value;
	    }
	    else if (strcmp (name, "input") == 0)
	    {
		input = string_to_input (value);
	    }
	    else if (strcmp (name, "input-offset") == 0)
	    {
		input_offset = atoi (value);
	    }
	    else if (strcmp (name, "input-format") == 0)
	    {
		input_format = string_to_audio_format (value);
	    }
	    else if (strcmp (name, "input-byte-order") == 0)
	    {
		input_byte_order = string_to_byte_order (value);
	    }
	    else if (strcmp (name, "input-sample-rate") == 0)
	    {
		input_sample_rate = atoi (value);
	    }
	    else if (strcmp (name, "input-bits") == 0)
	    {
		input_precision = atof (value);
	    }
	    else if (strcmp (name, "input-channels") == 0)
	    {
		input_channels = atoi (value);
	    }
	    else if (strcmp (name, "input-storage") == 0)
	    {
		input_storage = string_to_storage (value);
	    }
	}
    }
    else /* old-style */
    {
	extern char *optarg;
	extern int optind;

	optind = 0;
	while ((n = getopt (argc, argv, "+P:R:N:n:v:")) != -1)
	{
	    switch (n)
	    {
	    case 'v':
		volume = atoi (optarg);
		break;

	    case 'n':
		count = atoi (optarg);
		break;

	    case 'N':
		list_count = atoi (optarg);
		break;

	    case 'P':
		priority = atoi (optarg);
		break;

	    case 'R':
		sample_rate = atoi (optarg);
		break;

	    default:
		argc = optind;
		break;
	    }
	}

	if (argc == optind)
	{
	    for (i = 0; i < NCOMMANDS; i++)
	    {
		if (strcmp (commands[i].name, argv[0]) == 0)
		{
		    connection_reply (c, "%cerror=\"usage: %s %s\" command=%s",
				      RPTP_ERROR, commands[i].name, commands[i].usage,
				      argv[0]);
		    return -1;
		}
	    }
	}

	while (argv[optind] != NULL)
	{
	    val = rplay_set (rp, RPLAY_APPEND,
			     RPLAY_SOUND, argv[optind++],
			     RPLAY_VOLUME, volume,
			     RPLAY_COUNT, count,
			     RPLAY_SAMPLE_RATE, sample_rate,
			     RPLAY_RPTP_SEARCH, do_search,
			     NULL);
	    if (val < 0)
	    {
		connection_reply (c, "%cerror=\"%s failed\" command=%s",
				  RPTP_ERROR, argv[0], argv[0]);
		return -1;
	    }
	}
    }

    if (rplay_set (rp, RPLAY_LIST_COUNT, list_count, NULL) < 0)
    {
	connection_reply (c, "%cerror=\"%s failed\" command=%s client-data=\"%s\"",
			  RPTP_ERROR, argv[0], argv[0], client_data);
	return -1;
    }

    if (rplay_set (rp, RPLAY_PRIORITY, priority, NULL) < 0)
    {
	connection_reply (c, "%cerror=\"%s failed\" command=%s client-data=\"%s\"",
			  RPTP_ERROR, argv[0], argv[0], client_data);
	return -1;
    }

    if (do_random && rplay_set (rp, RPLAY_RANDOM_SOUND, NULL) < 0)
    {
	connection_reply (c, "%cerror=\"%s failed\" command=%s client-data=\"%s\"",
			  RPTP_ERROR, argv[0], argv[0], client_data);
	return -1;
    }

    if (list_name && rplay_set (rp, RPLAY_LIST_NAME, list_name, NULL) < 0)
    {
	connection_reply (c, "%cerror=\"%s failed\" command=%s client-data=\"%s\"",
			  RPTP_ERROR, argv[0], argv[0], client_data);
	return -1;
    }
    
    /* Flows */
    if (input == SOUND_FLOW && command == RPLAY_PLAY)
    {
	int spool_id, i;
	SOUND *s;
	char buf[RPTP_MAX_LINE], *sound_name, *p;

#ifdef AUTH
	/* Flows require `write' access along with the previously
	   checked `execute' access. */
	if (!host_access (c->sin, HOST_WRITE))
	{
	    report (REPORT_NOTICE, "%s %s access denied\n", argv[0],
		    inet_ntoa (c->sin.sin_addr));
	    connection_reply (c, "%cerror=\"access denied\" command=%s client-data=\"%s\"",
			      RPTP_ERROR, argv[0], client_data);
	    return -1;
	}
#endif /* AUTH */
	
	/* Generate a unique sound name. */
	sound_name = (char *) rplay_get (rp, RPLAY_SOUND, 0);
	if (!sound_name)
	{
	    connection_reply (c, "%cerror=\"missing required `sound' attribute\" command=%s client-data=\"%s\"",
			      RPTP_ERROR, argv[0], client_data);
	    return -1;
	}
	p = strrchr (sound_name, '/');
	if (p)
	{
	    sound_name = p + 1;
	}
	i = 0;
	do
	{
	    if (i)
	    {
		SNPRINTF (SIZE(buf,sizeof(buf)), "rplay%d-%s", i, sound_name);
	    }
	    else
	    {
		SNPRINTF (SIZE(buf,sizeof(buf)), sound_name);
	    }
	    i++;
	}
	while (sound_lookup (buf, SOUND_DONT_FIND, NULL));
	
	sound_name = cache_name (buf);
	rplay_set (rp, RPLAY_CHANGE, 0,
		   RPLAY_SOUND, sound_name,
		   RPLAY_RPTP_SEARCH, FALSE, /* never search */
		   NULL);
	s = sound_insert (sound_name, SOUND_NOT_READY, SOUND_FLOW);
	if (!s)
	{
	    connection_reply (c, "%cerror=\"%s failed\" command=%s client-data=\"%s\"",
			      RPTP_ERROR, argv[0], argv[0], client_data);
	    return -1;
	}

	s->offset = input_offset;
	s->format = input_format;
	s->byte_order = input_byte_order;
	s->sample_rate = input_sample_rate;
	s->input_precision = input_precision;
	s->output_precision = input_precision;
	s->channels = input_channels;
	s->storage = input_storage;

	/* Grab a spool entry. */
	spool_id = rplayd_play (rp, c->sin);
	if (spool_id < 0)
	{
	    connection_reply (c, "%cerror=\"spool full\" command=%s client-data=\"%s\"",
			      RPTP_ERROR, argv[0], client_data);
	    sound_delete (s, 0);
	    return -1;
	}

	connection_reply (c, "%cid=#%d sound=\"%s\" command=%s client-data=\"%s\"",
			  RPTP_OK, spool_id, buf, argv[0], client_data);
	
	return 0;
    }
    else
    {
	switch (command)
	{
	case RPLAY_PLAY:
	    val = rplayd_play (rp, c->sin);
	    break;

	case RPLAY_STOP:
	    val = rplayd_stop (rp, c->sin);
	    break;

	case RPLAY_PAUSE:
	    val = rplayd_pause (rp, c->sin);
	    break;

	case RPLAY_CONTINUE:
	    val = rplayd_continue (rp, c->sin);
	    break;

	case RPLAY_DONE:
	    val = rplayd_done (rp, c->sin);
	    break;
	}

	if (val == 0)
	{
	    connection_reply (c, "%cmessage=\"%s successful\" command=%s client-data=\"%s\"",
			      RPTP_OK, argv[0], argv[0], client_data);
	}
	else if (val < 0)
	{
	    connection_reply (c, "%cerror=\"%s failed\" command=%s client-data=\"%s\"",
			      RPTP_ERROR, argv[0], argv[0], client_data);
	}

	return val;
    }
}

/* OBSOLETE */
#ifdef __STDC__
static int
command_volume (CONNECTION *c, int argc, char **argv)
#else
static int
command_volume (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    int volume;
    int n;

#ifdef AUTH
    if (!host_access (c->sin, HOST_EXECUTE))
    {
	report (REPORT_NOTICE, "%s volume access denied\n",
	    inet_ntoa (c->sin.sin_addr));
	connection_reply (c, "%cerror=\"access denied\" command=volume", RPTP_ERROR);
	return 0;
    }
#endif /* AUTH */

    if (argv[1] == NULL)
    {
	volume = rplay_audio_get_volume ();
	if (volume >= 0)
	{
	    connection_reply (c, "%c%d", RPTP_OK, volume);
	}
	else
	{
	    connection_reply (c, "%cerror=\"volume failed\" command=volume", RPTP_ERROR);
	}
    }
    else if (*argv[1] == '+')
    {
	volume = rplay_audio_get_volume ();
	if (volume < 0)
	{
	    connection_reply (c, "%cerror=\"volume failed\" command=volume", RPTP_ERROR);
	}

	volume += atoi (argv[1] + 1);
	n = rplay_audio_set_volume (volume);
	if (n >= 0)
	{
	    connection_reply (c, "%c%d", RPTP_OK, n);
	    connection_notify (0, NOTIFY_VOLUME, n);
	}
	else
	{
	    connection_reply (c, "%cerror=\"volume failed\" command=volume", RPTP_ERROR);
	}
    }
    else if (*argv[1] == '-')
    {
	volume = rplay_audio_get_volume ();
	if (volume < 0)
	{
	    connection_reply (c, "%cerror=\"volume failed\" command=volume", RPTP_ERROR);
	}

	volume -= atoi (argv[1] + 1);
	n = rplay_audio_set_volume (volume);
	if (n >= 0)
	{
	    connection_reply (c, "%c%d", RPTP_OK, n);
	    connection_notify (0, NOTIFY_VOLUME, n);
	}
	else
	{
	    connection_reply (c, "%cerror=\"volume failed\" command=volume", RPTP_ERROR);
	}
    }
    else
    {
	volume = atoi (argv[1]);
	n = rplay_audio_set_volume (volume);
	if (n >= 0)
	{
	    connection_reply (c, "%c%d", RPTP_OK, n);
	    connection_notify (0, NOTIFY_VOLUME, n);
	}
	else
	{
	    connection_reply (c, "%cerror=\"volume failed\" command=volume", RPTP_ERROR);
	}
    }

    return 0;
}

#ifdef __STDC__
static int
command_info (CONNECTION *c, int argc, char **argv)
#else
static int
command_info (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    SOUND *s;
    char *sound_name = NULL;
    char *client_data = default_client_data;
    
#ifdef AUTH
    if (!host_access (c->sin, HOST_READ))
    {
	report (REPORT_NOTICE, "%s info %s - read access denied\n",
	    inet_ntoa (c->sin.sin_addr), argv[1]);
	connection_reply (c, "%cerror=\"access denied\" command=info client-data=\"%s\"",
			  RPTP_ERROR, client_data);
	return 0;
    }
#endif /* AUTH */

    if (strchr (command_buffer, '='))
    {
	sound_name = rptp_parse (command_buffer, "sound");
	client_data = rptp_parse (0, "client-data");
	if (!client_data)
	{
	    client_data = default_client_data;
	}
    }
    if (!sound_name)
    {
	sound_name = argv[1];
    }

    /* s = sound_lookup (sound_name, SOUND_CREATE, NULL); */
    s = sound_lookup (sound_name, SOUND_LOAD, NULL);
    if (s == NULL || s->status != SOUND_READY)
    {
	report (REPORT_NOTICE, "%s info %s - not found\n",
		inet_ntoa (c->sin.sin_addr), sound_name);
	connection_reply (c, "%cerror=\"%s not found\" command=info client-data=\"%s\"",
			  RPTP_ERROR, sound_name, client_data);
    }
    else
    {
	connection_reply (c, "\
%csound=\"%s\" size=%d bits=%g sample-rate=%d channels=%d format=%s byte-order=%s input=%s seconds=%.2f command=info client-data=\"%s\"",
			  RPTP_OK,
			  s->name,
			  s->size,
			  s->input_precision,
			  s->sample_rate,
			  s->channels,
			  audio_format_to_string (s->format),
			  byte_order_to_string (s->byte_order),
			  input_to_string (s->type),
			  s->sample_rate && s->samples ? (double) s->samples / s->sample_rate : 0,
			  client_data);
    }

    return 0;
}

#ifdef __STDC__
static int
command_version (CONNECTION *c, int argc, char **argv)
#else
static int
command_version (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    char *client_data;
    
    client_data = rptp_parse (command_buffer, "client-data");
    if (!client_data)
    {
	client_data = default_client_data;
    }
    
    connection_reply (c, "%cversion=%s command=version client-data=\"%s\"",
		      RPTP_OK, RPLAY_VERSION, client_data);

    return 0;
}

#ifdef __STDC__
static void
do_events (char *event_string, int *mask, int *spool_id)
#else
static void
do_events (event_string, mask, spool_id)
    char *event_string;
    int *mask;
    int *spool_id;
#endif
{
    char buf[RPTP_MAX_LINE];
    int first = 1;
    char *p;
    int operator, bit;

    *spool_id = 0;
    
    strncpy (buf, event_string, sizeof(buf));

    while (p = strtok (first ? buf : NULL, ",| "))
    {
	first = 0;
	bit = 0;
	operator = 0;
	
	if (*p == '+')
	{
	    operator = 0;
	    p++;
	}
	else if (*p == '-')
	{
	    operator = 1;
	    p++;
	}
	
	if (strcmp (p, "all") == 0 || strcmp (p, "any") == 0)
	{
	    bit = NOTIFY_ANY;
	}
	else if (strcmp (p, "none") == 0)
	{
	    *mask = 0;
	}
	else if (strcmp (p, "play") == 0)
	{
	    bit = NOTIFY_PLAY;
	}
	else if (strcmp (p, "stop") == 0)
	{
	    bit = NOTIFY_STOP;
	}
	else if (strcmp (p, "pause") == 0)
	{
	    bit = NOTIFY_PAUSE;
	}
	else if (strcmp (p, "continue") == 0)
	{
	    bit = NOTIFY_CONTINUE;
	}
	else if (strcmp (p, "volume") == 0)
	{
	    bit = NOTIFY_VOLUME;
	}
	else if (strcmp (p, "done") == 0)
	{
	    bit = NOTIFY_DONE;
	}
	else if (strcmp (p, "skip") == 0)
	{
	    bit = NOTIFY_SKIP;
	}
	else if (strcmp (p, "state") == 0)
	{
	    bit = NOTIFY_STATE;
	}
	else if (strcmp (p, "flow") == 0)
	{
	    bit = NOTIFY_FLOW;
	}
	else if (p[0] == '#')
	{
	    *spool_id = atoi (p+1);
	}
	else if (strcmp (p, "modify") == 0)
	{
	    bit = NOTIFY_MODIFY;
	}
	else if (strcmp (p, "level") == 0)
	{
	    bit = NOTIFY_LEVEL;
	}
	else if (strcmp (p, "position") == 0)
	{
	    bit = NOTIFY_POSITION;
	}
	
	if (operator == 0)
	{
	    SET_BIT (*mask, bit);
	}
	else
	{
	    CLR_BIT (*mask, bit);
	}
    }

    /* If a spool id is set and there's no mask, assign a default
       mask which will notify everything than can happen to the
       spool entry. */
    if (*spool_id && !*mask)
    {
	SET_BIT (*mask, NOTIFY_SPOOL);
    }
}

#ifdef __STDC__
static int
command_wait (CONNECTION *c, int argc, char **argv)
#else
static int
command_wait (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    EVENT *e;
    char *client_data;
    
    client_data = rptp_parse (command_buffer, "client-data");
    if (!client_data)
    {
	client_data = default_client_data;
    }
    
    /* `wait' - wait for anything */
    if (argc == 1)
    {
	e = event_create (EVENT_NOTIFY);
	SET_BIT (e->wait_mask, NOTIFY_ANY);
	event_insert (c, e);
	return 0;
    }
    /* `wait id=#spool-id' or `wait event=event,event,event' */
    else if (argc == 2 && strchr (argv[1], '='))
    {
	char *value;

	value = rptp_parse (argv[1], "id");
	if (value && *value)
	{
	    e = event_create (EVENT_NOTIFY);
	    SET_BIT (e->wait_mask, NOTIFY_DONE);
	    e->id = atoi (value + 1);
	    event_insert (c, e);
	    return 0;
	}

	value = rptp_parse (argv[1], "event");
	if (value && *value)
	{
	    e = event_create (EVENT_NOTIFY);
	    do_events (value, &e->wait_mask, &e->id);
	    event_insert (c, e);
	    return 0;
	}

	connection_reply (c, "%cerror=\"unknown wait attribute `%s'\" command=wait client-data=\"%s\"",
			  RPTP_ERROR, argv[1], client_data);
	return 0;
    }
    /* `wait play ...' */
    else if (strcmp (argv[1], "play") == 0)
    {
	int id;

	id = do_execute (c, argc - 1, argv + 1);
	if (id > 0)
	{
	    e = event_create (EVENT_NOTIFY);
	    SET_BIT (e->wait_mask, NOTIFY_DONE);
	    e->id = id;
	    event_insert (c, e);
	}

	return 0;
    }
    else
    {
	/* XXX: Need to fix command_buffer */
	int i, n;
	command_buffer[0] = '\0';
	n = 0;
	for (i = 1; i < argc; i++)
	{
	    SNPRINTF (SIZE(command_buffer+n,sizeof(command_buffer)-n), argv[i]);
	    n += strlen(argv[i]);
	    if (i + 1 != argc)
	    {
		SNPRINTF (SIZE(command_buffer+n,sizeof(command_buffer)-n), " ");
		n++;
	    }
	}

	return do_command (c, argc - 1, argv + 1);
    }
}

#ifdef __STDC__
static int
command_status (CONNECTION *c, int argc, char **argv)
#else
static int
command_status (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    EVENT *e;

    e = event_create (EVENT_WRITE, rplayd_status ());
    event_insert (c, e);

    return 0;
}

/* OBSOLETE */
#ifdef __STDC__
static int
command_application (CONNECTION *c, int argc, char **argv)
#else
static int
command_application (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    int i, n;
    char buf[RPTP_MAX_LINE];

    buf[0] = '\0';
    n = 0;
    for (i = 1; i < argc; i++)
    {
	SNPRINTF (SIZE(buf+n,sizeof(buf)-n), argv[i]);
	n += strlen(argv[i]);
	if (i + 1 != argc)
	{
	    SNPRINTF (SIZE(buf+n,sizeof(buf)-n), " ");
	    n++;
	}
    }

    if (c->application)
    {
	free (c->application);
    }

    c->application = strdup (buf);

    connection_reply (c, "%c%s", RPTP_OK, c->application);

    return 0;
}

#ifdef DEBUG
#ifdef __STDC__
static int
command_die (CONNECTION *c, int argc, char **argv)
#else
static int
command_die (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    report (REPORT_DEBUG, "time to die...\n");
    connection_close (c);
    done (0);
    return -1;			/* not reached */
}
#endif /* DEBUG */

#ifdef __STDC__
static int
command_reset (CONNECTION *c, int argc, char **argv)
#else
static int
command_reset (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    char *client_data;
    
    client_data = rptp_parse (command_buffer, "client-data");
    if (!client_data)
    {
	client_data = default_client_data;
    }

#ifdef AUTH
    if (!host_access (c->sin, HOST_EXECUTE))
    {
	report (REPORT_NOTICE, "%s reset access denied\n", inet_ntoa (c->sin.sin_addr));
	connection_reply (c, "%cerror=\"access denied\" command=reset client-data=\"%s\"",
			  RPTP_ERROR, client_data);
	return 0;
    }
#endif /* AUTH */

    need_reset ();

    connection_reply (c, "%cmessage=\"reset successful\" command=reset client-data=\"%s\"",
		      RPTP_OK, client_data);

    return 0;
}

#ifdef __STDC__
static int
command_skip (CONNECTION *c, int argc, char **argv)
#else
static int
command_skip (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    SPOOL *sp;
    int id = 0, count = 1;	/* default */
    int nskipped = 0;
    char *p;
    char *client_data;
    
    client_data = rptp_parse (command_buffer, "client-data");
    if (!client_data)
    {
	client_data = default_client_data;
    }

#ifdef AUTH
    if (!host_access (c->sin, HOST_EXECUTE))
    {
	report (REPORT_NOTICE, "%s skip access denied\n", inet_ntoa (c->sin.sin_addr));
	connection_reply (c, "%cerror=\"access denied\" command=skip client-data=\"%s\"",
			  RPTP_ERROR, client_data);
	return 0;
    }
#endif /* AUTH */

    p = rptp_parse (0, "id");
    if (p)
    {
	id = atoi (p + 1);
    }
    p = rptp_parse (0, "count");
    if (p)
    {
	count = atoi (p);
    }

    for (sp = spool; sp; sp = sp->next)
    {
	if (!id || sp->id == id)
	{
	    spool_skip (sp, count);
	    nskipped++;
	}
    }

    if (nskipped)
    {
	connection_reply (c, "%cmessage=\"skipped\" command=skip client-data=\"%s\"",
			  RPTP_OK, client_data);
    }
    else
    {
	connection_reply (c, "%cerror=\"skip failed\" command=skip client-data=\"%s\"",
			  RPTP_ERROR, client_data);
    }

    return 0;
}

#ifdef __STDC__
static int
command_set (CONNECTION *c, int argc, char **argv)
#else
static int
command_set (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    char *name, *value;
    BUFFER *b;
    EVENT *e;
    int volume_changed = 0;
    int priority_threshold_changed = 0;
    int audio_port_changed = 0;
    int notify_enabled = 0;
    SPOOL *sp;
    int notify_mask = 0;
    int notify_id = 0;
    char *client_data;
    char buf[RPTP_MAX_LINE];
    
    client_data = rptp_parse (command_buffer, "client-data");
    if (!client_data)
    {
	client_data = default_client_data;
    }

#ifdef AUTH
    if (!host_access (c->sin, HOST_EXECUTE))
    {
	report (REPORT_NOTICE, "%s set access denied\n", inet_ntoa (c->sin.sin_addr));
	connection_reply (c, "%cerror=\"access denied\" command=set client-data=\"%s\"",
			  RPTP_ERROR, client_data);
	return 0;
    }
#endif /* AUTH */

    b = buffer_create ();
    b->buf[0] = RPTP_OK;
    b->buf[1] = '\0';
    b->nbytes = 1;

    rptp_parse (command_buffer, 0);
    while (name = rptp_parse (0, 0))
    {
	value = rptp_parse (0, name);

	if (strcmp (name, "application") == 0)	/* `application=value' */
	{
	    if (value && *value)
	    {
		if (c->application)
		{
		    free (c->application);
		}
		c->application = strdup (value);
	    }
	    SNPRINTF (SIZE(buf, sizeof(buf)), "application=\"%s\"",
		      c->application ? c->application : "-1");
	    strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
	    b->nbytes += strlen(buf);
	}
	else if (strcmp (name, "notify") == 0)	/* `notify=[event,...]' */
	{
	    if (value)
	    {
		do_events (value, &c->notify_mask, &c->notify_id);
	    }
	    
	    if (!c->notify_mask)
	    {
		SNPRINTF (SIZE(buf, sizeof(buf)), "notify=none");
		strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
		b->nbytes += strlen(buf);
	    }
	    else
	    {
		SNPRINTF (SIZE (buf, sizeof(buf)), "notify=\"%s\"", value);
		strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
		b->nbytes += strlen(buf);
		notify_enabled++;
	    }
	}
	else if (strcmp (name, "volume") == 0)	/* `volume=[+|-]value' */
	{
	    int volume = rplay_audio_get_volume ();

	    if (value && *value)
	    {
		switch (value[0])
		{
		case '+':
		    volume += atoi (value + 1);
		    break;

		case '-':
		    volume -= atoi (value + 1);
		    break;

		case '*':
		    volume *= atoi (value + 1);
		    break;

		case '/':
		    volume /= atoi (value + 1);
		    break;

		default:
		    volume = atoi (value);
		}

		volume = rplay_audio_set_volume (volume);
	    }

	    if (volume >= 0)
	    {
		SNPRINTF (SIZE(buf, sizeof(buf)), "volume=%d", volume);
		strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
		b->nbytes += strlen(buf);
		volume_changed++;
	    }
	    else
	    {
		SNPRINTF (SIZE(buf, sizeof(buf)), "volume=-1");
		strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
		b->nbytes += strlen(buf);
	    }
	}
	else if (strcmp (name, "priority-threshold") == 0) /* `priority-threshold=value' */
	{
	    if (value && *value)
	    {
		rplay_priority_threshold = atoi (value);
		if (rplay_priority_threshold < RPLAY_MIN_PRIORITY)
		{
		    rplay_priority_threshold = RPLAY_MIN_PRIORITY;
		}
		else if (rplay_priority_threshold > RPLAY_MAX_PRIORITY)
		{
		    rplay_priority_threshold = RPLAY_MAX_PRIORITY;
		}

		priority_threshold_changed++;
	    }
	    SNPRINTF (SIZE(buf, sizeof(buf)), "priority-threshold=%d",
			   rplay_priority_threshold);
	    strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
	    b->nbytes += strlen(buf);
	}
	else if (strcmp (name, "notify-rate") == 0) /* `notify-rate=value' */
	{
	    if (value && *value)
	    {
		int i;
		double rate;
		
	    	rate = atof (value);
		
		for (i = 0; i < NOTIFY_RATE_MAX; i++)
		{
		    c->notify_rate[i].rate = rate;
		    c->notify_rate[i].next = 0.0;
		}
	    }
	    
	    SNPRINTF (SIZE(buf, sizeof(buf)), "level-notify-rate=%.2f position-notify-rate=%.2f",
		      c->notify_rate[NOTIFY_RATE_LEVEL].rate,
		      c->notify_rate[NOTIFY_RATE_POSITION].rate);
	    strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
	    b->nbytes += strlen(buf);
	}
	else if (strcmp (name, "level-notify-rate") == 0)
	{
	    if (value && *value)
	    {
		c->notify_rate[NOTIFY_RATE_LEVEL].rate = atof (value);
		c->notify_rate[NOTIFY_RATE_LEVEL].next = 0.0;
	    }

	    SNPRINTF (SIZE(buf, sizeof(buf)), "level-notify-rate=%.2f",
		      c->notify_rate[NOTIFY_RATE_LEVEL].rate);
	    strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
	    b->nbytes += strlen(buf);
	}
	else if (strcmp (name, "position-notify-rate") == 0)
	{
	    if (value && *value)
	    {
		c->notify_rate[NOTIFY_RATE_POSITION].rate = atof (value);
		c->notify_rate[NOTIFY_RATE_POSITION].next = 0.0;
	    }

	    SNPRINTF (SIZE(buf, sizeof(buf)), "position-notify-rate=%.2f",
		      c->notify_rate[NOTIFY_RATE_POSITION].rate);
	    strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
	    b->nbytes += strlen(buf);
	}
	else if (strcmp (name, "audio-port") == 0)
	{
	    int n;
	    
	    optional_port = rplay_audio_port;
	    
	    if (strstr (value, "none"))
	    {
		optional_port = 0;
	    }
	    if (strstr (value, "speaker"))
	    {
		SET_BIT (optional_port, RPLAY_AUDIO_PORT_SPEAKER);
	    }
	    if (strstr (value, "headphone"))
	    {
		SET_BIT (optional_port, RPLAY_AUDIO_PORT_HEADPHONE);
	    }
	    if (strstr (value, "lineout"))
	    {
		SET_BIT (optional_port, RPLAY_AUDIO_PORT_LINEOUT);
	    }

	    if (optional_port != rplay_audio_port)
	    {
		n = rplay_audio_init ();
		if (n >= 0)
		{
		    audio_port_changed++;
		}
	    }
	    else
	    {
		n = 0;
	    }
	    
	    if (n < 0)
	    {
		SNPRINTF (SIZE(buf, sizeof(buf)), "audio-port=-1");
		strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
		b->nbytes += strlen(buf);
	    }
	    else
	    {
		SNPRINTF (SIZE(buf, sizeof(buf)), "audio-port=%s", value);
		strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
		b->nbytes += strlen(buf);
	    }
	}
	else if (strcmp (name, "audio-info") == 0)
	{
	    char info[1024], *p;
	    SPOOL *sp, *sp_next;
	    int bufsize, rate;

	    /* Stop all sounds. */
	    for (sp = spool; sp; sp = sp_next)
	    {
		sp_next = sp->next;
		spool_stop (sp);
	    }
	    
	    /* Example: ulaw,8000,8,1 */
	    strncpy (info, value, sizeof(info));
	    p = strtok (info, ", ");
	    if (p) optional_format = string_to_audio_format (p);
	    p = strtok (NULL, ",");
	    if (p) optional_sample_rate = atoi (p);
	    p = strtok (NULL, ",");
	    if (p) optional_precision = atoi (p);
	    p = strtok (NULL, ",");
	    if (p) optional_channels = atoi (p);

#ifndef HAVE_OSS	    
	    bufsize = rplay_audio_bufsize; /* XXX: force recalculation */
	    rplay_audio_bufsize = 0;
#endif /* !HAVE_OSS */	    

	    rate = rplay_audio_rate;
	    rplay_audio_rate = 0;
	    
	    if (rplayd_audio_init () < 0)
	    {
		SNPRINTF (SIZE(buf, sizeof(buf)), "audio-info=-1");
		strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
		b->nbytes += strlen(buf);
#ifndef HAVE_OSS	    
		rplay_audio_bufsize = bufsize;
#endif /* !HAVE_OSS */	    
		rplay_audio_rate = rate;
	    }
	    else
	    {
		SNPRINTF (SIZE(buf, sizeof(buf)), "audio-info=%s", value);
		strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
		b->nbytes += strlen(buf);
	    }
	}
	else
	{
	    SNPRINTF (SIZE(buf, sizeof(buf)), "%s=-1", name);
	    strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
	    b->nbytes += strlen(buf);
	}

	SNPRINTF (SIZE(buf, sizeof(buf)), " ");
	strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
	b->nbytes += strlen(buf);
    }

    SNPRINTF (SIZE(buf, sizeof(buf)), "command=set client-data=\"%s\"\r\n",
	     client_data);
    strncat(b->buf + b->nbytes, buf, BUFFER_SIZE - b->nbytes);
    printf(buf);
    b->nbytes += strlen(buf);
    printf(b->buf);

    e = event_create (EVENT_WRITE, b);
    event_insert (c, e);

    if (notify_enabled)
    {
	/* Notify the current status.  */
	for (sp = spool; sp; sp = sp->next)
	{
	    switch (sp->state)
	    {
	    case SPOOL_PLAY:
		connection_notify (c, NOTIFY_PLAY, sp);
		break;

	    case SPOOL_PAUSE:
		connection_notify (c, NOTIFY_PAUSE, sp);
		break;
	    }
	}
	connection_notify (c, NOTIFY_VOLUME, rplay_audio_volume);

	e = event_create (EVENT_NOTIFY);
	event_insert (c, e);
    }
    if (volume_changed)
    {
	connection_notify (0, NOTIFY_VOLUME, rplay_audio_volume);
    }
    if (priority_threshold_changed || audio_port_changed)
    {
	connection_notify (0, NOTIFY_STATE);
    }

    connection_want_level_notify = 0;
    for (c = connections; c; c = c->next)
    {
	if (BIT (c->notify_mask, NOTIFY_LEVEL)
	    || (c->event && BIT (c->event->wait_mask, NOTIFY_LEVEL)))
	{
	    connection_want_level_notify++;
	}
    }

    return 0;
}

#ifdef __STDC__
static int
command_modify (CONNECTION *c, int argc, char **argv)
#else
static int
command_modify (c, argc, argv)
    CONNECTION *c;
    int argc;
    char **argv;
#endif
{
    SPOOL *sp;
    int id = 0;
    char *p;
    int new_sample_rate = -1;
    int new_volume = -1;
    int new_count = -1;
    int new_priority = -1;
    int new_list_count = -1;
    char *new_client_data = NULL;
    int modified, total_modified = 0;
    
#ifdef AUTH
    if (!host_access (c->sin, HOST_EXECUTE))
    {
	report (REPORT_NOTICE, "%s modify access denied\n", inet_ntoa (c->sin.sin_addr));
	connection_reply (c, "%cerror=\"access denied\" command=modify", RPTP_ERROR);
	return 0;
    }
#endif /* AUTH */

    p = rptp_parse (command_buffer, "id");
    if (!p || !*p)
    {
	connection_reply (c, "%cerror=\"missing `id'\" command=modify", RPTP_ERROR);
	return 0;
    }
    id = atoi (p + 1);

    p = rptp_parse (0, "count");
    if (p)
    {
	new_count = atoi (p);
    }

    p = rptp_parse (0, "list-count");
    if (p)
    {
	new_list_count = atoi (p);
    }

    p = rptp_parse (0, "priority");
    if (p)
    {
	new_priority = atoi (p);
    }
    
    p = rptp_parse (0, "sample-rate");
    if (p)
    {
	new_sample_rate = atoi (p);
    }

    p = rptp_parse (0, "volume");
    if (p)
    {
	new_volume = atoi (p);
    }

    p = rptp_parse (0, "client-data");
    if (p)
    {
	new_client_data = p;
    }
    
    for (sp = spool; sp; sp = sp->next)
    {
	if (!id || sp->id == id)
	{
	    modified = 0;
	    if (new_count != -1)
	    {
		spool_set_count (sp, new_count);
		modified++;
	    }
	    if (new_list_count != -1)
	    {
		spool_set_list_count (sp, new_list_count);
		modified++;
	    }
	    if (new_priority != -1)
	    {
		spool_set_priority (sp, new_priority);
		modified++;
	    }
	    if (new_sample_rate != -1)
	    {
		spool_set_sample_rate (sp, new_sample_rate);
		modified++;
	    }
	    if (new_volume != -1)
	    {
		spool_set_volume (sp, new_volume);
		modified++;
	    }
	    if (new_client_data)
	    {
		spool_set_client_data (sp, new_client_data);
		modified++;
	    }

	    if (modified)
	    {
		connection_notify (c, NOTIFY_MODIFY, sp);
		total_modified++;
	    }
	}
    }

    if (total_modified)
    {
	connection_reply (c, "%cmessage=\"modified\" command=modify", RPTP_OK);
    }
    else
    {
	connection_reply (c, "%cerror=\"nothing modified\" command=modify", RPTP_ERROR);
    }

    return 0;
}
