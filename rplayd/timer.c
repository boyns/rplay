/* $Id: timer.c,v 1.3 1998/11/07 21:15:41 boyns Exp $ */

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



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <stdio.h>
#include <unistd.h>
#include "rplayd.h"
#include "timer.h"

double timer_rate = 0.0;
double timer_count = 0.0;
int timer_enabled = 0;

void
timer_update()
{
#ifdef HAVE_OSS
    int bytes = rplay_audio_getospace_bytes();
    /* check bytes and sample size? */
    if (bytes > 0)
    {
	rplayd_write(bytes);
    }
    /* if audio device isn't open, just pretend to keep spool going */
    else if (!rplay_audio_isopen())
    {
	/* approximate the amount to be written */
	int sample_size = (rplay_audio_precision >> 3) * rplay_audio_channels;
	rplayd_write(rplay_audio_sample_rate / curr_rate * sample_size);
    }
#else
    rplayd_write(curr_bufsize);
#endif

    timer_count += timer_rate;
}

void
timer_init()
{
    struct sigaction sa;

    sa.sa_handler = timer_update;
    sa.sa_flags = 0;
#if defined(SA_RESTART) && ! defined(sgi)
    sa.sa_flags |= SA_RESTART;
#endif
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGALRM);
    if (sigaction(SIGALRM, &sa, (struct sigaction *) NULL) < 0)
    {
	report(REPORT_ERROR, "timer_init: sigaction: %s\n", sys_err_str(errno));
	done(1);
    }

    timer_enabled = 0;
}

void
timer_block()
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0)
    {
	report(REPORT_ERROR, "timer_block: sigsetprocmask: %s\n", sys_err_str(errno));
	done(1);
    }
}

void
timer_unblock()
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);

    if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0)
    {
	report(REPORT_ERROR, "timer_block: sigsetprocmask: %s\n", sys_err_str(errno));
	done(1);
    }
}

#ifdef __STDC__
void
timer_restart(int new_rate)
#else
void
timer_restart(new_rate)
    int new_rate;
#endif
{
    struct itimerval it;

    timer_enabled = 1;

    if (new_rate <= 0)
    {
	report(REPORT_ERROR, "timer_restart: new_rate=%d, must be >= 1\n", new_rate);
	done(1);
    }
    else if (new_rate == 1)
    {
	it.it_interval.tv_sec = 1;
	it.it_interval.tv_usec = 0;
    }
    else
    {
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 1000000 / new_rate;
    }
    it.it_value = it.it_interval;
    if (setitimer(ITIMER_REAL, &it, NULL) < 0)
    {
	report(REPORT_ERROR, "timer_restart: setitimer: %s\n", sys_err_str(errno));
	done(1);
    }

    if (it.it_interval.tv_sec)
    {
	timer_rate = it.it_interval.tv_sec;
    }
    else
    {
	timer_rate = (double) it.it_interval.tv_usec / 1000000;
    }
}

#ifdef __STDC__
void
timer_start(int new_rate)
#else
void
timer_start(new_rate)
    int new_rate;
#endif
{
    timer_restart(new_rate);
}

void
timer_stop()
{
    struct itimerval it;

    timer_enabled = 0;

    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value = it.it_interval;
    if (setitimer(ITIMER_REAL, &it, NULL) < 0)
    {
	report(REPORT_ERROR, "timer_stop: setitimer: %s\n", sys_err_str(errno));
	done(1);
    }
}
