/* 
 * Copyright (C) 1993 Raphael Quinet (quinet@montefiore.ulg.ac.be)
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

#include "xjukebox.h"
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/List.h>

#ifdef __STDC__
extern void setBusy(Boolean busy);
extern void add_spool_item(String new_info, spool_info ***list, String **nlist, int *items_count);
extern void free_spool_items(spool_info ***list, String **nlist, int *items_count);
#else
extern void setBusy();
extern void add_spool_item();
extern void free_spool_items();
#endif

extern String       *sound_names;
extern int           sound_count;
extern String       *queue_names;
extern int           queue_count;
extern String       *spool_names;
extern int           spool_count;
extern Widget        sound_list;
extern Widget        queue_list;
extern Widget        spool_list;
extern int           return_code;
extern XtAppContext  app_context;

XtInputId       input_id;
XtIntervalId    interval_id = NO_INTERVAL;
int             rptp_port = RPTP_PORT;
char           *rptp_host = NULL;
int             rptp_fd = -1;
char            rptp_buf[4096];
char            rptp_resp[RPTP_MAX_LINE];
int             rptp_mode = MODE_IDLE;
int             old_spool_count = -1;
spool_info    **spool_info_list;
static String   empty_spool[] = { "", NULL };
int             playing_sid = -1;

#ifdef __STDC__
void rptp_callback(XtPointer client_data, int *input, XtInputId *input_id);
void rptp_get_spool_list(XtPointer client_data, XtIntervalId *id);
#else
void rptp_callback();
void rptp_get_spool_list();
#endif

/*****************************************************************************/
#ifdef __STDC__
void setSpoolUpdate(Boolean update)
#else
void setSpoolUpdate(update)
     Boolean update;
#endif
{
  if (update)
    {
      if (interval_id == NO_INTERVAL)
	interval_id = XtAppAddTimeOut(app_context, 1000,
				      rptp_get_spool_list, NULL);
    }
  else
    {
      if (interval_id != NO_INTERVAL)
	XtRemoveTimeOut(interval_id);
      interval_id = NO_INTERVAL;
    }
}


/*****************************************************************************/
void rptp_disconnect()
{
  setSpoolUpdate(False);
  if (rptp_fd != -1)
    {
      XtRemoveInput(input_id);
      rptp_close(rptp_fd);
      rptp_fd = -1;
    }
}


/*****************************************************************************/
#ifdef __STDC__
void rptp_connect(char *hostname)
#else
void rptp_connect(hostname)
     char *hostname;
#endif
{
  rptp_disconnect();

  if ((hostname == NULL) || (hostname[0] == '\0'))
    rptp_host = rplay_default_host();
  else
    rptp_host = hostname;

  rptp_fd = rptp_open(rptp_host, rptp_port, rptp_resp, sizeof(rptp_resp));
  if (rptp_fd < 0)
    {
      rptp_perror("*** open");
      return; /* !!!! */
    }

  printf("Connected to %s port %d.\n", rptp_host, rptp_port);
  printf("%s\n", rptp_resp + 1);
  input_id = XtAppAddInput(app_context, rptp_fd,
			   (XtPointer) XtInputReadMask,
			   rptp_callback, NULL);
  old_spool_count = -1;
  /* setSpoolUpdate(True);  Loose, loose ! */
}


/*****************************************************************************/
int rptp_reconnect()
{
  printf("*** Error in connection with %s.  Trying to reconnect...\n",
	 rptp_host);

  rptp_disconnect();

  if (rptp_host == NULL)
    printf("BANG !  A bug...  (rptp_host = NULL)\n");

  rptp_fd = rptp_open(rptp_host, rptp_port, rptp_resp, sizeof(rptp_resp));
  if (rptp_fd < 0)
    {
      rptp_perror("*** reopen");
      return -1;
    }
  printf("Reconnected to %s port %d.\n", rptp_host, rptp_port);
  printf("%s\n", rptp_resp + 1);
  input_id = XtAppAddInput(app_context, rptp_fd,
			   (XtPointer) XtInputReadMask,
			   rptp_callback, NULL);
  old_spool_count = -1;
  setSpoolUpdate(True);
  return 1;
}


/*****************************************************************************/
#ifdef __STDC__
void rptp_play_sound(char *name)
#else
void rptp_play_sound(name)
     char *name;
#endif
{
  sprintf(rptp_buf, "play sound=%s", name);
/*  printf("%s\n", rptp_buf); */
  switch (rptp_command(rptp_fd, rptp_buf, rptp_resp, sizeof(rptp_resp)))
    {
    case -1:
      /* !!!!!!!!!! */
      rptp_perror("play");
      if (rptp_reconnect() > 0)
	rptp_play_sound(name);
      break;

    case 1:
      printf("?? %s\n", rptp_resp + 1);
      break;
      
    case 0:
	playing_sid = atoi (rptp_parse (rptp_resp, "id") + 1);
      break;
    }
}


/*****************************************************************************/
#ifdef __STDC__
void rptp_stop_sound(int spool_index)
#else
void rptp_stop_sound(spool_index)
     int spool_index;
#endif
{
  sprintf(rptp_buf, "stop id=#%d", spool_info_list[spool_index]->sid);
/*  printf("%s\n", rptp_buf); */
  switch (rptp_command(rptp_fd, rptp_buf, rptp_resp, sizeof(rptp_resp)))
    {
    case -1:
      /* !!!!!!!!!! */
      rptp_perror("stop");
      if (rptp_reconnect() > 0)
	rptp_stop_sound(spool_index);
      break;

    case 1:
      printf("!! %s\n", rptp_resp + 1);
      break;
      
    case 0:
      break;
    }
}


/*****************************************************************************/
#ifdef __STDC__
void rptp_pause_sound(int spool_index)
#else
void rptp_pause_sound(spool_index)
     int spool_index;
#endif
{
  sprintf(rptp_buf, "pause id=#%d", spool_info_list[spool_index]->sid);
/*  printf("%s\n", rptp_buf); */
  switch (rptp_command(rptp_fd, rptp_buf, rptp_resp, sizeof(rptp_resp)))
    {
    case -1:
      /* !!!!!!!!!! */
      rptp_perror("pause");
      if (rptp_reconnect() > 0)
	rptp_pause_sound(spool_index);
      break;

    case 1:
      printf("*!! %s\n", rptp_resp + 1);
      break;
      
    case 0:
      break;
    }
}


/*****************************************************************************/
#ifdef __STDC__
void rptp_continue_sound(int spool_index)
#else
void rptp_continue_sound(spool_index)
     int spool_index;
#endif
{
  sprintf(rptp_buf, "continue id=#%d", spool_info_list[spool_index]->sid);
/*  printf("%s\n", rptp_buf); */
  switch (rptp_command(rptp_fd, rptp_buf, rptp_resp, sizeof(rptp_resp)))
    {
    case -1:
      /* !!!!!!!!!! */
      rptp_perror("continue");
      if (rptp_reconnect() > 0)
	rptp_continue_sound(spool_index);
      break;

    case 1:
      printf("*!! %s\n", rptp_resp + 1);
      break;
      
    case 0:
      break;
    }
}


/*****************************************************************************/
void rptp_get_sound_list()
{
  if (rptp_fd == -1)
    {
      printf("Aaaargh !\n");
      /* !!!!!!! */
      return;
    }

  switch (rptp_command(rptp_fd, "list sounds", rptp_resp, sizeof(rptp_resp)))
    {
    case -1:
      /* !!!!!!!!!! */
      rptp_perror("get_sound_list");
      rptp_disconnect();
      return;
      
    case 1:
      printf("get_sound_list : %s\n---\n", rptp_resp + 1);
/*      rptp_mode = MODE_READING_SOUNDS; */
      break;
      
    case 0:
      rptp_mode = MODE_READING_SOUNDS;
      break;
    }
  sound_names = NULL;
  setSpoolUpdate(False);   /* Quick hack... */
}


/*****************************************************************************/
#ifdef __STDC__
void rptp_get_spool_list(XtPointer client_data, XtIntervalId *id)
#else
void rptp_get_spool_list(client_data, id)
     XtPointer     client_data;
     XtIntervalId *id;
#endif
{
  interval_id = NO_INTERVAL;
  if (rptp_fd == -1)
    {
      printf("Aaaargh !\n");
      /* !!!!!!! */
      return;
    }

  switch (rptp_command(rptp_fd, "list spool", rptp_resp, sizeof(rptp_resp)))
    {
    case -1:
      /* !!!!!!!!!! */
      rptp_perror("get_spool_list");
      rptp_disconnect();
      return;
      
    case 1:
      printf("get_spool_list : %s\n---\n", rptp_resp + 1);
/*      rptp_mode = MODE_READING_SPOOL; */
      break;
      
    case 0:
      rptp_mode = MODE_READING_SPOOL;
      break;
    }
  free_spool_items(&spool_info_list, &spool_names, &spool_count);
}


/*****************************************************************************/
#ifdef __STDC__
void rptp_callback(XtPointer client_data, int *input, XtInputId *id)
#else
void rptp_callback(client_data, input, id)
     XtPointer client_data;
     int *input;
     XtInputId *id;
#endif
{
  int  n, i;

  switch (rptp_mode)
    {
    case MODE_IDLE :
      printf("*** Unexpected event !  Connection timed out ?\n");
      sleep(1); /* !!!!!!!!!! */
/*
      n = rptp_getline(rptp_fd, rptp_buf, sizeof(rptp_buf));
      printf("    n = %d, rptp_buf = \"%s\"\n", n, rptp_buf);
*/
      break;
    case MODE_READING_SOUNDS :
      n = rptp_getline(rptp_fd, rptp_buf, sizeof(rptp_buf));
      if (n < 0)
	{
	  /* !!!!!!!!!! */
	  rptp_perror("list sounds");
	  return;
	}
      if (! strcmp(rptp_buf, ".")) 
	{
	  rptp_mode = MODE_IDLE;
	  XawListChange(sound_list, sound_names, sound_count, 0, True);
	  setBusy(False);
	  setSpoolUpdate(True);
	  return;
	}
      add_item(rptp_parse (rptp_buf, "sound"), &sound_names, &sound_count);
      break;
    case MODE_READING_SPOOL :
      n = rptp_getline(rptp_fd, rptp_buf, sizeof(rptp_buf));
      if (n < 0)
	{
	  /* !!!!!!!!!! */
	  rptp_perror("list spool");
	  return;
	}
      if (! strcmp(rptp_buf, ".")) 
	{
	  rptp_mode = MODE_IDLE;
	  if ((spool_count > 0) || (old_spool_count != 0))
	    {
	      if (spool_count > 0)
		XawListChange(spool_list, spool_names, spool_count, 0, True);
	      else
		XawListChange(spool_list, empty_spool, 1, 0, True);
	      old_spool_count = spool_count;
	    }
	  if (playing_sid > 0)
	    {
	      for (i = 0; i < spool_count; i++)
		if (spool_info_list[i]->sid == playing_sid)
		  break;
	      if (i >= spool_count)  /* Sound not found, play next one */
		{
		  playing_sid = -1;
		  if (queue_count > 0)
		    {
		      rptp_play_sound(queue_names[0]);
		      delete_item(0, &queue_names, &queue_count);
		      XawListChange(queue_list, queue_names, queue_count,
				    0, True);
		    }
		}
	    }
	  setSpoolUpdate(True);
	  return;
	}
      add_spool_item(rptp_buf, &spool_info_list, &spool_names, &spool_count);
      break;
    default :
      printf("*** Unknown mode : BUG !\n");
      break;
    }
}
