/* $Id: helper.c,v 1.5 2002/02/08 22:11:13 lmoore Exp $ */

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

#ifdef HAVE_HELPERS

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "rplayd.h"
#include "helper.h"
#include "misc.h"

/* Make sure MAXPATHLEN is defined. */
#ifndef MAXPATHLEN
#define MAXPATHLEN 4096
#endif

HELPER *helpers = NULL;
static time_t helper_read_time;

#ifdef __STDC__
void
helper_read(char *filename)
#else
void
helper_read(filename)
    char *filename;
#endif
{
    FILE *fp;
    char buf[MAXPATHLEN];
    char *pat, *prog, *p, *info;
    HELPER *hp, *hp_next, *tail;
    int line = 0;
    int error;

    helper_read_time = time(0);

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
	return;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
	line++;

	switch (buf[0])
	{
	case '#':
	case ' ':
	case '\t':
	case '\n':
	    continue;
	}
	p = strchr(buf, '\n');
	if (p)
	{
	    *p = '\0';
	}

	hp = (HELPER *) malloc(sizeof(HELPER));
	if (hp == NULL)
	{
	    report(REPORT_ERROR, "helper_read: out of memory\n");
	    done(1);
	}
	hp->next = NULL;
	hp->program = NULL;
	hp->format = 0;
	hp->sample_rate = 0;
	hp->precision = 0;
	hp->channels = 0;
	hp->byte_order = 0;

	/* pattern */
	pat = strtok(buf, " \t");
	if (!pat)
	{
	    report(REPORT_ERROR, "helper_read: parse error line %d\n", line);
	    done(1);
	}
	//memset ((char *) &hp->pattern, 0, sizeof (hp->pattern));
	error = regcomp(&hp->pattern, pat, REG_ICASE | REG_NOSUB);
	if (error)
	{
	    report(REPORT_ERROR, "helper_read: %d line %d\n", error, line);
	    done(1);
	}

	/* info */
	info = strtok(NULL, " \t");
	if (!info)
	{
	    report(REPORT_ERROR, "helper_read: parse error line %d\n", line);
	    done(1);
	}
	for (; *info && (*info == ' ' || *info == '\t'); info++) ;

	/* program */
	prog = strtok(NULL, "");
	if (!prog)
	{
	    report(REPORT_ERROR, "helper_read: parse error line %d\n", line);
	    done(1);
	}
	for (; *prog && (*prog == ' ' || *prog == '\t'); prog++) ;
	hp->program = strdup(prog);

	/* parse info */
	p = strtok(info, ",");
	if (p)
	    hp->format = string_to_audio_format(p);
	p = strtok(NULL, ",");
	if (p)
	    hp->sample_rate = atoi(p);
	p = strtok(NULL, ",");
	if (p)
	    hp->precision = atoi(p);
	p = strtok(NULL, ",");
	if (p)
	    hp->channels = atoi(p);
	p = strtok(NULL, ",");
	if (p)
	    hp->byte_order = string_to_byte_order(p);
	if (!hp->format || !hp->sample_rate || !hp->precision || !hp->channels || !hp->byte_order)
	{
	    report(REPORT_ERROR, "helper_read: parse error line %d\n", line);
	    done(1);
	}

	report(REPORT_DEBUG, "adding helper for \"%s\"\n", pat);

	if (helpers == NULL)
	{
	    helpers = hp;
	    tail = hp;
	}
	else
	{
	    tail->next = hp;
	    tail = hp;
	}
    }

    fclose(fp);
}

#ifdef __STDC__
HELPER *
helper_lookup(char *sound)
#else
HELPER *
helper_lookup(sound)
    char *sound;
#endif
{
    HELPER *hp;

    for (hp = helpers; hp; hp = hp->next)
    {
	if (regexec(&hp->pattern, sound, 0, 0, 0) == 0)
	{
	    return hp;
	}
    }
    return NULL;
}

#ifdef __STDC__
void
helper_reread(char *filename)
#else
void
helper_reread(filename)
    char *filename;
#endif
{
    HELPER *hp, *hp_next;

    report(REPORT_DEBUG, "re-reading helpers\n");

    for (hp = helpers; hp; hp = hp_next)
    {
	hp_next = hp->next;
	regfree(&hp->pattern);
	free(hp->program);
    }

    helpers = NULL;
    helper_read(filename);
}

#ifdef __STDC__
void
helper_stat(char *filename)
#else
void
helper_stat(filename)
    char *filename;
#endif
{
    if (modified(filename, helper_read_time))
    {
	helper_reread(filename);
    }
}

#endif /* HAVE_HELPERS */
