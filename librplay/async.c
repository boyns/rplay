/* async.c - Asynchronous I/O buffer system  */

/* 
 * Copyright (C) 1995 Andrew Scherpbier <andrew@sdsu.edu>
 *
 * This file is part of rplay.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "version.h"
#include "rplay.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#ifdef __STDC__
# include <stdarg.h>
#else
# include <varargs.h>
#endif

typedef void (*cb)();

typedef struct obuf_s
{
	struct obuf_s	*next;
	char			*data;
	char			*current;
	int				length;
	cb				callback;
} obuf;

typedef struct ibuf_s
{
	struct ibuf_s	*next;
	char			*data;
	char			*current;
	int				length;
} ibuf;

typedef struct
{
	obuf	*olist;
	obuf	*otail;
	ibuf	*ilist;
	ibuf	*itail;
	int		writing;
	int		notify_mask;
	cb		rcallback;
	int		rraw;
	cb		wcallback;
	int		wraw;
} grp;


static grp	group[FD_SETSIZE];
static int	looping;
static int	main_loop_return_value;


#ifdef __STDC__
static void write_proc(int rptp_fd);
static void do_register(int rptp_fd);
static void do_unregister(int rptp_fd);
static void notify_line(int rptp_fd, char *line);
static void read_proc(int rptp_fd);
static void process_input(int rptp_fd);
#else
static void write_proc(/* int rptp_fd */);
static void do_register(/* int rptp_fd */);
static void do_unregister(/* int rptp_fd */);
static void notify_line(/* int rptp_fd, char *line */);
static void read_proc(/* int rptp_fd */);
static void process_input(/* int rptp_fd */);
#endif


/*
 * Builtin mainloop.  Use this if not using another library's event handling routine
 */
int
rptp_main_loop()
{
	fd_set		read_fds;
	fd_set		write_fds;
	int			i, n;

	looping = 1;
	main_loop_return_value = 0;
	while (looping)
	{
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);

		for (i = 0; i < FD_SETSIZE; i++)
		{
			if (group[i].olist)
				FD_SET(i, &write_fds);
			if (group[i].rcallback)
				FD_SET(i, &read_fds);
		}

		n = select(FD_SETSIZE, &read_fds, &write_fds, NULL, NULL);
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}

		for (i = 0; i < FD_SETSIZE && n; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{
				n--;
				if (group[i].rcallback)
					rptp_async_process(i, RPTP_ASYNC_READ);
			}
			if (FD_ISSET(i, &write_fds))
			{
				n--;
				if (group[i].writing)
					rptp_async_process(i, RPTP_ASYNC_WRITE);
			}
		}
	}
	return main_loop_return_value;
}


/*
 * Terminate the mainloop at the earliest convenient time.  The integer parameter
 * will be returned by mainloop()
 */
#ifdef __STDC__
void
rptp_stop_main_loop(int return_value)
#else
void
rptp_stop_main_loop(return_value)
	int return_value;
#endif
{
	looping = 0;
	main_loop_return_value = return_value;
}


/*
 * Write a line to the RPTP connection asynchronously.
 *
 * "\r\n" is appended to the string.
 */
#ifdef __STDC__
int
rptp_async_putline(int rptp_fd, cb callback, char *fmt, ...)
#else
int
rptp_async_putline(va_alist)
va_dcl
#endif
{
	va_list		args;
	char		buf[RPTP_MAX_LINE];

#ifdef __STDC__
	va_start(args, fmt);
#else
	int			rptp_fd;
	char		*fmt;
	cb			callback;
	char		*tmp;
	va_start(args);
	rptp_fd = va_arg(args, int);
	tmp = va_arg(args, char *);
	callback = (cb) tmp;
	fmt = va_arg(args, char *);
#endif

	if (rptp_fd < 0 || rptp_fd >= FD_SETSIZE)
	{
		rptp_errno = RPTP_ERROR_SOCKET;
		return -1;
	}

	rptp_errno = RPTP_ERROR_NONE;
	
	vsprintf(buf, fmt, args);
	va_end(args);
	strcat(buf, "\r\n");

	return rptp_async_write(rptp_fd, callback, buf, strlen(buf)) != strlen(buf) ? -1 : 0;
}


/*
 * Write nbytes to the RPTP connection asynchronously
 *
 * If a callback function is given, it will be called as soon as all
 * nbytes have been written.
 */
#ifdef __STDC__
int
rptp_async_write(int rptp_fd, cb callback, char *ptr, int nbytes)
#else
int
rptp_async_write(rptp_fd, callback, ptr, nbytes)
    int rptp_fd;
    cb callback;
    char *ptr;
    int nbytes;
#endif
{
	obuf	*new;

	if (rptp_fd < 0 || rptp_fd >= FD_SETSIZE)
	{
		rptp_errno = RPTP_ERROR_SOCKET;
		return -1;
	}

	/*
	 * Create a new obuf with a copy of the data that needs to be written
	 */
	new = (obuf *) malloc(sizeof(obuf));
	new->next = NULL;
	if (nbytes > 0 && ptr)
	{
		new->data = malloc(nbytes);
		memcpy(new->data, ptr, nbytes);
	}
	else						/* allow empty data */
	{
		new->data = NULL;
	}
	new->current = new->data;
	new->length = nbytes;
	new->callback = callback;

	/*
	 * Append the obuf to the list of obufs for the given file descriptor
	 */
	if (group[rptp_fd].otail)
	{
		group[rptp_fd].otail->next = new;
		group[rptp_fd].otail = new;
	}
	else
	{
		group[rptp_fd].otail = group[rptp_fd].olist = new;
	}

	/*
	 * If the output system is already writing we don't need to do anything else,
	 * but if not, we need to register our output function
	 */
	if (!group[rptp_fd].writing)
	{
		do_register(rptp_fd);
	}
}


/*
 * Register a callback for I/O operations on a file descriptor.
 * There are two modes of operation for this call.  The mode
 * depends on whether you use rptp_main_loop() or not.
 * Case 1 (using rptp_main_loop()):
 *   Use rptp_async_register to register a callback for reading
 *   or writing to a file descriptor.  In this case the
 *   what parameter is one of RPTP_ASYNC_READ or RPTP_ASYNC_WRITE | RPTP_ASYNC_RAW.
 *   A typical prototype for the callback will be something like:
 *      void callback(int fd);
 * Case 2 (NOT using rptp_main_loop()):
 *   In this case, only RPTP_ASYNC_WRITE should be used.  This will register
 *   a callback which will have different parameters than the previous case.
 *      void callback(int rptp_fd, int action);
 *   The action parameter will be one of RPTP_ASYNC_ENABLE or RPTP_ASYNC_DISABLE.
 *   If it is RPTP_ASYNC_ENABLE, the callback is supposed to register the rptp_fd
 *   file descriptor with whatever I/O processing system the application uses.
 *   For RPTP_ASYNC_DISABLE, it should be unregistered.
 */
#ifdef __STDC__
void
rptp_async_register(int rptp_fd, int what, cb callback)
#else
void
rptp_async_register(rptp_fd, what, callback)
    int rptp_fd;
    int what;
    cb callback;
#endif
{
	if (rptp_fd < 0 || rptp_fd >= FD_SETSIZE)
	{
		rptp_errno = RPTP_ERROR_SOCKET;
		return;
	}

	switch (what)
	{
		case RPTP_ASYNC_WRITE:
			group[rptp_fd].wcallback = callback;
			group[rptp_fd].wraw = 0;
			break;

		case RPTP_ASYNC_WRITE | RPTP_ASYNC_RAW:
			group[rptp_fd].wcallback = callback;
			group[rptp_fd].wraw = 1;
			break;

		case RPTP_ASYNC_READ:
			group[rptp_fd].rcallback = callback;
			group[rptp_fd].rraw = 1;
			break;
	}
}


/*
 * This gets called to let us know that some I/O can be preformed.
 * When rptp_main_loop() is not used, this function needs to be called
 * when either data is available or data can be written to a file descriptor.
 * The what parameter is one of RPTP_ASYNC_READ or RPTP_ASYNC_WRITE.
 * When rptp_main_loop() is used, this function should NEVER be called
 * by the application code.
 */
#ifdef __STDC__
void
rptp_async_process(int rptp_fd, int what)
#else
void
rptp_async_process(rptp_fd, what)
    int rptp_fd;
    int what;
#endif
{
	if (rptp_fd < 0 || rptp_fd >= FD_SETSIZE)
	{
		rptp_errno = RPTP_ERROR_SOCKET;
		return;
	}

	switch (what)
	{
		case RPTP_ASYNC_READ:
			if (group[rptp_fd].rraw && group[rptp_fd].rcallback)
				(*group[rptp_fd].rcallback)(rptp_fd);
			else
				read_proc(rptp_fd);
			break;

		case RPTP_ASYNC_WRITE:
			if (group[rptp_fd].wraw && group[rptp_fd].wcallback)
				(*group[rptp_fd].wcallback)(rptp_fd);
			else
				write_proc(rptp_fd);
			break;
	}
}


/*
 * Register a callback which will be called when data from a RPTP connection
 * becomes available.
 * The mask parameter specifies what kinds of events will be looked for and is
 * the logical OR of RPTP_EVENT_* constants.
 * The callback function takes 3 parameters. A typical prototype is something
 * like:
 *    void callback(int rptp_fd, int event, char *string);
 */
#ifdef __STDC__
void
rptp_async_notify(int rptp_fd, int mask, void (*callback)())
#else
void
rptp_async_notify(rptp_fd, mask, callback)
    int rptp_fd;
    int mask;
    void (*callback)();
#endif
{
	char	command[RPTP_MAX_LINE];
	char	*p;

	strcpy (command, "set notify=");

	if (mask & RPTP_EVENT_CONTINUE)
		strcat(command, "continue,");
	if (mask & RPTP_EVENT_DONE)
		strcat(command, "done,");
	if (mask & RPTP_EVENT_PAUSE)
		strcat(command, "pause,");
	if (mask & RPTP_EVENT_PLAY)
		strcat(command, "play,");
	if (mask & RPTP_EVENT_SKIP)
		strcat(command, "skip,");
	if (mask & RPTP_EVENT_STATE)
		strcat(command, "state,");
	if (mask & RPTP_EVENT_STOP)
		strcat(command, "stop,");
	if (mask & RPTP_EVENT_VOLUME)
		strcat(command, "volume,");
	if (mask & RPTP_EVENT_FLOW)
		strcat(command, "flow,");
	if (mask & RPTP_EVENT_MODIFY)
		strcat(command, "modify,");
	if (mask & RPTP_EVENT_LEVEL)
		strcat(command, "level,");
	if (mask & RPTP_EVENT_POSITION)
		strcat(command, "position,");

	p = strrchr(command, ',');
	if (!p)
		strcat(command, "none");
	else
		*p = '\0';

	rptp_async_putline(rptp_fd, NULL, command);

	group[rptp_fd].rcallback = callback;
	group[rptp_fd].notify_mask = mask;
	group[rptp_fd].rraw = 0;
}


/*-------------------------------------------------------------------------
 * Start of static routines.  All functions below are only utility routines
 * for this module
 */

/*
 * Attempt to write data from the buffers to an fd.
 */
#ifdef __STDC__
static void
write_proc(int rptp_fd)
#else
static void
write_proc(rptp_fd)
#endif
{
	obuf	*current;
	int		n;

	if (!group[rptp_fd].olist)
	{
		do_unregister(rptp_fd);
	}

	current = group[rptp_fd].olist;
	if (current->current)
	{
		n = write(rptp_fd, current->current, current->length);
		if (n < 0)
		{
			return;
		}
	}
	else
	{
		n = 0;
	}
	current->length -= n;
	current->current += n;
	if (current->length <= 0)
	{
		if (current->callback)
		{
			(*current->callback)(rptp_fd);
		}
		if (current->data)
		{
			free(current->data);
		}
		group[rptp_fd].olist = current->next;
		if (group[rptp_fd].otail == current)
		{
			group[rptp_fd].otail = NULL;
		}
		free(current);

		if (!group[rptp_fd].olist)
		{
			do_unregister(rptp_fd);
		}
	}
}


/*
 * Attempt to read data from an fd to the buffer.
 */
#ifdef __STDC__
static void
read_proc(int rptp_fd)
#else
static void
read_proc(rptp_fd)
#endif
{
	char		buffer[RPTP_MAX_LINE];
	int			n;
	ibuf		*new;

	n = read(rptp_fd, buffer, sizeof(buffer));
	if (n <= 0)
	{
		if (group[rptp_fd].notify_mask & RPTP_EVENT_CLOSE && group[rptp_fd].rcallback)
		{
			(*group[rptp_fd].rcallback)(rptp_fd, RPTP_EVENT_CLOSE, "");
			group[rptp_fd].rcallback = NULL;
		}
		return;		/* Don't do anything when we get an error.  We'll get it later */
	}

	new = (ibuf *) malloc(sizeof(ibuf));
	new->next = NULL;
	new->data = malloc(n);
	memcpy(new->data, buffer, n);
	new->length = n;
	new->current = new->data;

	if (group[rptp_fd].itail)
	{
		group[rptp_fd].itail->next = new;
		group[rptp_fd].itail = new;
	}
	else
	{
		group[rptp_fd].itail = group[rptp_fd].ilist = new;
	}

	process_input(rptp_fd);
}


/*
 * Attempt to make complete lines from the input buffers for the given
 * file descriptor.
 */
#ifdef __STDC__
static void
process_input(int rptp_fd)
#else
static void
process_input(rptp_fd)
    int rptp_fd;
#endif
{
	char		*p;
	ibuf		*ptr;
	static char	line[RPTP_MAX_LINE * 2] = "";
	static int	length = 0;

	while ((ptr = group[rptp_fd].ilist))
	{
		if (p = memchr(ptr->current, '\r', ptr->length))
		{
			memcpy(&line[length], ptr->current, p - ptr->current);
			length += p - ptr->current;
			line[length] = '\0';
			notify_line(rptp_fd, line);
			length = 0;
			
			p+= 2;
			if (p < ptr->current + ptr->length)
			{
				/*
				 * There is still unused data in this block.
				 */
				ptr->length -= p - ptr->current;
				ptr->current = p;
			}
			else
			{
				free(ptr->data);
				if (ptr == group[rptp_fd].itail)
				{
					group[rptp_fd].ilist = group[rptp_fd].itail = NULL;
				}
				else
				{
					group[rptp_fd].ilist = ptr->next;
				}
				free(ptr);
			}
		}
		else
		{
			/*
			 * The current block of data doesn't contain any carriage
			 * returns.  We'll just add it to our line and get rid of it.
			 */
			memcpy(&line[length], ptr->current, ptr->length);
			length += ptr->length;
			free(ptr->data);
			if (ptr == group[rptp_fd].itail)
			{
				group[rptp_fd].ilist = group[rptp_fd].itail = NULL;
			}
			else
			{
				group[rptp_fd].ilist = ptr->next;
			}
			free(ptr);
		}
	}
}


/*
 * A mapping between event and event mask
 */
static struct
{
	char	*event;
	int		mask;
} events[] =
{
	{"continue",	RPTP_EVENT_CONTINUE},
	{"done",		RPTP_EVENT_DONE},
	{"pause",		RPTP_EVENT_PAUSE},
	{"play",		RPTP_EVENT_PLAY},
	{"skip",		RPTP_EVENT_SKIP},
	{"state",		RPTP_EVENT_STATE},
	{"stop",		RPTP_EVENT_STOP},
	{"volume",		RPTP_EVENT_VOLUME},
	{"flow",		RPTP_EVENT_FLOW},
	{"modify",		RPTP_EVENT_MODIFY},
	{"level",		RPTP_EVENT_LEVEL},
	{"position",	RPTP_EVENT_POSITION},
	{NULL,			0}
};


/*
 * Tell the application that a line has arrived from a connection
 */
#ifdef __STDC__
static void
notify_line(int rptp_fd, char *line)
#else
static void
notify_line(rptp_fd, line)
    int rptp_fd;
    char *line;
#endif
{
	int		what;
	char	*event;
	int		i;

	/*
	 * Make sure that there is a callback registered.  No use doing
	 * anything without one...
	 */
	if (!group[rptp_fd].rcallback)
		return;

	switch (*line)
	{
		case '+':
			what = RPTP_EVENT_OK;
			break;
		
		case '-':
			what = RPTP_EVENT_ERROR;
			break;
			
		case '!':
			what = RPTP_EVENT_TIMEOUT;
			break;
		
		case '@':
			event = rptp_parse(line, "event");
			what = RPTP_EVENT_OTHER;
			for (i = 0; events[i].event; i++)
			{
				if (strcmp(events[i].event, event) == 0)
				{
					what = events[i].mask;
					break;
				}
			}
			break;

		default:
			what = RPTP_EVENT_OTHER;
			break;
	}
	if (what & group[rptp_fd].notify_mask)
	{
		(*group[rptp_fd].rcallback)(rptp_fd, what, line);
	}
#if 0
	else
	{
		printf("Input line being ignored: '%s'\n", line);
	}
#endif
}


/*
 * Register us with whatever controls the write_fds on the select
 */
#ifdef __STDC__
static void
do_register(int rptp_fd)
#else
static void
do_register(rptp_fd)
    int rptp_fd;
#endif
{
	if (group[rptp_fd].wcallback)
	{
		(*group[rptp_fd].wcallback)(rptp_fd, RPTP_ASYNC_ENABLE);
	}
	group[rptp_fd].writing = 1;
}


/*
 * Unregister us with whatever controls the write_fds on the select
 */
#ifdef __STDC__
static void
do_unregister(int rptp_fd)
#else
static void
do_unregister(rptp_fd)
    int rptp_fd;
#endif
{
	if (group[rptp_fd].wcallback)
	{
		(*group[rptp_fd].wcallback)(rptp_fd, RPTP_ASYNC_DISABLE);
	}
	group[rptp_fd].writing = 0;
}


/*
 * Local variables:
 * tab-width: 4
 * End:
 */
