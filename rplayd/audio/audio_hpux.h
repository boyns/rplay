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



/*
 * The _hpux version was cobbled together from the _generic version by Hendrik
 * (J.C.Harrison@ncl.ac.uk).
 */

#ifndef _audio_hpux_h
#define _audio_hpux_h

#include "rplay.h"

#define RPLAY_AUDIO_DEVICE		"/dev/audio"
#define RPLAY_AUDIO_TIMEOUT		5
#define RPLAY_AUDIO_FLUSH_TIMEOUT	-1
#define RPLAY_AUDIO_RATE		10
#define RPLAY_AUDIO_BYTE_ORDER		RPLAY_BIG_ENDIAN

#endif /* _audio_hpux_h */
