/* 
 * Copyright (C) 1993 Andrew Scherpbier <Andrew@sdsu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <rplay.h>
#include <pwd.h>

#define	PUBLIC
#define	PRIVATE		static

#undef TRUE
#define	TRUE		(1)
#undef FALSE
#define	FALSE		(0)

#define	OK		(0)
#define	NOTOK		(-1)

/* Private routines
 * ================
 */
PRIVATE void dump_state();
PRIVATE void parse_args(int ac, char **av, int size);
PRIVATE compare(char *buffer, char *from, long size);
PRIVATE play_sound(char *sound, long size);

/* Private variables
 * =================
 */
PRIVATE char	hostname[200] = "localhost";
PRIVATE int	volume = 127;
PRIVATE int	use_random = FALSE;
PRIVATE int	use_size_volume = FALSE;
PRIVATE int	use_size_sound = FALSE;
PRIVATE int	min_size = 0;
PRIVATE int	max_size = 10000;
PRIVATE int	min_volume = 10;
PRIVATE int	max_volume = 200;
PRIVATE char	*sound = "youvegotmail";
PRIVATE char	*subject_re = NULL;
PRIVATE char	subject[200];
PRIVATE int	debug = FALSE;


/* Public routines
 * ===============
 */

/* Public variables
 * ================
 */

/**************************************************************************
 * PUBLIC main(int ac, char **av)
 * PURPOSE:
 *   None
 */
PUBLIC main(int ac, char **av)
{
	char		buffer[1000];
	char		path[200];
	char		address[1000];
	char		*p;
	FILE		*fl;
	int		size = 0;
	struct passwd	*pw;

	/*
	 *   Read  the  first  line of the message. This is what we will use
	 *   to find out what sound to play.
	 */
	gets(buffer);
	size = strlen(buffer) + 1;
	if (!strtok(buffer, " "))
		exit(1);
	p = strtok(NULL, " \t\n");
	strcpy(address, p);
	subject[0] = '\0';

	/*
	 *   We  now  need  to read the rest of the message to determine its
	 *   size.
	 */
	while (gets(buffer))
	{
		if (strncmp(buffer, "Subject: ", 9) == 0)
		{
			strcpy(subject, buffer + 9);
		}
		size += strlen(buffer) + 1;
	}

	/*
	 *   Deal with the command line arguments to be used as defaults.
	 */
	parse_args(ac, av, size);

	if (debug)
		dump_state();

	/*
	 *   Find the configuration file in the home directory
	 */
	pw = getpwuid(getuid());
	sprintf(path, "%s/.mailsounds", pw->pw_dir);
	fl = fopen(path, "r");
	if (fl == NULL)
		exit(0);

	/*
	 *   Search  through  this  file to find a line which will match the
	 *   address
	 */
	while (fgets(buffer, 1000, fl) != NULL)
	{
		compare(buffer, address, size);
	}

	play_sound(av[1], 0);
	fclose(fl);
	exit(0);
}


/**************************************************************************
 * PRIVATE compare(char *buffer, char *from, long size)
 * PURPOSE:
 *   None
 * PARAMETERS:
 *   None
 */
PRIVATE compare(char *buffer, char *from, long size)
{
	char	*pattern, *sound_args;
	char	*p;

	pattern = strtok(buffer, " \t");
	sound_args = strtok(NULL, "\t\n");
	if ((char *)re_comp(pattern) != NULL)
		return;
	if (re_exec(from))
	{
		if (debug)
			printf("Normal sound is played\n");
		if (play_sound(sound_args, size) == OK)
			exit(0);
	}
}


/**************************************************************************
 * PRIVATE play_sound(char *sound_args, long size)
 * PURPOSE:
 *   None
 * PARAMETERS:
 *   None
 */
PRIVATE play_sound(char *sound_args, long size)
{
	int	ac;
	char	*av[50];
	char	*token;

	/*
	 *   We need to build an argument list from the sound_args string.
	 */
	ac = 1;
	av[0] = "xxxx";
	token = strtok(sound_args, " \t");
	while (token && ac < 50)
	{
		av[ac++] = token;
		token = strtok(NULL, " \t");
	}

	subject_re = NULL;

	parse_args(ac, av, size);

	if (subject_re)
	{
		if ((char *)re_comp(subject_re) != NULL)
			return NOTOK;
		if (!re_exec(subject))
			return NOTOK;
	}

	if (debug)
		dump_state();

	/*
	 *   Play the sound on each of the hosts listed in the hostname.
	 */
	token = strtok(hostname, ":");
	while (token)
	{
		rplay_host_volume(token, sound, volume);
		token = strtok(NULL, ":");
	}
	return OK;
}


/**************************************************************************
 * PRIVATE void parse_args(int ac, char **av, int size)
 * PURPOSE:
 *   Take  arguments  and  extract data from them. The arguments are
 *   assumed  to  be  in  the  same  format as what is passed to the
 *   main() program.
 */
PRIVATE void parse_args(int ac, char **av, int size)
{
	int		c;
	int		i, count;
	extern char	*optarg;
	extern int	optind;
	char		*sounds[50];

	optind = 1;
	subject_re = NULL;
	while ((c = getopt(ac, av, "s:h:rz:Z:v:d")) != NOTOK)
	{
		switch (c)
		{
			case 'h':
				strcpy(hostname, optarg);
				break;
			case 'r':
				use_random = TRUE;
				break;
			case 's':
				subject_re = strdup(optarg);
				break;
			case 'z':
				sscanf(optarg, "%d:%d,%d:%d", &min_size, &min_volume, &max_size, &max_volume);
				use_size_volume = TRUE;
				break;
			case 'Z':
				sscanf(optarg, "%d,%d", &min_size, &max_size);
				use_size_sound = TRUE;
				break;
			case 'v':
				volume = atoi(optarg);
				break;
			case 'd':
				debug = TRUE;
				break;
		}
	}

	/*
	 *   The rest of the arguments are assumed to be sounds.
	 */
	count = 0;
	for (i = optind; i < ac && count < 50; i++)
	{
		sounds[count++] = av[i];
	}

	/*
	 * By default we will pick the first sound in the list
	 */
	sound = sounds[0];

	/*
	 *   Now pick the sound and the volume.
	 */
	if (use_random)
	{
		srandom(time(NULL));
		sound = sounds[random() % count];
	}
	else if (use_size_sound)
	{
		if (size < min_size)
			sound = sounds[0];
		if (size > max_size)
			sound = sounds[count - 1];
		else
		{
			sound = sounds[(size - min_size) * count / (max_size - min_size)];
		}
	}

	if (use_size_volume)
	{
		if (size < min_size)
			volume = min_volume;
		else if (size > max_size)
			volume = max_volume;
		else
			volume = (size - min_size) * (max_volume - min_volume) / (max_size - min_size) + min_volume;
	}
}



/**************************************************************************
 * PRIVATE void dump_state()
 * PURPOSE:
 *   Produce  debug  output  of  all the global variables which make
 *   up the state of the program
 */
PRIVATE void dump_state()
{
	printf("host: '%s', volume: %d, random: %d, size_volume: %d, size_sound: %d\n",
		hostname,
		volume,
		use_random,
		use_size_volume,
		use_size_sound);
	printf("range data: v: (%d, %d), s: (%d, %d)\n",
		min_volume, max_volume,
		min_size, max_size);
	printf("sound: '%s'\n", sound);
	if (subject_re)
		printf("Subject regular expression: '%s'\n", subject_re);
	printf("Subject: '%s'\n", subject);
}



