/* $Id: cache.c,v 1.4 1999/03/10 07:58:02 boyns Exp $ */

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
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/errno.h>
#ifdef ultrix
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <unistd.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <dirent.h>
#include "rplayd.h"
#include "sound.h"
#include "spool.h"
#include "cache.h"

static DIR *cache_dir;
static char cache_path[MAXPATHLEN];
static char cache_directory[MAXPATHLEN];

int cache_max_size = RPLAY_CACHE_SIZE;
int cache_remove = 0;

/*
 * initialize the cache
 */
#ifdef __STDC__
void
cache_init(char *dir_name)
#else
void
cache_init(dir_name)
    char *dir_name;
#endif
{
    struct stat st;

    strcpy(cache_directory, dir_name);
    if (stat(cache_directory, &st) < 0)
    {
	report(REPORT_DEBUG, "creating cache directory `%s'\n", cache_directory);
	if (mkdir(cache_directory, 0777) < 0)
	{
	    report(REPORT_ERROR, "cache_init: cannot create cache directory '%s'\n", cache_directory);
	}
    }
    else if (!S_ISDIR(st.st_mode))
    {
	report(REPORT_ERROR, "cache_init: %s not a directory\n", cache_directory);
	done(1);
    }
}

/*
 * return the name of the first cache entry
 */
char *
cache_first()
{
    if (cache_dir)
    {
	rewinddir(cache_dir);
    }
    else
    {
	cache_dir = opendir(cache_directory);
	if (cache_dir == NULL)
	{
	    report(REPORT_ERROR, "cache_first: opendir %s: %s\n", cache_directory, sys_err_str(errno));
	    return NULL;
	}
    }

    return cache_next();
}

/*
 * return the name of the next cache entry
 */
char *
cache_next()
{
    struct dirent *dp;

    do
    {
	dp = readdir(cache_dir);
	if (dp == NULL)
	{
	    closedir(cache_dir);
	    cache_dir = NULL;
	    return "";
	}
    }
    while (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0);

    SNPRINTF(SIZE(cache_path, sizeof(cache_path)), "%s/%s", cache_directory, dp->d_name);

    return cache_path;
}

/*
 * calculate the size of the cache
 */
int
cache_size()
{
    struct stat st;
    int size = 0;
    char *file;

    for (file = cache_first(); file && *file; file = cache_next())
    {
	if (stat(file, &st) < 0)
	{
	    report(REPORT_ERROR, "cache_size: stat %s: %s\n", file, sys_err_str(errno));
	    return -1;
	}
	size += (int) st.st_size;
    }

    return (int) size;
}

/*
 * load files that are already in the cache
 */
void
cache_read()
{
    char *file;

    for (file = cache_first(); file && *file; file = cache_next())
    {
	sound_insert(file, SOUND_READY, SOUND_FILE);
    }
}

/*
 * prepend the cache path to the sound
 *
 * this returns static data!
 */
#ifdef __STDC__
char *
cache_name(char *sound)
#else
char *
cache_name(sound)
    char *sound;
#endif
{
    static char _cache_path[MAXPATHLEN];

    SNPRINTF(SIZE(_cache_path, sizeof(_cache_path)), "%s/%s", cache_directory, sound);

    return _cache_path;
}

/*
 * free enough space in the cache for size bytes
 */
#ifdef __STDC__
int
cache_free(int size)
#else
int
cache_free(size)
    int size;
#endif
{
    int curr_size, limit;
    char *file;
    SOUND *s;
    struct stat st;
    int n;

    if (cache_max_size == 0)
    {
	return 0;
    }

    if (size > cache_max_size)
    {
	report(REPORT_DEBUG, "%d bytes cannot fit in the cache\n", size);
	return -1;
    }

    curr_size = cache_size();

    report(REPORT_DEBUG, "sound_count = %d, current cache size = %d bytes\n", sound_count, curr_size);

    if (size + curr_size <= cache_max_size)
    {
	return 0;
    }

    /*
     * set the initial limit
     */
    limit = sound_count / 2;

    for (;;)
    {
	report(REPORT_DEBUG, "removing cache entries (limit = %d)\n", limit);

	for (file = cache_first(); file && *file; file = cache_next())
	{
	    s = sound_lookup(file, SOUND_DONT_COUNT, NULL);
	    if (s == NULL || s->status != SOUND_READY)
	    {
		continue;
	    }
	    if (s->count < limit)
	    {
		if (stat(file, &st) < 0)
		{
		    report(REPORT_ERROR, "cache_free: stat %s: %s\n", file, sys_err_str(errno));
		    continue;
		}
		curr_size -= (int) st.st_size;
		spool_remove(s);
		sound_delete(s, 1);
		report(REPORT_DEBUG, "removed %s size=%d count=%d\n", file, st.st_size, s->count);
	    }
	}

	if (size + curr_size <= cache_max_size)
	{
	    break;
	}
	else if (limit == sound_count)
	{
	    report(REPORT_ERROR, "cache_free: cannot make room for %d bytes in the cache\n", size);
	    return -1;
	}
	else
	{
	    n = limit / 4;
	    limit += n ? n : 1;
	    limit = MIN(limit, sound_count);
	}
    }

    return 0;
}

/*
 * create room in the cache for the file of size bytes
 */
#ifdef __STDC__
int
cache_create(char *name, int size)
#else
int
cache_create(name, size)
    char *name;
    int size;
#endif
{
    int fd;

    fd = open(name, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
    {
	report(REPORT_ERROR, "cache_create: open: %s\n", sys_err_str(errno));
	return -1;
    }

    if (lseek(fd, size - 1, SEEK_SET) < 0)
    {
	report(REPORT_ERROR, "cache_create: lseek: %d %s\n", sys_err_str(errno));
	return -1;
    }

  restart:
    if (write(fd, "", 1) != 1)
    {
	if (errno == EINTR || errno == EAGAIN)
	{
	    goto restart;
	}
	report(REPORT_ERROR, "cache_create: write: %s\n", sys_err_str(errno));
	return -1;
    }

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
	report(REPORT_ERROR, "cache_create: lseek: 0 %s\n", sys_err_str(errno));
	unlink(name);
	return -1;
    }

    return fd;
}

/* Optionally remove the cache directory and all its contents. */
void
cache_cleanup()
{
    int size;
    char *file;

    if (*cache_directory == '\0')	/* cache_init wasn't called */
    {
	return;
    }

    size = cache_size();
    if (size == 0 || cache_remove)
    {
	report(REPORT_DEBUG, "cleaning `%s'\n", cache_directory);
	for (file = cache_first(); file && *file; file = cache_next())
	{
	    if (unlink(file) < 0)
	    {
		report(REPORT_ERROR, "cache_cleanup: unlink %s: %s\n",
		       file, sys_err_str(errno));
	    }
	}

	if (rmdir(cache_directory) < 0)
	{
	    report(REPORT_ERROR, "cache_cleanup: rmdir %s: %s\n",
		   cache_directory, sys_err_str(errno));
	}
    }
}
