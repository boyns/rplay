/* timer.h - Definitions for timer.c.  */

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



#ifndef _timer_h
#define _timer_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

extern double timer_rate;
extern double timer_count;
extern int timer_enabled;

#ifdef __STDC__
extern void timer_update ();
extern void timer_init ();
extern void timer_block ();
extern void timer_unblock ();
extern void timer_start (int interval);
extern void timer_restart (int interval);
extern void timer_stop ();
#else
extern void timer_update ();
extern void timer_init ();
extern void timer_block ();
extern void timer_unblock ();
extern void timer_start ( /* int interval */ );
extern void timer_restart ( /* int interval */ );
extern void timer_stop ();
#endif

#endif /* _timer_h */
