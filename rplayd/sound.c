/* $Id: sound.c,v 1.10 1999/06/09 06:27:44 boyns Exp $ */

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
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#ifdef ultrix
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/param.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "sound.h"
#include "rplayd.h"
#include "cache.h"
#include "ulaw.h"
#include "xhash.h"
#include "server.h"
#include "buffer.h"
#include "connection.h"
#include "misc.h"
#include "strdup.h"
#include "spool.h"
#ifdef HAVE_CDROM
#include "cdrom.h"
#endif /* HAVE_CDROM */
#ifdef HAVE_HELPERS
#include "helper.h"
#endif /* HAVE_HELPERS */
#ifdef HAVE_RX_RXPOSIX_H
#include <rx/rxposix.h>
#else
#ifdef HAVE_RXPOSIX_H
#include <rxposix.h>
#else
#include "rxposix.h"
#endif
#endif

SOUND *sounds = NULL;
int sound_count = 0;
static time_t sound_read_time;

int sound_cache_size = 0;
int sound_cache_max_sound_size = MEMORY_CACHE_SOUND_SIZE;
int sound_cache_max_size = MEMORY_CACHE_SIZE;

#ifdef BAD_DIRS
static regex_t bad_dirs;

/* Prepare `bad_dirs'.  */
static void
bad_dirs_init()
{
    static char *buf;
    char *p;
    char *dirs;
    int first, length;

    dirs = strdup(BAD_DIRS);	/* XXX */

    length = strlen("^\\(") + strlen("\\)") + strlen(dirs) + 1;

    //length += strlen ("^");
    for (p = dirs; *p; p++)
    {
	if (*p == ':')
	{
	    length += strlen("\\|") - strlen(":");
	}
    }

    if (buf)
    {
	free(buf);
    }
    buf = (char *) malloc(length);
    if (buf == NULL)
    {
	report(REPORT_ERROR, "bad_dir_init: out of memory\n");
	done(1);
    }

    first = 1;
    strcpy(buf, "^\\(");
    while (p = (char *) strtok(first ? dirs : 0, ":"))
    {
	if (first)
	{
	    //strcat (buf, "^");
	    first = 0;
	}
	else
	{
	    //strcat (buf, "\\|^");
	    strcat(buf, "\\|");
	}
	strcat(buf, p);
    }
    strcat(buf, "\\)");

#if 0
    report(REPORT_DEBUG, "bad_dirs=%s, strlen=%d, length=%d\n",
	   buf, strlen(buf), length);
#endif

    //memset ((char *) &bad_dirs, 0, sizeof (bad_dirs));

    if (regncomp(&bad_dirs, buf, strlen(buf), REG_ICASE | REG_NOSUB))
    {
	report(REPORT_ERROR, "bad_dirs: regncomp failed\n");
	done(1);
    }

    free(dirs);
}

#ifdef __STDC__
static int
bad_dir(char *dir)
#else
static int
bad_dir(dir)
    char *dir;
#endif
{
    /* return 1 if bad */
    return regnexec(&bad_dirs, dir, strlen(dir), 0, 0, 0) ? 0 : 1;
}

#endif /* BAD_DIRS */

#ifdef __STDC__
void
sound_read_directory(char *dirname)
#else
void
sound_read_directory(dirname)
    char *dirname;
#endif
{
    char soundname[MAXPATHLEN];
    struct dirent *dp;
    DIR *dir = opendir(dirname);
    if (dir == NULL)
    {
	report(REPORT_ERROR, "opendir %s: %s", sys_err_str(errno));
	return;
    }

    while ((dp = readdir(dir)) != NULL)
    {
	if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
	{
	    continue;
	}
	sprintf(soundname, "%s/%s", dirname, dp->d_name);
	sound_insert(soundname, SOUND_READY, SOUND_FILE);
    }

    closedir(dir);
}


/*
 * read the rplay configuration file and load the hash table
 */
#ifdef __STDC__
void
sound_read(char *filename)
#else
void
sound_read(filename)
    char *filename;
#endif
{
    FILE *fp;
    char buf[MAXPATHLEN], *p;
    struct stat st;

    xhash_init(MAX_SOUNDS);

#ifdef BAD_DIRS
    bad_dirs_init();
#endif

#ifdef HAVE_CDROM
    {
	/* Insert the CDROM devices into the sound list. */
	int i;
	for (i = 0; i < MAX_CDROMS; i++)
	{
	    sound_insert(cdrom_table[i].name, SOUND_READY, SOUND_CDROM);
	}
    }
#endif /* HAVE_CDROM */

    sound_read_time = time(0);

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
	report(REPORT_NOTICE, "warning: cannot open %s\n", filename);
	/*
	 * no local sounds
	 */
	return;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
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

	if (stat(buf, &st) < 0)
	{
	    report(REPORT_ERROR, "sound_read: %s: %s\n", buf, sys_err_str(errno));
	    continue;
	}

	if (S_ISDIR(st.st_mode))
	{
	    sound_read_directory(buf);
	}
	else
	{
	    sound_insert(buf, SOUND_READY, SOUND_FILE);
	}
    }

    fclose(fp);
}

#ifdef __STDC__
static char *
destroy(char *hash_string, char *hash_value)
#else
static char *
destroy(hash_string, hash_value)
    char *hash_string;
    char *hash_value;
#endif
{
    SOUND *s = (SOUND *) hash_value;

    sound_free(s);
    free((char *) s);

    return NULL;
}

/*
 * Re-read the rplay configuration file and reload the hash table.
 */
#ifdef __STDC__
void
sound_reread(char *filename)
#else
void
sound_reread(filename)
    char *filename;
#endif
{
    SOUND *s1, *s1_next, *s2, *s2_next;

    report(REPORT_DEBUG, "re-reading sounds\n");

    /*
     * Delete all the sounds.
     */
#if 1
    for (s1 = sounds; s1; s1 = s1_next)
    {
	s1_next = s1->list;
	for (s2 = s1; s2; s2 = s2_next)
	{
	    s2_next = s2->next;
	    /* xhash_delete(s2->hash_key); */
	    sound_free(s2);
	    free((char *) s2);
	}
    }
#else
    xhash_apply(destroy);
#endif

    xhash_die();
    xhash_init(MAX_SOUNDS);

    sounds = NULL;
    sound_count = 0;

    /*
     * and now load the sounds
     */
    sound_read(filename);

    /*
     * don't forget the cache...
     */
    cache_read();
}

#ifdef __STDC__
void
sound_stat(char *filename)
#else
void
sound_stat(filename)
    char *filename;
#endif
{
    if (modified(filename, sound_read_time))
    {
	sound_reread(filename);
    }
}

BUFFER *
sound_list_create()
{
    SOUND *s;
    int n;
    BUFFER *start, *b;
    char line[RPTP_MAX_LINE];

    b = buffer_create();
    start = b;
    SNPRINTF(SIZE(b->buf + strlen(b->buf), BUFFER_SIZE), "+message=\"sounds\"\r\n");
    b->nbytes += strlen(b->buf);

    for (s = sounds; s; s = s->list)
    {
	if (s->status != SOUND_READY)
	{
	    continue;
	}

	if (s->mapped)
	{
	    SNPRINTF(SIZE(line, sizeof(line)), "\
sound=\"%s\" size=%d bits=%g sample_rate=%d channels=%d samples=%d\r\n",
		     s->name, s->size, s->input_precision, s->sample_rate, s->channels,
		     s->samples);
	}
	else
	{
	    SNPRINTF(SIZE(line, sizeof(line)), "sound=\"%s\"\r\n", s->name);
	}

	n = strlen(line);
	if (b->nbytes + n > BUFFER_SIZE)
	{
	    b->next = buffer_create();
	    b = b->next;
	}
	strncat(b->buf + b->nbytes, line, BUFFER_SIZE - b->nbytes);
	b->nbytes += n;
    }

    if (b->nbytes + 3 > BUFFER_SIZE)
    {
	b->next = buffer_create();
	b = b->next;
    }

    SNPRINTF(SIZE(b->buf + b->nbytes, BUFFER_SIZE - b->nbytes), ".\r\n");
    b->nbytes += 3;

    return start;
}

#ifdef __STDC__
SOUND *
sound_insert(char *path, int status, int type)
#else
SOUND *
sound_insert(path, status, type)
    char *path;
    int status;
    int type;
#endif
{
    SOUND *s;
    char *hash_val;

#ifdef BAD_DIRS
    if (bad_dir(path))
    {
	report(REPORT_NOTICE, "bad_dir: attempt to load %s\n", path);
	return NULL;
    }
#endif

    s = sound_create();
    if (s == NULL)
    {
	report(REPORT_ERROR, "sound_insert: out of memory\n");
	done(1);
    }

    s->type = type;

    s->path = strdup(path);
    if (s->path == NULL)
    {
	report(REPORT_ERROR, "sound_insert: out of memory\n");
	done(1);
    }
    s->hash_key = strdup(xhash_name(s->path));
    if (s->hash_key == NULL)
    {
	report(REPORT_ERROR, "sound_insert: out of memory\n");
	done(1);
    }
    s->name = s->path[0] == '/' ? strrchr(s->path, '/') + 1 : s->path;
    s->status = status;

    hash_val = xhash_get(s->hash_key);
    if (hash_val == NULL)
    {
	xhash_put(s->hash_key, (char *) s);

	s->list_prev = NULL;
	s->list = sounds;
	if (sounds)
	{
	    sounds->list_prev = s;
	}
	sounds = s;
    }
    else
    {
	SOUND *ss, *prev = NULL;

	for (ss = (SOUND *) hash_val; ss; prev = ss, ss = ss->next)
	{
	    if (strcmp(ss->path, s->path) == 0)
	    {
		free((char *) s);
		return ss;
	    }
	}
	prev->next = s;
	s->prev = prev;
    }

    return s;
}

/*
 * create a sound
 */
#ifdef __STDC__
SOUND *
sound_create(void)
#else
SOUND *
sound_create()
#endif
{
    SOUND *s;

    s = (SOUND *) malloc(sizeof(SOUND));
    if (s == NULL)
    {
	return s;
    }
    s->list = NULL;
    s->list_prev = NULL;
    s->next = NULL;
    s->prev = NULL;
    s->type = 0;
    s->storage = SOUND_STORAGE_NULL;
    s->path = NULL;
    s->name = NULL;
    s->hash_key = NULL;
    s->status = SOUND_NULL;
    s->count = 0;
    s->format = RPLAY_FORMAT_NONE;
    s->byte_order = 0;
    s->sample_rate = 0;
    s->input_precision = 0;
    s->output_precision = 0;
    s->channels = 0;
    s->samples = 0;
    s->input_sample_size = 0;
    s->output_sample_size = 0;
    s->offset = 0;
    s->size = 0;
    s->tail = 0;
    s->chunk_size = 0;
    s->mapped = 0;
    s->cache = NULL;
    s->flow = NULL;
    s->flowp = &s->flow;
#ifdef HAVE_CDROM
    s->starting_track = 0;
    s->ending_track = 0;
#endif /* HAVE_CDROM */
#ifdef HAVE_HELPERS
    s->needs_helper = 0;
#endif /* HAVE_HELPERS */

    return s;
}

/*
 * Lookup the sound name in the hash table and map the sound file if
 * necessary.
 */
#ifdef __STDC__
SOUND *
sound_lookup(char *name, int mode, SERVER *lookup_server)
#else
SOUND *
sound_lookup(name, mode, lookup_server)
    char *name;
    int mode;
    SERVER *lookup_server;
#endif
{
    SOUND *s = NULL;
    int has_extension = 0, has_pathname = 0;
    struct stat st;
    int i;

#ifdef HAVE_CDROM
    /* Map sounds with cdrom prefixes to the cdrom itself. */
    for (i = 0; i < MAX_CDROMS; i++)
    {
	if (strncmp(name, cdrom_table[i].name, strlen(cdrom_table[i].name)) == 0)
	{
	    break;
	}
    }
    if (i < MAX_CDROMS)
    {
	s = (SOUND *) xhash_get(xhash_name(cdrom_table[i].name));
    }
#endif /* HAVE_CDROM */

    if (s == NULL)
    {
	s = (SOUND *) xhash_get(xhash_name(name));
    }

    if (s == NULL)
    {
	report(REPORT_DEBUG, "%s not in hash table\n", name);

	if (mode == SOUND_DONT_FIND || mode == SOUND_DONT_COUNT)
	{
	    return NULL;
	}

	/*
	 * see if a local file can be loaded (mode == SOUND_LOAD)
	 */
	if (stat(name, &st) == 0)
	{
	    report(REPORT_DEBUG, "loading local file %s\n", name);
	    s = sound_insert(name, SOUND_READY, SOUND_FILE);
	    if (s == NULL)
	    {
		return NULL;
	    }
	}
#if 0
	/* virtual sounds cause searching problems */
#ifdef HAVE_HELPERS
	else if (mode == SOUND_CREATE && helper_lookup(name))
	{
	    s = sound_insert(name, SOUND_READY, SOUND_VIRTUAL);
	}
#endif
#endif
	else if (mode == SOUND_FIND)
	{
	    if (name[0] == '/')
	    {
		report(REPORT_DEBUG, "not searching for %s\n", name);
		return NULL;
	    }
	    if (lookup_server)
	    {
		report(REPORT_DEBUG, "searching for %s\n", name);
		s = sound_insert(cache_name(name), SOUND_SEARCH, SOUND_FILE);
		if (s == NULL)
		{
		    return NULL;
		}
		connection_server_open(lookup_server, s);
	    }
	    else if (servers)
	    {
		report(REPORT_DEBUG, "searching for %s\n", name);
		s = sound_insert(cache_name(name), SOUND_SEARCH, SOUND_FILE);
		if (s == NULL)
		{
		    return NULL;
		}
		connection_server_open(servers, s);
	    }
	    else
	    {
		return NULL;
	    }
	}
	else
	{
	    return NULL;
	}
    }
    else
    {
	/*
	 * Make sure the sound found has the correct path and dot
	 * extension.
	 */
	if (name[0] == '/')
	{
	    has_pathname++;
	}
	else if (strrchr(name, '.'))
	{
	    has_extension++;
	}
	for (; s; s = s->next)
	{
	    if (has_pathname && (!s->path || strcmp(name, s->path) != 0))
	    {
		continue;
	    }
	    if (has_extension && (!s->path || strcmp(name, s->name) != 0))
	    {
		continue;
	    }
	    break;
	}
	if (s == NULL)
	{
	    if (has_pathname)
	    {
		report(REPORT_DEBUG, "`%s' file name not in hash table\n", name);
		if (stat(name, &st) == 0)
		{
		    report(REPORT_DEBUG, "loading local file `%s'\n", name);
		    s = sound_insert(name, SOUND_READY, SOUND_FILE);
		    if (s == NULL)
		    {
			return NULL;
		    }
		}
		else
		{
		    return NULL;
		}
	    }
	    else if (has_extension)
	    {
		report(REPORT_DEBUG, "`%s' extension not in hash table\n", name);
		return NULL;
	    }
	}
    }

    if (mode == SOUND_DONT_COUNT)
    {
	return s;
    }
    else
    {
	sound_count++;
	s->count = sound_count;
    }

    switch (s->status)
    {
    case SOUND_NOT_READY:
    case SOUND_SEARCH:
	return s;
    }

    if (!s->mapped)
    {
	return sound_map(s) == 0 ? s : NULL;
    }
    else
    {
	return s;
    }
}

/*
 * Map a sound using its header or file extension.
 */
#ifdef __STDC__
int
sound_map(SOUND *s)
#else
int
sound_map(s)
    SOUND *s;
#endif
{
    SINDEX *si;
    BUFFER *b;
    int n, storage;
    int optional_offset;
    int optional_format;
    int optional_byte_order;
    int optional_sample_rate;
    float optional_input_precision;
    int optional_output_precision;
    int optional_channels;
    int optional_storage;
    char buf[SOUND_MAX_HEADER_SIZE];
    int sound_chunk_size = 0;
#ifdef HAVE_HELPERS
    HELPER *helper = NULL;
#endif /* HAVE_HELPERS */

    if (s->mapped)
    {
	return 0;
    }

    /* Save the optional settings. */
    optional_offset = s->offset;
    optional_format = s->format;
    optional_byte_order = s->byte_order;
    optional_sample_rate = s->sample_rate;
    optional_input_precision = s->input_precision;
    optional_output_precision = s->output_precision;
    optional_channels = s->channels;
    optional_storage = s->storage;

#ifdef HAVE_HELPERS
    helper = helper_lookup(s->path);
    if (helper)
    {
	s->needs_helper++;
	strcpy(buf, "HELPER");	/* XXX: not used; see below */
    }
    else
#endif /* HAVE_HELPERS */
#ifdef HAVE_CDROM
    if (s->type == SOUND_CDROM)
    {
	strcpy(buf, "CDROM");
    }
    else
#endif /* HAVE_CDROM */
    {
	si = sound_open(s, 1);
	if (si == NULL)
	{
	    return -1;
	}

	/* Trick sound_fill to not free any buffers used to determine what
	   sort of sound this is. */
	storage = s->storage;
	s->storage = SOUND_STORAGE_MEMORY;
	b = buffer_alloc(SOUND_MAX_HEADER_SIZE, BUFFER_FREE);
	n = sound_fill(si, b, 1);
	s->storage = storage;
	if (n <= 0)
	{
	    report(REPORT_DEBUG, "sound_map: sound_fill %d\n", n);
	    buffer_dealloc(b, 1);
	    return -1;
	}

	sound_close(si);

	/* Copy the buffer contents to `buf' so `b' can be destroyed. */
	memcpy(buf, b->buf, MIN(sizeof(buf), b->nbytes));
	buffer_dealloc(b, 1);
    }

    /* u-law */
    if (strncmp(buf, ".snd", 4) == 0)
    {
	long ulaw_hdr_size;
	int encoding;
	int data_size;
	char *p;

	p = buf + 4;

	ulaw_hdr_size = big_long(p);
	p += 4;
	if (ulaw_hdr_size < ULAW_HDRSIZE)
	{
	    report(REPORT_DEBUG, "%s not a ulaw file\n", s->path);
	    return -1;
	}

	/*
	 * The audio data size which may be ~0.
	 * This value is ignored.
	 */
	data_size = big_long(p);
	p += 4;

	encoding = big_long(p);
	p += 4;
	switch (encoding)
	{
	case ULAW_MULAW_8:
	    s->format = RPLAY_FORMAT_ULAW;
	    s->input_precision = 8;
	    s->output_precision = 8;
	    break;

	case ULAW_LINEAR_8:
	    s->format = RPLAY_FORMAT_LINEAR_8;
	    s->input_precision = 8;
	    s->output_precision = 8;
	    break;

	case ULAW_LINEAR_16:
	    s->format = RPLAY_FORMAT_LINEAR_16;
	    s->input_precision = 16;
	    s->output_precision = 16;
	    break;

#ifdef HAVE_ADPCM
	case ULAW_G721:
	    s->format = RPLAY_FORMAT_G721;
	    s->input_precision = 4;
	    s->output_precision = 16;	/* uncompressed to 16-bit */
	    break;

	case ULAW_G723_3:
	    s->format = RPLAY_FORMAT_G723_3;
	    s->input_precision = 3;
	    s->output_precision = 16;	/* uncompressed to 16-bit */
	    break;

	case ULAW_G723_5:
	    s->format = RPLAY_FORMAT_G723_5;
	    s->input_precision = 5;
	    s->output_precision = 16;	/* uncompressed to 16-bit */
	    break;
#endif /* HAVE_ADPCM */

	default:
	    report(REPORT_DEBUG, "%s: %d - unsupported ulaw encoding\n",
		   s->path, encoding);
	    return -1;
	}

	/*
	 * Byte order.
	 */
	s->byte_order = RPLAY_BIG_ENDIAN;

	/*
	 * Sample rate.
	 */
	s->sample_rate = big_long(p);
	p += 4;

	/*
	 * Number of channels.
	 */
	s->channels = big_long(p);
	p += 4;

	s->offset = ulaw_hdr_size;

	if (data_size == ~0)
	{
	    sound_chunk_size = s->size - s->offset;
	}
	else
	{
	    sound_chunk_size = data_size;
	}
    }
    /* aiff */
    else if (strncmp(buf, "FORM", 4) == 0)
    {
	unsigned long total_size;
	unsigned long chunk_size;
	unsigned long frames;
	unsigned long offset;
	unsigned long block_size;
	double rate;
	int channels;
	int bits;
	char id[5];
	char *p;

	p = buf + 4;

	total_size = big_long(p);
	p += 4;

	if (memcmp(p, "AIFF", 4) != 0)
	{
	    return -1;
	}
	p += 4;

	/*
	 * Scan the chunks.
	 */
	for (;;)
	{
	    /*
	     * Chunk id.
	     */
	    memcpy(id, p, 4);
	    id[4] = '\0';
	    p += 4;

	    /*
	     * Chunk size.
	     */
	    chunk_size = big_long(p);
	    p += 4;

	    /*
	     * COMM Chunk.
	     */
	    if (memcmp(id, "COMM", 4) == 0)
	    {
		channels = big_short(p);
		p += 2;

		frames = big_long(p);
		p += 4;

		bits = big_short(p);
		p += 2;

		rate = ConvertFromIeeeExtended((unsigned char *) p);
		p += 10;
	    }
	    /*
	     * SSN Chunk.
	     */
	    else if (memcmp(id, "SSND", 4) == 0)
	    {
		offset = big_long(p);
		p += 4;

		block_size = big_long(p);
		sound_chunk_size = block_size;
		p += 4;
		break;
	    }
	    /*
	     * Ignore other chunks.
	     */
	    else
	    {
		p += chunk_size;
		/*
		 * skip the pad byte, if necessary
		 */
		if (chunk_size % 2 == 1)
		    p++;
	    }
	}

	/*
	 * Byte order.
	 */
	s->byte_order = RPLAY_BIG_ENDIAN;

	s->offset = p - buf;

	s->sample_rate = rate;
	s->input_precision = bits;
	s->output_precision = bits;
	s->channels = channels;

	switch ((int) s->input_precision)
	{
	case 8:
	    s->format = RPLAY_FORMAT_LINEAR_8;
	    break;

	case 16:
	    s->format = RPLAY_FORMAT_LINEAR_16;
	    break;

	default:
	    report(REPORT_ERROR, "unsupported aiff precision `%g'\n",
		   s->input_precision);
	    return -1;
	}
    }
    /* deadsnd files */
    else if (*((long *) &buf) == 0x4a02b6d2)
    {
	DSID *dsid = (DSID *) & buf;

	s->offset = sizeof(DSID);
	s->sample_rate = dsid->freq;
	s->input_precision = s->output_precision = dsid->bps;
	s->channels = (dsid->mode + 1);
	s->byte_order = RPLAY_LITTLE_ENDIAN;

	switch (dsid->bps)
	{
	case 8:
	    s->format = RPLAY_FORMAT_ULINEAR_8;
	    break;

	case 16:
	    s->format = RPLAY_FORMAT_LINEAR_16;
	    break;

	default:
	    report(REPORT_ERROR, "unsupported deadsnd precision `%d'\n", dsid->bps);
	    return -1;
	}
    }
    /* wave */
    else if (strncmp(buf, "RIFF", 4) == 0)
    {
	unsigned long chunk_size;
	short chunk_type;
	double rate;
	int channels;
	int bits;
	char id[5];
	char *p;

	p = buf + 4;
	p += 4;

	if (memcmp(p, "WAVE", 4) != 0)
	{
	    report(REPORT_ERROR, "wave file missing `WAVE' header\n");
	    return -1;
	}
	p += 4;

	/*
	 * Scan the chunks.
	 */
	for (;;)
	{
	    /*
	     * Chunk id.
	     */
	    memcpy(id, p, 4);
	    id[4] = '\0';
	    p += 4;

	    /*
	     * Chunk size.
	     */
	    chunk_size = little_long(p);
	    p += 4;

	    /*
	     * 'fmt ' Chunk.
	     */
	    if (memcmp(id, "fmt ", 4) == 0)
	    {
		char *start_of_chunk = p;
		chunk_type = little_short(p);
		p += 2;
		/*
		 * Only one wave format is supported.
		 */
		if (chunk_type == 0x0001)
		{
		    channels = little_short(p);
		    p += 2;
		    rate = little_long(p);
		    p += 4;
		    p += 4;	/* bytes/second ? */
		    p += 2;	/* block align ? */
		    bits = little_short(p);
		    p += 2;
		}
		else
		{
		    report(REPORT_ERROR, "unknown wave chunk `%x'\n", chunk_type);
		    return -1;
		}
		p = start_of_chunk + chunk_size;
	    }
	    /*
	     * data Chunk.
	     */
	    else if (memcmp(id, "data", 4) == 0)
	    {
		/* size */
		sound_chunk_size = chunk_size;
		break;
	    }
	    /*
	     * Ignore other chunks.
	     */
	    else
	    {
		report(REPORT_DEBUG, "ignoring chunk - %s (%d)\n",
		       id, chunk_size);
		p += chunk_size;
		/*
		 * skip the pad byte, if necessary
		 */
		if (chunk_size % 2 == 1)
		    p++;
	    }
	}

	s->offset = p - buf;
	s->sample_rate = rate;
	s->input_precision = bits;
	s->output_precision = bits;
	s->channels = channels;

	switch ((int) s->input_precision)
	{
	case 8:
	    s->format = RPLAY_FORMAT_ULINEAR_8;
	    break;

	case 16:
	    s->format = RPLAY_FORMAT_LINEAR_16;
	    break;

	default:
	    report(REPORT_ERROR, "unsupported wave precision `%g'\n",
		   s->input_precision);
	    return -1;
	}

	/*
	 * Byte order.
	 */
	s->byte_order = RPLAY_LITTLE_ENDIAN;
    }
    /* voc */
    else if (strncmp(buf, "Creative Voice File\032", 20) == 0)
    {
	char *p;
	short header_size;
	unsigned long block_length;
	unsigned char block_id, rate;

	p = buf + 20;

	header_size = little_short(p);	/* sizeof header */
	p += 2;

	/* major/minor version + checksum of version */
	p += 2 * sizeof(short);

	for (;;)
	{
	    block_id = *p++;

	    if (block_id == 0)	/* VOC_TERM */
	    {
		break;
	    }

	    /* Block length */
	    block_length = (unsigned char) *p++;
	    block_length |= ((unsigned char) *p++ << 8);
	    block_length |= ((unsigned char) *p++ << 16);

	    if (block_id == 1)	/* VOC_DATA */
	    {
		//sound_chunk_size = block_length;

		rate = *p++;
		s->input_precision = 8;
		s->output_precision = 8;
		s->sample_rate = 1000000.0 / (256 - rate);
		s->channels = 1;
		break;
	    }
	    else if (block_id == 9)	/* VOC_DATA_16 */
	    {
		//sound_chunk_size = block_length;

		s->sample_rate = little_long(p);
		p += 4;

		switch (*p)
		{
		case 8:
		    s->input_precision = 8;
		    s->output_precision = 8;
		    break;

		case 16:
		    s->input_precision = 16;
		    s->output_precision = 16;
		    break;

		default:
		    report(REPORT_ERROR, "unsupported voc_data_16 precision `%d'\n",
			   *p);
		    return -1;
		}
		*p++;

		s->channels = *p++;

		p++;		/* unknown */
		p++;		/* not used */
		p++;		/* not used */
		p++;		/* not used */
		p++;		/* not used */
		p++;		/* not used */

		break;
	    }

	    p += block_length;
	}

	s->offset = p - buf;
	s->byte_order = RPLAY_LITTLE_ENDIAN;

	switch ((int) s->input_precision)
	{
	case 8:
	    s->format = RPLAY_FORMAT_ULINEAR_8;
	    break;

	case 16:
	    s->format = RPLAY_FORMAT_ULINEAR_16;
	    break;

	default:
	    report(REPORT_ERROR, "unsupported voc precision `%d'\n",
		   s->input_precision);
	    return -1;
	}
    }
#ifdef HAVE_GSM
    /* gsm */
    else if (((*buf >> 4) & 0xf) == GSM_MAGIC)
    {
	s->offset = 0;
	s->format = RPLAY_FORMAT_GSM;
	s->byte_order = RPLAY_BIG_ENDIAN;	/* ??? */
	s->sample_rate = 8000;
	s->input_precision = 1.65;
	s->output_precision = 16;
	s->channels = 1;
    }
#endif /* HAVE_GSM */
#ifdef HAVE_CDROM
    else if (strcmp(buf, "CDROM") == 0)
    {
	s->offset = 0;
	s->format = RPLAY_FORMAT_LINEAR_16;
	s->byte_order = RPLAY_LITTLE_ENDIAN;
	s->sample_rate = 44100;
	s->input_precision = 16;
	s->output_precision = 16;
	s->channels = 2;
	s->storage = SOUND_STORAGE_NONE;
    }
#endif /* HAVE_CDROM */
    /* Unknown audio header -- try to use the extension. */
    else
    {
	char *p;

	report(REPORT_DEBUG, "%s missing sound header; trying helper or extension\n", s->path);
	p = strrchr(s->path, '.');
#ifdef HAVE_HELPERS
	if (s->needs_helper)
	{
	    s->offset = 0;
	    s->format = helper->format;
	    s->byte_order = helper->byte_order;
	    s->sample_rate = helper->sample_rate;
	    s->input_precision = helper->precision;
	    s->output_precision = helper->precision;
	    s->channels = helper->channels;
	    s->storage = SOUND_STORAGE_NONE;
	}
	else
#endif
	if (p && strcmp(p, ".ub") == 0)
	{
	    report(REPORT_DEBUG, "%s assuming ub\n", s->path);
	    s->offset = 0;
	    s->format = RPLAY_FORMAT_ULINEAR_8;
	    s->byte_order = RPLAY_BIG_ENDIAN;
	    s->sample_rate = 11025;
	    s->input_precision = 8;
	    s->output_precision = 8;
	    s->channels = 1;
	}
	else if (p && (strcmp(p, ".au") == 0 || strcmp(p, ".ul") == 0))
	{
	    report(REPORT_DEBUG, "%s assuming ulaw\n", s->path);
	    s->offset = 0;
	    s->format = RPLAY_FORMAT_ULAW;
	    s->byte_order = RPLAY_BIG_ENDIAN;
	    s->sample_rate = 8000;
	    s->input_precision = 8;
	    s->output_precision = 8;
	    s->channels = 1;
	}
#ifdef HAVE_GSM
	else if (p && (strcmp(p, ".gsm") == 0 || strcmp(p, ".GSM") == 0))
	{
	    report(REPORT_DEBUG, "%s assuming gsm\n", s->path);
	    s->offset = 0;
	    s->format = RPLAY_FORMAT_GSM;
	    s->byte_order = RPLAY_BIG_ENDIAN;	/* ??? */
	    s->sample_rate = 8000;
	    s->input_precision = 1.65;
	    s->output_precision = 16;
	    s->channels = 1;
	}
#endif /* HAVE_GSM */
	else if (!optional_format
		 && !optional_byte_order
		 && !optional_sample_rate
		 && !optional_input_precision
		 && !optional_channels)
	{
	    report(REPORT_ERROR, "`%s' unknown audio file\n", s->path);
	    return -1;
	}
    }

    if (optional_offset)
	s->offset = optional_offset;
    if (optional_format)
	s->format = optional_format;
    if (optional_byte_order)
	s->byte_order = optional_byte_order;
    if (optional_sample_rate)
	s->sample_rate = optional_sample_rate;
    if (optional_input_precision)
	s->input_precision = optional_input_precision;
    if (optional_output_precision)
	s->output_precision = optional_output_precision;
    if (optional_channels)
	s->channels = optional_channels;
    if (optional_storage)
	s->storage = optional_storage;

    switch (s->format)
    {
#ifdef HAVE_ADPCM
	/* G.72X files *must* have 16-bit output. */
    case RPLAY_FORMAT_G721:
    case RPLAY_FORMAT_G723_3:
    case RPLAY_FORMAT_G723_5:
	s->output_precision = 16;
	break;
#endif /* HAVE_ADPCM */

#ifdef HAVE_GSM
	/* Fix-up GSM info. */
    case RPLAY_FORMAT_GSM:
	s->output_precision = 16;
	s->input_precision = 1.65;
	s->byte_order = RPLAY_BIG_ENDIAN;
	s->channels = 1;
	s->offset = 0;
	break;
#endif /* HAVE_GSM */

    default:
	break;
    }

    if (!s->format || !s->byte_order || !s->sample_rate
	|| !s->input_precision || !s->output_precision || !s->channels)
    {
	report(REPORT_ERROR, "`%s' invalid audio parameters\n", s->path);
	return -1;
    }

    /* The input sample size is a floating point number since compressed
       samples can be smaller than 8 bits.  */
    /* XXX output_precision -> input_precision */
    s->input_sample_size = (s->input_precision / 8) * s->channels;
    s->output_sample_size = (s->output_precision >> 3) * s->channels;

    if (sound_chunk_size > 0)
    {
	/* Make sure sound_chunk_size is valid */
	if (s->size == 0 || sound_chunk_size < s->size)		/* size==0 for flows */
	{
	    s->chunk_size = sound_chunk_size;
	}
    }

    /* Calculate tail only if chunk_size is in sound header */
    if (s->chunk_size)
    {
	s->tail = s->offset + s->chunk_size;
    }

    /* Calculate chunk_size */
    if (!s->chunk_size && s->size)
    {
	s->chunk_size = s->size - s->offset;
    }

#if 0
    printf("SOUND: %s size=%d chunk=%d tail=%d offset=%d\n",
	   s->name, s->size, s->chunk_size, s->tail, s->offset);
#endif

    if (s->type != SOUND_FILE)
    {
	s->size = 0;
	s->samples = 0;
    }
#ifdef HAVE_HELPERS
    /* helper sounds should never have size and samples set */
    else if (s->needs_helper)
    {
	s->size = 0;
	s->samples = 0;
    }
#endif
    else
    {
	s->samples = number_of_samples(s, 0);	// NUMBER_OF_SAMPLES (s);

    }

    s->mapped = 1;

    report(REPORT_DEBUG, "\
%s input=%s bits=%g sample-rate=%d channels=%d samples=%d format=%s byte-order=%s\n",
	   s->name,
	   input_to_string(s->type),
	   s->input_precision,
	   s->sample_rate,
	   s->channels,
	   s->samples,
	   audio_format_to_string(s->format),
	   byte_order_to_string(s->byte_order));

    return 0;
}

/*
 * unmap a sound -- forget about it
 */
#ifdef __STDC__
int
sound_unmap(SOUND *s)
#else
int
sound_unmap(s)
    SOUND *s;
#endif
{
    if (s->type == SOUND_FILE)
    {
	if (s->cache)
	{
#ifdef HAVE_MMAP
	    munmap(s->cache, s->size);
#else /* not HAVE_MMAP */
	    free((char *) s->cache);
#endif /* not HAVE_MMAP */
	    s->cache = NULL;
	    sound_cache_size -= s->size;
	}
    }

    if (s->type == SOUND_FLOW || s->needs_helper)
    {
	buffer_dealloc(s->flow, 1);
	s->flow = NULL;
	s->flowp = &s->flow;
    }

    s->mapped = 0;

    return 0;
}

/* Free *all* memory used by a sound. */
#ifdef __STDC__
int
sound_free(SOUND *s)
#else
int
sound_free(s)
    SOUND *s;
#endif
{
    sound_unmap(s);

    free((char *) s->path);
    s->path = NULL;
    free((char *) s->hash_key);
    s->hash_key = NULL;

    return 0;
}

/* Delete a sound from the list of sounds. */
#ifdef __STDC__
void
sound_delete(SOUND *s, int remove)
#else
void
sound_delete(s, remove)
    SOUND *s;
    int remove;
#endif
{
    /* No other sounds with the same hash key.
       This is the general case. */
    if (s->next == NULL && s->prev == NULL)
    {
	if (s->hash_key)
	{
	    xhash_delete(s->hash_key);
	}
	if (s->list_prev)
	{
	    s->list_prev->list = s->list;
	}
	else
	{
	    sounds = s->list;
	}
	if (s->list)
	{
	    s->list->list_prev = s->list_prev;
	}
    }
    /* Sounds with same hash key. */
    else
    {
	/* not the first node */
	if (s->prev)
	{
	    s->prev->next = s->next;
	    if (s->next)
	    {
		s->next->prev = s->prev;
	    }
	}
	/* the first node with a next node */
	else
	{
	    if (s->list_prev)
	    {
		s->list_prev->list = s->next;
	    }
	    else
	    {
		sounds = s->next;
	    }
	    if (s->list)
	    {
		s->list->list_prev = s->next;
	    }

	    /* Copy the list links. */
	    s->next->list = s->list;
	    s->next->list_prev = s->list_prev;

	    xhash_replace(s->next->hash_key, (char *) s->next);
	}
    }

    if (remove)
    {
	struct stat st;

	if (stat(s->path, &st) == 0)
	{
	    if (unlink(s->path) < 0)
	    {
		report(REPORT_ERROR, "sound_delete: unlink %s: %s\n", s->path,
		       sys_err_str(errno));
	    }
	}
    }

    sound_free(s);
    free((char *) s);
}

/*
 * Free cached sounds and delete sound flows.
 */
void
sound_cleanup()
{
    SOUND *s1, *s1_next, *s, *s_next;
    EVENT *e;
    CONNECTION *c;
    SPOOL *sp;
    int n;

    report(REPORT_DEBUG, "cleaning up sounds\n");

    for (s1 = sounds; s1; s1 = s1_next)
    {
	s1_next = s1->list;

	for (s = s1; s; s = s_next)
	{
	    s_next = s->next;

	    /* Cached sounds */
	    if (s->type == SOUND_FILE && s->status == SOUND_READY && s->cache)
	    {
		n = 0;
		for (c = connections; c && n == 0; c = c->next)
		{
		    /* Don't clean sounds with events. */
		    for (e = c->event; e && n == 0; e = e->next)
		    {
			switch (e->type)
			{
			case EVENT_WRITE_FIND:
			case EVENT_READ_FIND_REPLY:
			case EVENT_WRITE_GET:
			case EVENT_READ_GET_REPLY:
			case EVENT_READ_SOUND:
			case EVENT_WRITE_SOUND:
			    if (e->sound == s)
			    {
				n++;
			    }
			    break;
			}
		    }
		    /* Don't clean sounds in the spool. */
		    for (sp = spool; sp; sp = sp->next)
		    {
			if (sp->sound[sp->curr_sound] == s)
			{
			    n++;
			}
		    }
		}
		if (n == 0)
		{
		    sound_unmap(s);
		}
	    }
	    /* Flows */
	    else if (s->type == SOUND_FLOW)
	    {
		/* Don't cleanup flows in the spool. */
		n = 0;
		for (sp = spool; sp; sp = sp->next)
		{
		    if (sp->sound[sp->curr_sound] == s)
		    {
			n++;
		    }
		}
		if (n != 0)
		{
		    report(REPORT_DEBUG, "sound_clean: not cleaning flow `%s'\n", s->name);
		}
		else
		{
		    sound_clean(s);
		}
	    }
#ifdef HAVE_CDROM
	    else if (s->type == SOUND_CDROM)
	    {
		n = 0;
		for (sp = spool; sp; sp = sp->next)
		{
		    if (sp->sound[sp->curr_sound] == s)
		    {
			n++;
		    }
		}
		if (n != 0)
		{
		    report(REPORT_DEBUG, "sound_clean: not cleaning cdrom `%s'\n", s->name);
		}
		else
		{
		    sound_clean(s);
		}
	    }
#endif /* HAVE_CDROM */
	}
    }

    sound_cache_size = 0;
}

#ifdef __STDC__
void
sound_clean(SOUND *s)
#else
void
sound_clean(s)
    SOUND *s;
#endif
{
    BUFFER *b;
    int n, fd;

#ifdef HAVE_HELPERS
    if (s->needs_helper)
    {
	s->samples = 0;
	s->offset = 0;
	s->size = 0;
	if (s->flow)
	{
	    buffer_dealloc(s->flow, 1);
	}
	s->flow = NULL;
	s->flowp = &s->flow;
	return;
    }
#endif /* HAVE_HELPERS */

    if (s->type == SOUND_FILE)
    {
	return;
    }
#ifdef HAVE_CDROM
    else if (s->type == SOUND_CDROM)
    {
	s->samples = 0;
	s->offset = 0;
	s->size = 0;
	if (s->flow)
	{
	    buffer_dealloc(s->flow, 1);
	}
	s->flow = NULL;
	s->flowp = &s->flow;
	return;
    }
#endif /* HAVE_CDROM */

    switch (s->storage)
    {
    case SOUND_STORAGE_NONE:
	/* nothing to free -- delete it */
	report(REPORT_DEBUG, "sound_clean: deleting flow `%s'\n", s->name);
	sound_delete(s, 0);
	break;

    case SOUND_STORAGE_MEMORY:
	/* keep it in memory? */
	report(REPORT_DEBUG, "sound_clean: keeping flow `%s' in memory\n", s->name);
	break;

    case SOUND_STORAGE_DISK:
	/* Save the flow in the disk cache. */
	if (cache_free(s->size) < 0)
	{
	    report(REPORT_ERROR, "sound_clean: can't cache_free %d bytes\n", s->size);
	    sound_delete(s, 0);	/* delete it */
	    break;
	}

	fd = cache_create(s->path, s->size);
	if (fd < 0)
	{
	    report(REPORT_ERROR, "sound_clean: can't cache_create %d bytes\n", s->size);
	    sound_delete(s, 0);
	    break;
	}

	/* Dump the flow buffers to disk. */
	report(REPORT_DEBUG, "sound_clean: storing %s\n", s->path);
	for (b = s->flow; b; b = b->next)
	{
	  retry:
	    n = write(fd, b->buf, b->nbytes);
	    if (n < 0)
	    {
		if (errno == EINTR || errno == EINTR)
		{
		    goto retry;
		}
		else
		{
		    report(REPORT_ERROR, "sound_clean: write: %s\n", sys_err_str(errno));
		    sound_delete(s, 1);
		    s = NULL;
		    break;
		}
	    }
	}
	close(fd);

	if (s)
	{
	    /* The flow is now a file. */
	    sound_unmap(s);
	    s->type = SOUND_FILE;
	}
	break;
    }
}

#ifdef __STDC__
SINDEX *
sound_open(SOUND *s, int use_helper)
#else
SINDEX *
sound_open(s, use_helper)
    SOUND *s;
    int use_helper;
#endif
{
    SINDEX *si;
    int n;

    si = (SINDEX *) malloc(sizeof(SINDEX));
    if (si == NULL)
    {
	return NULL;
    }

    si->sound = s;
    si->offset = 0;
    si->is_cached = 0;
    si->eof = 0;
    si->fd = 0;
    si->skip = 0;
    si->buffer_offset = 0;
    si->is_flow = 0;
    si->flowp = NULL;
    si->water_mark = 0;
    si->low_water_mark = 0;
    si->high_water_mark = 0;
#if defined (HAVE_CDROM) || defined (HAVE_HELPERS)
    si->pid = -1;
#endif

    switch (s->format)
    {
#ifdef HAVE_ADPCM
    case RPLAY_FORMAT_G721:
    case RPLAY_FORMAT_G723_3:
    case RPLAY_FORMAT_G723_5:
	g72x_init_state(&si->adpcm_state[0]);
	g72x_init_state(&si->adpcm_state[1]);
	si->adpcm_in_buffer = 0;
	si->adpcm_in_bits = 0;
	break;
#endif /* HAVE_ADPCM */

#ifdef HAVE_GSM
    case RPLAY_FORMAT_GSM:
	si->gsm_object = gsm_create();
	si->gsm_bit_frame_bytes = 0;
	si->gsm_fixed_buffer_size = 0;
	break;
#endif /* HAVE_GSM */

    default:
	break;
    }

#ifdef HAVE_HELPERS
    if (use_helper && s->needs_helper)
    {
	int fds[2];
	HELPER *helper;

	helper = helper_lookup(s->path);
	if (!helper)
	{
	    report(REPORT_ERROR, "%s helper not found\n", s->path);
	    done(1);
	}

	si->is_flow = 1;
	si->flowp = &s->flow;

	if (pipe(fds) < 0)
	{
	    report(REPORT_ERROR, "helper pipe: %s\n", sys_err_str(errno));
	    sound_close(si);
	    return NULL;
	}

	si->pid = fork();
	if (si->pid < 0)
	{
	    report(REPORT_ERROR, "helper fork: %s\n", sys_err_str(errno));
	    sound_close(si);
	    return NULL;
	}

	if (si->pid == 0)	/* child */
	{
	    int first;
	    char *argv[64];	/* XXX */
	    int argc = 0;
	    char buf[MAXPATHLEN], *p;
	    int input_fd = -1;

	    if (s->type == SOUND_FILE)
	    {
		input_fd = open(s->path, O_RDONLY /* | O_NDELAY */ , 0);
		if (input_fd < 0)
		{
		    report(REPORT_ERROR, "can't open %s: %s\n", s->path, sys_err_str(errno));
		    exit(1);
		}
	    }
	    else if (s->type == SOUND_FLOW)
	    {
		CONNECTION *c;
		EVENT *e;
		for (c = connections; c && input_fd == -1; c = c->next)
		{
		    for (e = c->event; e; e = e->next)
		    {
			if (e->type == EVENT_PIPE_FLOW && e->sound == s)
			{
			    input_fd = c->fd;
			    fd_block(input_fd);
			    break;
			}
		    }
		}
	    }

	    if (input_fd == -1)
	    {
		report(REPORT_ERROR, "No input for helper, type=%d.\n", s->type);
		exit(1);
	    }

	    first = 1;
	    while (p = (char *) strtok(first ? helper->program : NULL, " \t\n"))
	    {
		first = 0;
		argv[argc++] = p;
	    }
	    argv[argc] = NULL;

	    close(fds[0]);

	    if (input_fd != -1)
	    {
		if (dup2(input_fd, 0) != 0)	/* input for the helper */
		{
		    report(REPORT_ERROR, "can't dup2 stdin: %s\n", sys_err_str(errno));
		    exit(1);
		}
	    }
	    if (dup2(fds[1], 1) != 1)	/* output for rplayd */
	    {
		report(REPORT_ERROR, "can't dup2 stdout: %s\n", sys_err_str(errno));
		exit(1);
	    }

	    execv(argv[0], argv);

	    report(REPORT_ERROR, "can't execute %s: %s\n", argv[0], sys_err_str(errno));
	    exit(1);
	}

	report(REPORT_DEBUG, "forked helper process %d\n", si->pid);
	close(fds[1]);
	si->fd = fds[0];
	FD_SET(si->fd, &read_mask);
    }
#else
    if (0)
    {
	/* hack */
    }
#endif /* HAVE_HELPERS */
    else if (s->type == SOUND_FILE)
    {
	if (!s->cache)
	{
	    struct stat st;

	    si->fd = open(s->path, O_RDONLY | O_NDELAY, 0);
	    if (si->fd < 0)
	    {
		report(REPORT_DEBUG, "sound_open: open %s: %s\n", s->path,
		       sys_err_str(errno));
		sound_close(si);
		return NULL;
	    }

	    fd_nonblock(si->fd);

	    if (!s->mapped || (s->mapped && !s->size))
	    {
		if (fstat(si->fd, &st) < 0)
		{
		    report(REPORT_ERROR, "sound_open: fstat %s: %s\n",
			   s->path, sys_err_str(errno));
		    sound_close(si);
		    return NULL;
		}

		if (S_ISDIR(st.st_mode))
		{
		    report(REPORT_ERROR, "sound_open: %s is a directory\n",
			   s->path);
		    sound_close(si);
		    return NULL;
		}

		s->size = (int) st.st_size;

		/* See if the entire sound can be cached. */
		if (sound_cache_max_size
		    && !helper_lookup(s->path)
		    && (sound_cache_max_sound_size == 0 || (s->size <= sound_cache_max_sound_size))
		    && (sound_cache_size + s->size <= sound_cache_max_size))
		{
		    /* Use mmap to load the entire sound. */
#ifdef HAVE_MMAP

#ifndef MAP_FILE
#define MAP_FILE 0
#endif
		    s->cache = mmap(0, s->size, PROT_READ, MAP_SHARED | MAP_FILE, si->fd, 0);
		    if (s->cache == (caddr_t) - 1)
		    {
			report(REPORT_ERROR, "sound_open: %s size=%d mmap: %s\n",
			       s->path, s->size, sys_err_str(errno));
			sound_close(si);
			return NULL;
		    }
#else /* not HAVE_MMAP */
		    /* Use malloc+read to load the entire sound. */
		    s->cache = (char *) malloc(s->size);
		    if (s->cache == NULL)
		    {
			report(REPORT_ERROR, "sound_open: %s size=%d malloc: %s\n",
			       s->path, s->size, sys_err_str(errno));
			sound_close(si);
			return NULL;
		    }
		  again:
		    n = read(si->fd, s->cache, s->size);
		    if (n < 0 && (errno == EINTR || errno == EAGAIN))
		    {
			goto again;
		    }
		    if (n != s->size)
		    {
			report(REPORT_ERROR, "sound_open: read %s %d: %s\n",
			       s->path, s->size, sys_err_str(errno));
			free((char *) s->cache);
			s->cache = NULL;
			sound_close(si);
			return NULL;
		    }
#endif /* not HAVE_MMAP */

		    sound_cache_size += s->size;
#if 0
		    report(REPORT_DEBUG, "- cached %s %d, cache_size=%d\n",
			   s->path, s->size, sound_cache_size);
#endif

		    /* Cached sounds won't need the file descriptor. */
		    close(si->fd);
		    si->fd = 0;
		}
	    }
	}

	if (s->cache)
	{
	    si->is_cached = 1;
	}
	else
	{
	    si->is_cached = 0;
	}
    }
    else if (s->type == SOUND_FLOW)
    {
	si->is_flow = 1;
	si->flowp = &s->flow;
    }
#ifdef HAVE_CDROM
    /* Fork the reader process and treat it like a flow, reading
       audio data from the pipe. */
    else if (s->type == SOUND_CDROM)
    {
	int fds[2];

	si->is_flow = 1;
	si->flowp = &s->flow;

	if (pipe(fds) < 0)
	{
	    report(REPORT_ERROR, "cdrom pipe: %s\n", sys_err_str(errno));
	    sound_close(si);
	    return NULL;
	}

	si->pid = fork();
	if (si->pid < 0)
	{
	    report(REPORT_ERROR, "cdrom fork: %s\n", sys_err_str(errno));
	    sound_close(si);
	    return NULL;
	}

	if (si->pid == 0)	/* child */
	{
	    int i;

	    close(fds[0]);

#ifdef HAVE_SIGSET
	    sigset(SIGHUP, SIG_DFL);
	    sigset(SIGINT, SIG_DFL);
	    sigset(SIGCHLD, SIG_DFL);
#else
	    signal(SIGHUP, SIG_DFL);
	    signal(SIGINT, SIG_DFL);
	    signal(SIGCHLD, SIG_DFL);
#endif

	    for (i = 0; i < MAX_CDROMS; i++)
	    {
		if (strcmp(cdrom_table[i].name, si->sound->name) == 0)
		{
		    cdrom_reader(i, si->sound->starting_track, si->sound->ending_track, fds[1]);
		    exit(0);
		}
	    }

	    report(REPORT_DEBUG, "cdrom `%s' not found\n", si->sound->name);
	    exit(1);
	}

	report(REPORT_DEBUG, "forked cdrom process %d\n", si->pid);
	close(fds[1]);
	si->fd = fds[0];

	FD_SET(si->fd, &read_mask);
    }
#endif /* HAVE_CDROM */

    return si;
}

#ifdef __STDC__
int
sound_close(SINDEX *si)
#else
int
sound_close(si)
    SINDEX *si;
#endif
{
#ifdef HAVE_HELPERS
    if (si->sound->needs_helper)
    {
	if (si->pid != -1)
	{
	    report(REPORT_DEBUG, "killing helper process %d\n", si->pid);
	    if (kill(si->pid, SIGKILL) < 0)
	    {
		report(REPORT_DEBUG, "kill %d: %s\n", si->pid, sys_err_str(errno));
	    }
	}
	if (si->fd != 0)
	{
	    FD_CLR(si->fd, &read_mask);
	    close(si->fd);
	}
	if (si->sound->type == SOUND_FLOW)
	{
	    CONNECTION *c, *c_next;
	    EVENT *e;
	    for (c = connections; c; c = c_next)
	    {
		c_next = c->next;
		for (e = c->event; e; e = e->next)
		{
		    if (e->type == EVENT_PIPE_FLOW && e->sound == si->sound)
		    {
			connection_close(c);
			break;
		    }
		}
	    }
	}

    }
#endif /* HAVE_HELPERS */

    if (si->sound->type == SOUND_FILE)
    {
	if (!si->is_cached)
	{
	    close(si->fd);
	}
    }
#ifdef HAVE_CDROM
    else if (si->sound->type == SOUND_CDROM)
    {
	if (si->pid != -1)
	{
	    report(REPORT_DEBUG, "killing cdrom process %d\n", si->pid);
	    if (kill(si->pid, SIGKILL) < 0)
	    {
		report(REPORT_DEBUG, "kill %d: %s\n", si->pid, sys_err_str(errno));
	    }
	}
	if (si->fd != 0)
	{
	    FD_CLR(si->fd, &read_mask);
	    close(si->fd);
	}
    }
#endif /* HAVE_CDROM */

    switch (si->sound->format)
    {
#ifdef HAVE_GSM
    case RPLAY_FORMAT_GSM:
	gsm_destroy(si->gsm_object);
	break;
#endif /* HAVE_GSM */

    default:
	break;
    }

    free((char *) si);

    return 0;
}

#ifdef HAVE_ADPCM
/* This routine is based on the unpack_input routine from
   Sun's CCITT ADPCM decoder. */
static int
adpcm_unpack(si, b, code, bits)
    SINDEX *si;
    BUFFER *b;
    unsigned char *code;
    int bits;
{
    unsigned char in_byte;

    if (si->adpcm_in_bits < bits)
    {
	if (b->offset >= b->nbytes)
	{
	    *code = 0;
	    return -1;
	}
	in_byte = b->buf[b->offset++];
	si->adpcm_in_buffer |= (in_byte << si->adpcm_in_bits);
	si->adpcm_in_bits += 8;
    }

    *code = si->adpcm_in_buffer & ((1 << bits) - 1);
    si->adpcm_in_buffer >>= bits;
    si->adpcm_in_bits -= bits;

    return si->adpcm_in_bits > 0;
}

/* Uncompress the `data' buffers into 16-bit linear data. */
static void
adpcm_decode(si, data, decode_routine, decode_bits)
    SINDEX *si;
    BUFFER *data;
    int (*decode_routine) ();
    int decode_bits;
{
    BUFFER *b = buffer_create();
    short *ptr;
    unsigned char code;
    int channel = 0;

    for (; data && data->nbytes; data = data->next)
    {
	memcpy(b->buf, data->buf, data->nbytes);
	b->nbytes = data->nbytes;
	b->offset = 0;

	ptr = (short *) data->buf;
	data->nbytes = 0;

	while (adpcm_unpack(si, b, &code, decode_bits) >= 0)
	{
	    *ptr++ = (*decode_routine) (code, AUDIO_ENCODING_LINEAR, &si->adpcm_state[channel]);
	    if (si->sound->channels == 2)
	    {
		channel ^= 1;
	    }
	    data->nbytes += 2;	/* 16-bit */
	}
#if 1
	report(REPORT_DEBUG, "adpcm_decode: uncompressed from %d to %d bytes\n",
	       b->nbytes, data->nbytes);
#endif
    }

    buffer_destroy(b);
}
#endif /*  HAVE_ADPCM */

#ifdef HAVE_GSM
/* Uncompress the `data' buffers into 16-bit linear data. */
static void
gsm_uncompress(si, data)
    SINDEX *si;
    BUFFER *data;
{
    BUFFER *b = buffer_create();
    gsm_signal sample[160];
    int nbytes;

    for (; data; data = data->next)
    {
	memcpy(b->buf, data->buf, data->nbytes);
	b->nbytes = data->nbytes;
	b->offset = 0;
	data->nbytes = 0;

	do
	{
	    nbytes = MIN(b->nbytes - b->offset, sizeof(si->gsm_bit_frame) - si->gsm_bit_frame_bytes);
	    memcpy(si->gsm_bit_frame, b->buf + b->offset, nbytes);
	    b->offset += nbytes;
	    si->gsm_bit_frame_bytes += nbytes;

	    if (si->gsm_bit_frame_bytes == sizeof(si->gsm_bit_frame))	/* Only decode complete frames */
	    {
		gsm_decode(si->gsm_object, si->gsm_bit_frame, sample);
		memcpy(data->buf + data->nbytes, (char *) sample, sizeof(sample));
		data->nbytes += sizeof(sample);
		si->gsm_bit_frame_bytes = 0;
	    }

#if 0
	    printf("b->offset=%d data->nbytes=%d gsm_bit_frame_bytes=%d\n",
		   b->offset, data->nbytes, si->gsm_bit_frame_bytes);
#endif

	}
	while (b->offset < b->nbytes);

#if 1
	report(REPORT_DEBUG, "gsm_uncompress: uncompressed from %d to %d bytes\n",
	       b->nbytes, data->nbytes);
#endif
    }

    buffer_destroy(b);
}
#endif /* HAVE_GSM */

#ifdef __STDC__
int
sound_fill(SINDEX *si, BUFFER *data, int as_is)
#else
int
sound_fill(si, data, as_is)
    SINDEX *si;
    BUFFER *data;
    int as_is;
#endif
{
    static struct iovec iov[UIO_MAXIOV];
    int i, n;
    int total = 0;		/* The total number of bytes filled. */
    BUFFER *b;
    float bit_factor;
    int max_per_buffer;

    if (si->eof && !si->is_flow)
    {
	return 0;
    }

    if (si->sound->mapped)
    {
	/* ADPCM compression expands from 3/4/5 bit input to 16 bit output. */
	bit_factor = ((float) si->sound->input_precision / si->sound->output_precision);
    }
    else
    {
	bit_factor = 1.0;
    }
    max_per_buffer = BUFFER_SIZE * bit_factor;

    /* Force the buffer size to be a multiple of the GSM bit frame size. */
    if (si->sound->format == RPLAY_FORMAT_GSM)
    {
	if (si->gsm_fixed_buffer_size == 0)
	{
	    si->gsm_fixed_buffer_size = max_per_buffer;
	    while (si->gsm_fixed_buffer_size % sizeof(si->gsm_bit_frame))
	    {
		si->gsm_fixed_buffer_size--;
	    }
	}
	max_per_buffer = si->gsm_fixed_buffer_size;
    }

    /* Read from a flow.  */
    if (si->is_flow)
    {
	BUFFER *f, *f_next;

	if (!si->flowp)
	{
	    return 0;
	}

	if (!si->high_water_mark)
	{
#ifdef HAVE_CDROM
	    if (si->sound->type == SOUND_CDROM)
	    {
		SOUND *s = si->sound;
		si->low_water_mark = (s->sample_rate * s->input_sample_size) / 2;
		si->high_water_mark = s->sample_rate * s->input_sample_size;
	    }
#endif
#ifdef HAVE_HELPERS
	    if (!si->high_water_mark && si->sound->needs_helper)
	    {
		SOUND *s = si->sound;
		si->low_water_mark = (s->sample_rate * s->input_sample_size) / 4;
		si->high_water_mark = (s->sample_rate * s->input_sample_size) / 2;
	    }
#endif
	    if (!si->high_water_mark)
	    {
#if 1
		/* XXX test */
		SOUND *s = si->sound;
		si->low_water_mark = (s->sample_rate * s->input_sample_size) / 4;
		si->high_water_mark = (s->sample_rate * s->input_sample_size) / 2;
#else
		si->low_water_mark = LOW_WATER_MARK(si->sound);
		si->high_water_mark = HIGH_WATER_MARK(si->sound);
#endif
	    }
	}

	f = *si->flowp;
	for (b = data; b && f; b = b->next)
	{
	    while (f && b->nbytes < max_per_buffer)
	    {
		/* Calculate the total number of bytes available. */
		n = f->nbytes - si->buffer_offset - si->skip;
		if (n < 0)
		{
		    si->buffer_offset += si->skip;
		    si->skip -= f->nbytes;
		}
		else
		{
		    /* Calculate the number of bytes needed. */
		    i = MIN(n, max_per_buffer - b->nbytes);
		    memcpy(b->buf + b->nbytes, f->buf + si->buffer_offset + si->skip, i);
		    si->skip = 0;
		    b->nbytes += i;
		    total += i;
		    si->offset += i;
		    si->buffer_offset += i;
		}

		/* See if a new flow buffer is needed. */
		if (si->buffer_offset >= f->nbytes)
		{
		    BUFFER *f_next = f->next;

		    si->buffer_offset = 0;
		    si->flowp = &f->next;

		    /* Free the buffer. */
		    if (si->sound->storage == SOUND_STORAGE_NONE
			|| si->sound->storage == SOUND_STORAGE_NULL)
		    {
			buffer_destroy(f);
		    }

		    f = f_next;
		}
	    }
	}

	/* Update the flow pointers if the sound is not being stored. */
	if (si->sound->storage == SOUND_STORAGE_NONE
	    || si->sound->storage == SOUND_STORAGE_NULL)
	{
	    si->sound->flow = f;
	    if (!f)
	    {
		si->sound->flowp = &si->sound->flow;
	    }
	    si->flowp = &si->sound->flow;
	}

	if (si->sound->tail && si->offset >= si->sound->tail)
	{
	    si->eof = 1;
	}
    }
    /* Read from the cache. */
    else if (si->is_cached)
    {
	n = si->sound->size - si->offset;
	for (b = data; b; b = b->next)
	{
	    i = MIN(n, max_per_buffer);
	    if (i > 0)
	    {
#ifdef DEBUG
		report(REPORT_DEBUG, "- cache-read %s %d\n", si->sound->name, i);
#endif
		memcpy(b->buf, si->sound->cache + si->offset, i);
		b->nbytes = i;
		si->offset += i;
		n -= i;
		total += i;
	    }
	    else
	    {
		b->nbytes = 0;
		n -= i;
	    }
	}
	if (si->offset >= si->sound->size)
	{
	    si->eof = 1;
	}
    }
    /* Read from disk. */
    else
    {
	for (b = data, i = 0; b && i < UIO_MAXIOV; b = b->next, i++)
	{
	    iov[i].iov_base = b->buf;
	    iov[i].iov_len = max_per_buffer;
	}

	n = readv(si->fd, iov, i);
	if (n < 0)
	{
	    report(REPORT_ERROR, "sound_fill: readv: %s, fd=%d, %s\n",
		   si->sound->name, si->fd, sys_err_str(errno));
	    return -1;
	}
	else if (n == 0)
	{
	    si->eof = 1;
	    return 0;
	}
	else
	{
	    si->offset += n;
	    for (b = data, i = n; b; b = b->next, i -= max_per_buffer)
	    {
		if (i < 0)
		{
		    i = 0;
		}
		b->nbytes = MIN(i, max_per_buffer);
	    }
	    total = n;
	}
    }

    /* uncompress/decode */
    if (total && si->sound->mapped && !as_is)
    {
	switch (si->sound->format)
	{
#ifdef HAVE_ADPCM
	case RPLAY_FORMAT_G721:
	    adpcm_decode(si, data, g721_decoder, 4);
	    break;

	case RPLAY_FORMAT_G723_3:
	    adpcm_decode(si, data, g723_24_decoder, 3);
	    break;

	case RPLAY_FORMAT_G723_5:
	    adpcm_decode(si, data, g723_40_decoder, 5);
	    break;
#endif /* HAVE_ADPCM */

#ifdef HAVE_GSM
	case RPLAY_FORMAT_GSM:
	    gsm_uncompress(si, data);
	    break;
#endif /* HAVE_GSM */
	}
    }

    return total;
}

#ifdef __STDC__
int
sound_seek(SINDEX *si, int offset, int whence)
#else
int
sound_seek(si, offset, whence)
    SINDEX *si;
    int offset;
    int whence;
#endif
{
    if (si->is_flow)
    {
	si->skip += offset;	/* XXX: whence is ignored */
	return 0;
    }
    else if (si->is_cached)
    {
	si->offset += offset;	/* XXX: whence is ignored */
	return 0;
    }
#ifdef HAVE_CDROM
    else if (si->sound->type == SOUND_CDROM)
    {
	return 0;
    }
#endif /* HAVE_CDROM */
#ifdef HAVE_HELPERS
    else if (si->sound->needs_helper)
    {
	return 0;
    }
#endif /* HAVE_HELPERS */
    else
    {
	return lseek(si->fd, offset, whence);
    }
}

#if defined(HAVE_CDROM) || defined (HAVE_HELPERS)
/* Read data from the pipe and return the data in a buffer list. */
#ifdef __STDC__
BUFFER *
sound_pipe_read(SINDEX *si)
#else
BUFFER *
sound_pipe_read(si)
    SINDEX *si;
#endif
{
    static struct iovec iov[UIO_MAXIOV];
    int i, n, nbytes;
    BUFFER *b;
    BUFFER *data;

    /* Pause */
    if (si->water_mark - si->offset > si->high_water_mark)
    {
	FD_CLR(si->fd, &read_mask);
	return NULL;
    }

    /* Determine the number of bytes available in the pipe. */
#if defined (sun) && defined (SVR4)	/* Solaris 2.x */
    {
	struct stat st;
	if (fstat(si->fd, &st) < 0)
	{
	    report(REPORT_DEBUG, "sound_pipe_read: fstat: %s\n",
		   sys_err_str(errno));
	    return NULL;
	}
	nbytes = st.st_size;
    }
#else /* not Solaris */
#if 0				/* disable FIONREAD for now */
#ifdef FIONREAD
    if (ioctl(si->fd, FIONREAD, &nbytes) < 0)
    {
	report(REPORT_DEBUG, "sound_pipe_read: ioctl FIONREAD: %s\n",
	       sys_err_str(errno));
	return NULL;
    }
#endif /* not FIONREAD */
#endif
    /* guess */
    nbytes = (si->sound->sample_rate * si->sound->input_sample_size) / 2;
#endif /* not Solaris */

    data = buffer_alloc(nbytes, BUFFER_FREE);

    for (b = data, i = 0; b && i < UIO_MAXIOV; b = b->next, i++)
    {
	iov[i].iov_base = b->buf;
	iov[i].iov_len = BUFFER_SIZE;
    }

    n = readv(si->fd, iov, i);
    if (n <= 0)
    {
	buffer_dealloc(data, 1);
	spool_remove(si->sound);	/* XXX */
	return NULL;
    }

    for (b = data, i = n; b; b = b->next, i -= BUFFER_SIZE)
    {
	if (i < 0)
	{
	    i = 0;
	}
	b->nbytes = MIN(i, BUFFER_SIZE);
    }

    return data;
}
#endif

#ifdef __STDC__
static int
bytes_in_sound(SOUND *s)
#else
static int
bytes_in_sound(s)
    SOUND *s;
#endif
{
    int bytes = 0;

    if (s->chunk_size)
    {
	bytes = s->chunk_size;
    }
    else if (s->tail)
    {
	bytes = s->tail - s->offset;
    }
    else if (s->size)
    {
	bytes = s->size - s->offset;
    }

    return bytes;
}

#ifdef __STDC__
int
number_of_samples(SOUND *s, int bytes)
#else
int
number_of_samples(s, bytes)
    SOUND *s;
    int bytes;
#endif
{
    if (!bytes)
    {
	bytes = bytes_in_sound(s);
    }
    return (bytes << 3) / (s->input_precision * s->channels);
}
