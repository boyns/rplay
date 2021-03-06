/* $Id: rptp.h,v 1.2 1998/08/13 06:13:22 boyns Exp $ */

/* 
 * Copyright (C) 1995 Mark Boyns <boyns@sdsu.edu>
 *
 * This file is part of rplaytool.
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

#ifndef _rptp_h
#define _rptp_h

#include <rplay.h>

extern int rptp_fd;
extern char *rptp_host;

extern int do_rptp_open ();
extern int do_rptp_pause ();
extern int do_rptp_cont ();
extern int do_rptp_stop ();
extern int do_rptp_volume ();
extern int do_rptp_sounds ();
extern int do_rptp_play ();
extern int do_rptp_play_info ();

#endif /* _rptp_h */
