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

extern XtAppContext  app_context;

#ifdef __STDC__
extern void rptp_connect(char *hostname);
extern void rptp_get_sound_list();
extern void init_application(char *app_name, int argc, char *argv[]);
#else
extern void rptp_connect();
extern void rptp_get_sound_list();
extern void init_application();
#endif

String        *sound_names;
int            sound_count;
String        *queue_names;
int            queue_count;
String        *spool_names;
int            spool_count;
static String  empty_list[] = { "< No sounds >", NULL };
char          *program_name;
int            return_code;

/*****************************************************************************/
#ifdef __STDC__
void add_item(String new_item, String **list, int *items_count)
#else
void add_item(new_item, list, items_count)
     String   new_item;
     String **list;
     int     *items_count;
#endif
{
  if ((*list != NULL)  && (*list != empty_list))
    *list = (String *)realloc(*list, (*items_count + 1) * sizeof(String));
  else
    {
      *items_count = 0;
      *list = (String *)malloc(sizeof(String));
    }
  (*list)[*items_count] =
    (String)malloc((strlen(new_item) + 1) * sizeof(char));
  strcpy((*list)[*items_count], new_item);
  (*items_count)++;
}

/*****************************************************************************/
#ifdef __STDC__
void delete_item(int list_index, String **list, int *items_count)
#else
void delete_item(list_index, list, items_count)
     int      list_index;
     String **list;
     int     *items_count;
#endif
{
  int i;

  free((*list)[list_index]);
  if (*items_count > 1)
    {
      for (i = list_index; i < *items_count - 1; i++)
	(*list)[i] = (*list)[i + 1];
      *list = (String *)realloc(*list, (*items_count - 1) * sizeof(String));
    }
  else
    {
      free(*list);
      *list = empty_list;
      *items_count = 0;
    }
  (*items_count)--;
}

/*****************************************************************************/
#ifdef __STDC__
void move_item(int old_index, int new_index, String **list)
#else
void move_item(old_index, new_index, list)
     int      old_index;
     int      new_index;
     String **list;
#endif
{
  int    i;
  String tmp_s;

  tmp_s = (*list)[old_index];
  if (new_index < old_index)
    {
      for (i = old_index; i > new_index; i--)
	(*list)[i] = (*list)[i - 1];
    }
  else
    {
      for (i = old_index; i < new_index; i++)
	(*list)[i] = (*list)[i + 1];
    }
  (*list)[new_index] = tmp_s;
}


/*****************************************************************************/
#ifdef __STDC__
void load_list(String file_name, String **list, int *items_count)
#else
void load_list(file_name, list, items_count)
     String   file_name;
     String **list;
     int     *items_count;
#endif
{
  FILE *list_file;
  char  buffer[1024];

  list_file = fopen(file_name, "r");
  if (list_file == NULL)
    {
      /* !!! */
      return;
    }
  *list = empty_list;
  while (fscanf(list_file, "%1023s", buffer) > 0)
    add_item(buffer, list, items_count);
  fclose(list_file);
}


/*****************************************************************************/
#ifdef __STDC__
void save_list(String file_name, String *list, int items_count)
#else
void save_list(file_name, list, items_count)
     String   file_name;
     String  *list;
     int      items_count;
#endif
{
  FILE *list_file;
  int   i;

  list_file = fopen(file_name, "w");
  if (list_file == NULL)
    {
      /* !!! */
      return;
    }
  for (i = 0; i < items_count; i++)
    fprintf(list_file, "%s\n", list[i]);
  fclose(list_file);
}


/*****************************************************************************/
#ifdef __STDC__
int alphacompare(char **i, char **j)
#else
int alphacompare(i, j)
     char **i;
     char **j;
#endif
{
   return strcmp(*i, *j);
}


/*****************************************************************************/
#ifdef __STDC__
void add_spool_item(String new_info, spool_info ***list, String **nlist, int *items_count)
#else
void add_spool_item(new_info, list, nlist, items_count)
     String        new_info;
     spool_info ***list;
     String      **nlist;
     int          *items_count;
#endif
{
  spool_info *new_item;
  char        state_buf[10];
  char        name_buf[1024];
  int         n;

  new_item = (spool_info *)malloc(sizeof(spool_info));

  new_item->sid = atoi (1 + rptp_parse (new_info, "id"));
  new_item->vol = atoi (rptp_parse (NULL, "volume"));
  new_item->pri = atoi (rptp_parse (NULL, "priority"));
  new_item->count = atoi (rptp_parse (NULL, "count"));
  new_item->seconds = atoi (rptp_parse (NULL, "seconds"));
  new_item->remain = atoi (rptp_parse (NULL, "remain"));
  strcpy (new_item->host, rptp_parse (NULL, "host"));
  strcpy (state_buf, rptp_parse (NULL, "state"));
  strcpy (name_buf, rptp_parse (NULL, "sound"));
  
  if (!strcmp(state_buf, "play"))
    new_item->state = SPOOL_PLAY;
  else
    if (!strcmp(state_buf, "pause"))
      new_item->state = SPOOL_PAUSE;
    else
      if (!strcmp(state_buf, "(wait)"))
	new_item->state = SPOOL_WAIT;
      else
	new_item->state = SPOOL_UNKNOWN;

  new_item->sound = (char *)malloc((20 + strlen(name_buf)) * sizeof(char));
  if ((new_item->seconds > 0) && (new_item->remain > 0))
    sprintf(new_item->sound, "%6s %3d%% %s", state_buf,
	    100 - ((new_item->remain * 100) / new_item->seconds), name_buf);
  else
    sprintf(new_item->sound, "%6s 100%% %s", state_buf, name_buf);

  if (*list != NULL)
    *list = (spool_info **)realloc(*list, (*items_count + 1) *
				   sizeof(spool_info *));
  else
    {
      *items_count = 0;
      *list = (spool_info **)malloc(sizeof(spool_info *));
    }
  (*list)[*items_count] = new_item;
  if ((*nlist != NULL)  && (*nlist != empty_list))
    *nlist = (String *)realloc(*nlist, (*items_count + 1) * sizeof(String));
  else
    *nlist = (String *)malloc(sizeof(String));
  (*nlist)[*items_count] = new_item->sound;
  (*items_count)++;
}

/*****************************************************************************/
#ifdef __STDC__
void free_spool_items(spool_info ***list, String **nlist, int *items_count)
#else
void free_spool_items(list, nlist, items_count)
     spool_info ***list;
     String      **nlist;
     int          *items_count;
#endif
{
  int i;

  for (i = 0; i < *items_count; i++)
    {
      free((*nlist)[i]);
      free((*list)[i]);
    }
  *list = NULL;
  *nlist = NULL;
  *items_count = 0;
}


/*****************************************************************************/
#ifdef __STDC__
void main(int argc, char *argv[])
#else
void main(argc, argv)
     int   argc;
     char *argv[];
#endif
{
  if ((program_name = strrchr(argv[0], (int) '/')) == NULL)
    program_name = argv[0];
  else
    program_name++;
  return_code = 0;
  sound_names = empty_list;
  spool_names = empty_list;
  queue_names = empty_list;
  init_application("XJukebox", argc, argv);
  rptp_connect(NULL);                     /* Connect to default rplay server */
  rptp_get_sound_list();
  XtAppMainLoop(app_context);
}
