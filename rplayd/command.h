/* $Id: command.h,v 1.2 1998/08/13 06:13:47 boyns Exp $ */

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



#ifndef _command_h
#define _command_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "connection.h"

#ifdef __STDC__
extern int command (CONNECTION *c, char *buf);
#else
extern int command ( /* CONNECTION *c, char *buf */ );
#endif

#endif /* _command_h */
