/*
 * Copyright (c) 1993 by Raphael Quinet (quinet@montefiore.ulg.ac.be)
 * Bits of code from "rplay.c" :  Copyright (c) 1992-93 by Mark Boyns
 *                                                   (boyns@sdsu.edu)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 */

/*
 * Thanks to Mark Boyns and R.K.Lloyd@csc.liv.ac.uk for their comments and
 * bug fixes.
 *
 * If you make any useful changes/improvements/bug fixes to this program,
 * please send them to me (quinet@montefiore.ulg.ac.be) and they will be
 * included in the next version of this program.
 */

static char *jukebox_version = "JUKEBOX 1.3";

#include <rplay.h>
#include <conf.h>
#include <stdio.h>
#include <strings.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>

extern char *optarg; 
extern int   optind;

/* Structure used in the "songs" table. */
typedef struct
  {
    int   idx;
    char *name;
    int   length;
  } song;

/* Global variables. */
song          *songs;
int            nsongs = 0;
char          *progname;
int            stop_song = 0;
unsigned long  total_time = 0;
int            opt_quiet;


/*****************************************************************************
 * usage()                                                                   *
 *---------------------------------------------------------------------------*
 * Displays a help message.                                                  *
 *****************************************************************************/
usage()
{
  printf("\n%s - Copyright (c) 1993 by Raphael Quinet (quinet@montefiore.ulg.ac.be)\n\n", jukebox_version);
  printf("usage: %s [options] [directory ...]\n", progname);
  printf("\t-a\tplay songs in alphabetical order (file names)\n");
  printf("\t-f name\tload the list of songs from a file\n");
  printf("\t-h host\tuse this instead of the default host (%s)\n",
	 rplay_default_host());
  printf("\t-l\tloop mode - no exit\n");
  printf("\t-q\tquiet - only error messages\n");
  printf("\t-r\tplay songs in a random order\n");
  printf("\t-s n\tsleep n milliseconds between songs (n may be negative)\n");
  printf("\t-v n\tvolume n (%d <= n <= %d), default = %d\n",
	 RPLAY_MIN_VOLUME, RPLAY_MAX_VOLUME, RPLAY_DEFAULT_VOLUME);
  printf("No options will play all songs (*.au) from the current\n");
  printf("directory at the default volume\n\n");
  exit(1);
}


/*****************************************************************************
 * set_timer()                                                               *
 *---------------------------------------------------------------------------*
 * Sets the real-time timer to "msec" milliseconds.  A SIGALRM signal is     *
 * delivered when this timer expires.                                        *
 *****************************************************************************/
#ifdef __STDC__
set_timer(int msec)
#else
set_timer(msec)
     int  msec;
#endif
{
  struct itimerval it;

  it.it_value.tv_sec = msec / 1000;
  it.it_value.tv_usec = (msec % 1000) * 1000;
  it.it_interval.tv_sec = 0;
  it.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &it, (struct itimerval *)0);
}


/*****************************************************************************
 * catch_sigtstp()                                                           *
 *---------------------------------------------------------------------------*
 * SIGTSTP handler : If the user wants to stop the jukebox (Ctrl-Z).         *
 *****************************************************************************/
catch_sigtstp()
{
  signal(SIGTSTP, catch_sigtstp);
  stop_song = 2; /* Stop current song and call the default SIGTSTP handler. */
}


/*****************************************************************************
 * catch_sigint()                                                            *
 *---------------------------------------------------------------------------*
 * SIGINT handler : If the user wants another song (Ctrl-C).                 *
 *****************************************************************************/
catch_sigint()
{
  signal(SIGINT, catch_sigint);
  stop_song = 1; /* Stop current song and play the next one. */
}


/*****************************************************************************
 * catch_sigalrm()                                                           *
 *---------------------------------------------------------------------------*
 * SIGALRM handler : It's time to play the next song...                      *
 *****************************************************************************/
catch_sigalrm()
{
  signal(SIGALRM, catch_sigalrm);
  stop_song = 0; /* Play the next song. */
}


/*****************************************************************************
 * readsongs(char *dirname)                                                  *
 *---------------------------------------------------------------------------*
 * Looks for "*.au" files in the "dirname" directory and stores the names    *
 * and lengths of the files in the "songs" table.                            *
 *****************************************************************************/
#ifdef __STDC__
readsongs(char *dirname)
#else
readsongs(dirname)
     char *dirname;
#endif
{
  DIR           *dirp;
  struct dirent *dp;
  struct stat    statbuf;
  char           filename[MAXPATHLEN];

  strcpy(filename, dirname);
  strcat(filename, "/");

  dirp = opendir(dirname);
  if (dirp == NULL)
    {
      fprintf(stderr, "%s: can't open directory %s\n", progname, dirname);
      exit(1);
    }

  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
    if ((strlen(dp->d_name) > 3) &&
	!strcmp(dp->d_name + strlen(dp->d_name) - 3, ".au"))
      {
	/* "stat()" needs the full path to the files, so it is built in
	 * the "filename" buffer.  As "rplayd" doesn't need the paths, only
	 * the file name is stored in the "songs" table.
         */
	strcat(filename, dp->d_name);
	if (stat(filename, &statbuf) == -1)
	  {
	    if (!opt_quiet)
	      fprintf(stderr, "%s warning: can't stat %s\n",
		      progname, filename);
	    continue;
	  }
	filename[strlen(filename) - dp->d_namlen] = '\0';

	if (nsongs == 0)
	  songs = (song *)malloc(sizeof(song) * ++nsongs);
	else
	  songs = (song *)realloc(songs, sizeof(song) * ++nsongs);
	if (songs == NULL)
	  {
	    fprintf(stderr, "%s: out of memory\n", progname);
	    exit(1);
	  }

	songs[nsongs - 1].name = (char *)malloc(dp->d_namlen + 1);
	if (songs[nsongs - 1].name == NULL)
	  {
	    fprintf(stderr, "%s: out of memory\n", progname);
	    exit(1);
	  }

	strcpy(songs[nsongs - 1].name, dp->d_name);
	songs[nsongs - 1].length = statbuf.st_size / 8 - 10;
	total_time += songs[nsongs - 1].length;
      }
  closedir (dirp);   
}


/*****************************************************************************
 * readlist(char *listname)                                                  *
 *---------------------------------------------------------------------------*
 * Reads a list of songs from the file "listname".                           *
 *****************************************************************************/
#ifdef __STDC__
readlist(char *listname)
#else
readlist(listname)
     char *listname;
#endif
{
  FILE          *fp;
  struct stat    statbuf;
  char           buf[MAXPATHLEN], filename[MAXPATHLEN], *s;

  fp = fopen(listname, "r");
  if (fp == NULL)
    {
      fprintf(stderr, "%s: cannot open %s\n", progname, listname);
      exit(1);
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

      sscanf(buf, "%[^\n]", filename);
      if (stat(filename, &statbuf) == -1)
	{
	  if (!opt_quiet)
	    fprintf(stderr, "%s warning: can't stat %s\n",
		    progname, filename);
	  continue;
	}

      if (nsongs == 0)
	songs = (song *)malloc(sizeof(song) * ++nsongs);
      else
	songs = (song *)realloc(songs, sizeof(song) * ++nsongs);
      if (songs == NULL)
	{
	  fprintf(stderr, "%s: out of memory\n", progname);
	  exit(1);
	}

      if (s = (char *)strrchr(filename, '/'))
	s++;
      else
	s = filename;

      songs[nsongs - 1].name = (char *)malloc(strlen(s) + 1);
      if (songs[nsongs - 1].name == NULL)
	{
	  fprintf(stderr, "%s: out of memory\n", progname);
	  exit(1);
	}

      strcpy(songs[nsongs - 1].name, s);
      songs[nsongs - 1].length = statbuf.st_size / 8 - 10;
      total_time += songs[nsongs - 1].length;
    }
  fclose(fp);
}


/*****************************************************************************
 * idxcompare(song *i, song *j)                                              *
 *---------------------------------------------------------------------------*
 * Compares two song's indexes (used by "qsort()" - random order).           *
 *****************************************************************************/
#ifdef __STDC__
idxcompare(song *i, song *j)
#else
idxcompare(i,j)
     song *i, *j;
#endif
{
  return (i->idx - j->idx);
}


/*****************************************************************************
 * alphacompare(song *i, song *j)                                            *
 *---------------------------------------------------------------------------*
 * Compares two song's names (used by "qsort()" - alphabetical order).       *
 *****************************************************************************/
#ifdef __STDC__
alphacompare(song *i, song *j)
#else
alphacompare(i,j)
     song *i, *j;
#endif
{
  return strcmp(i->name, j->name);
}


/*****************************************************************************
 * main(int argc, char **argv)                                               *
 *---------------------------------------------------------------------------*
 * Come on, don't tell me that you don't know why I wrote a function named   *
 * "main()" !  My compiler told me it was really useful...                   *
 *****************************************************************************/
#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc, argv)
     int    argc;
     char **argv;
#endif
{
  int     rplay_fd, c, i;
  int     opt_volume, opt_sleep, opt_loop, opt_random, opt_sort;
  char   *host;
  RPLAY  *rpp, *rps;

  if (progname = (char *)strrchr(*argv, '/'))
    progname++;
  else
    progname = *argv;

  host = rplay_default_host();

  opt_loop = 0;
  opt_quiet = 0;
  opt_random = 0;
  opt_sort = 0;
  opt_sleep = 0;
  opt_volume = RPLAY_DEFAULT_VOLUME;
  
  while ((c = getopt(argc, argv, "af:h:lqrs:v:")) != -1)
    {
      switch(c)
	{
	case 'a':
	  if (opt_random && !opt_quiet)
	    fprintf(stderr, "%s warning: option -a overrides previous option -r\n", progname);
	  opt_sort = 1;
	  break;

	case 'f':
	  readlist(optarg);
	  break;

	case 'h':
	  host = optarg;
	  if (host[0] == '-')
	    usage();
	  break;
	  
	case 'l':
	  opt_loop = 1;
	  break;
	  
	case 'q':
	  opt_quiet = 1;
	  break;
	  
	case 'r':
	  if (opt_sort && !opt_quiet)
	    fprintf(stderr, "%s warning: option -r overrides previous option -a\n", progname);
	  opt_random = 1;
	  srandom(getpid());
	  break;
	  
	case 's':
	  opt_sleep = atoi(optarg);
	  break;
	  
	case 'v':
	  opt_volume = atoi(optarg);

	default:
	  usage();
	  exit(1);
	} 
    }

  if (argc == optind)
    readsongs(".");
  else
    while (optind < argc)
      readsongs(argv[optind++]);

  if (nsongs == 0)
    {
      fprintf(stderr, "%s: no songs (*.au) to play\n", progname);
      exit(1);
    }

  rplay_fd = rplay_open(host);
  if (rplay_fd < 0)
    {
      rplay_perror(host);
      exit(1);
    }

  rpp = rplay_create(RPLAY_PLAY);
  if (rpp == NULL)
    {
      rplay_perror("rplay_create");
      exit(1);
    }

  c = rplay_set(rpp, RPLAY_APPEND,
		RPLAY_SOUND, "",
		RPLAY_VOLUME, opt_volume,
		NULL);
  if (c < 0)
    {
      rplay_perror("rplay_set");
      exit(1);
    }

  rps = rplay_create(RPLAY_STOP);
  if (rps == NULL)
    {
      rplay_perror("rplay_create");
      exit(1);
    }

  c = rplay_set(rps, RPLAY_APPEND,
		RPLAY_SOUND, "",
		NULL);
  if (c < 0)
    {
      rplay_perror("rplay_set");
      exit(1);
    }

  /* Use Ctrl-Z (SIGTSTP) if you want to stop the program then send a SIGTERM
   * or SIGHUP if you want to kill it.  Ctrl-C (SIGINT) will only stop the
   * current song and play the next one.
   */
  signal(SIGINT, catch_sigint);
  signal(SIGTSTP, catch_sigtstp);
  signal(SIGALRM, catch_sigalrm);

  if (!opt_quiet)
    {
      printf("\n%s - Welcome !\n\n", jukebox_version);
      printf("I will play %d songs just for you.\n", nsongs);
      if (opt_sleep)
	  printf("(Estimated total time : %lu%+ld seconds).\n\n",
		 total_time / 1000L, ((long)opt_sleep * (long)nsongs) / 1000L);
	else
	  printf("(Estimated total time : %lu seconds).\n\n",
		 total_time / 1000L);
    }

  if (opt_sort)
    qsort((char *)songs, nsongs, sizeof(song), alphacompare);

  do
    {

      if (opt_random)
	{
	  if (!opt_quiet && opt_loop)
	    printf("Building random list ...\n\n");
	  for (i = 0; i < nsongs; i++)
	    songs[i].idx = (int)random();
	  qsort((char *)songs, nsongs, sizeof(song), idxcompare);
	}

      for (i = 0; i < nsongs; i++)
	{
	  if (!opt_quiet)
	    {
	      if (opt_sleep)
		printf("Playing %s ...  (%d%+d seconds)\n", songs[i].name,
		       songs[i].length / 1000, opt_sleep / 1000);
	      else
		printf("Playing %s ...  (%d seconds)\n", songs[i].name,
		       songs[i].length / 1000);
	    }
	  c = rplay_set(rpp, RPLAY_CHANGE, 0,
			RPLAY_SOUND, songs[i].name,
			NULL);
	  if (c < 0)
	    {
	      rplay_perror("rplay_set");
	      exit(1);
	    }

	  if (rplay(rplay_fd, rpp) < 0)
	    {
	      rplay_perror(host);
	      exit(1);
	    }

	  if (songs[i].length + opt_sleep > 100)
	    set_timer(songs[i].length + opt_sleep);
	  else
	    set_timer(100);

	  /* Wait until the next signal.  This may be a SIGALRM delivered
	   * by the timer (just play the next song) or another signal from
	   * the user (stop the current song).
	   */
	  pause();

	  if (stop_song != 0) /* Stop the current song. */
	    {
	      if (!opt_quiet)
		printf(" Ouch !\n");
	      c = rplay_set(rps, RPLAY_CHANGE, 0,
			    RPLAY_SOUND, songs[i].name,
			    NULL);
	      if (c < 0)
		{
		  rplay_perror("rplay_set");
		  exit(1);
		}

	      if (rplay(rplay_fd, rps) < 0)
		{
		  rplay_perror(host);
		  exit(1);
		}

	      if (stop_song == 2) /* And stop the jukebox too. */
		{
		  signal(SIGTSTP, SIG_DFL);
		  kill(getpid(), SIGTSTP); /* Stop now ! */
		  if (!opt_quiet)
		    printf("\nI still have %d songs to play.\n",
			   nsongs - i - 1);
		  signal(SIGTSTP, catch_sigtstp);
		}
	    }
	}

    } while (opt_loop > 0);

  if (!opt_quiet)
    printf("\nPlease insert coins to play another song.\n");

  rplay_close(rplay_fd);
  exit(0);
}

