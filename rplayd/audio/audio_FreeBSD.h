/* audio_FreeBSD.h - Definitions for FreeBSD 2.0 / 2.0.5 / 2.1.0 audio stubs. */

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



#ifndef _audio_FreeBSD_h
#define _audio_FreeBSD_h

#include "rplay.h"

#define RPLAY_AUDIO_DEVICE		"/dev/dsp"
#define RPLAY_MIXER_DEVICE		"/dev/mixer"
#define RPLAY_AUDIO_TIMEOUT		3
#define RPLAY_AUDIO_FLUSH_TIMEOUT	-1
#define RPLAY_AUDIO_RATE		10
#define RPLAY_AUDIO_BYTE_ORDER		RPLAY_LITTLE_ENDIAN

#endif /* _audio_FreeBSD_h */