/* $Id: helper.h,v 1.6 2002/12/11 05:12:16 boyns Exp $ */

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

#ifndef _helper_h
#define _helper_h

#ifdef HAVE_HELPERS

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#ifdef HAVE_RX_RXPOSIX_H
#include <rx/rxposix.h>
#else
#ifdef HAVE_RXPOSIX_H
#include <rxposix.h>
#else
#include "rxposix.h"
#endif
#endif
#endif

typedef struct _helper
{
    struct _helper *next;
    regex_t pattern;
    char *program;
    int format;
    int sample_rate;
    int precision;
    int channels;
    int byte_order;
}
HELPER;

extern HELPER *helpers;

extern void helper_read (/* char *filename */);
extern void helper_reread (/* char *filename */);
extern void helper_stat (/* char *filename */);
extern HELPER *helper_lookup (/* char *sound */);

#endif /* HAVE_HELPERS */
#endif /* _helper_h */
