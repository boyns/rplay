/* $Id: misc.h,v 1.2 1998/08/13 06:13:57 boyns Exp $ */

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



#ifndef _misc_h
#define _misc_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>

#ifdef __STDC__
extern char *sys_err_str (int error);
extern int udp_socket (int port);
extern int tcp_socket (int port);
extern void fd_nonblock (int fd);
extern void fd_block (int fd);
extern int modified (char *filename, time_t since);
extern char *time2string (time_t t);
extern int string_to_audio_format (char *string);
extern char *audio_format_to_string (int format);
extern int string_to_byte_order (char *string);
extern char *byte_order_to_string (int byte_order);
extern int string_to_storage (char *string);
extern char *storage_to_string (int storage);
extern int string_to_input (char *string);
extern char *input_to_string (int input);
extern char *audio_port_to_string (int port);
#else
extern char *sys_err_str ( /* int error */ );
extern int udp_socket ( /* int port */ );
extern int tcp_socket ( /* int port */ );
extern void fd_nonblock ( /* int fd */ );
extern void fd_block ( /* int fd */ );
extern int modified ( /* char *filename, time_t since */ );
extern char *time2string ( /* time_t t */ );
extern int string_to_audio_format ( /* char *string */ );
extern char *audio_format_to_string ( /* int format */ );
extern int string_to_byte_order ( /* char *string */ );
extern char *byte_order_to_string ( /* int byte_order */ );
extern int string_to_storage ( /* char *string */ );
extern char *storage_to_string ( /* int storage */ );
extern int string_to_input ( /* char *string */ );
extern char *input_to_string ( /* int input */ );
extern char *audio_port_to_string ( /* int port */ );
#endif

unsigned short little_short ( /* char *p */ );
unsigned short big_short ( /* char *p */ );
unsigned long little_long ( /* char *p */ );
unsigned long big_long ( /* char *p */ );
double ConvertFromIeeeExtended ( /* unsigned char *bytes */ );

#endif /* _misc_h */
