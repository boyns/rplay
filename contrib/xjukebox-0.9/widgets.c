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
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Dialog.h>

#ifdef __STDC__
extern char *rplay_default_host();
extern void  setSpoolUpdate(Boolean update);
extern void  rptp_connect(char *hostname);
extern void  rptp_get_sound_list();
extern void  rptp_get_spool_list();
extern int   alphacompare(char **i, char **j);
#else
extern char *rplay_default_host();
extern void  setSpoolUpdate();
extern void  rptp_connect();
extern void  rptp_get_sound_list();
extern void  rptp_get_spool_list();
extern int   alphacompare();
#endif

extern String *sound_names;
extern int     sound_count;
extern String *queue_names;
extern int     queue_count;
extern String *spool_names;
extern int     spool_count;
extern int     return_code;


#ifdef __STDC__
void wmProtocolsProc(Widget w, XEvent *event, String params[],
		     Cardinal *num_params);
void okProc(Widget w, XEvent *event, String params[],
	    Cardinal *num_params);
void cancelProc(Widget w, XEvent *event, String params[],
		Cardinal *num_params);
#else
void wmProtocolsProc();
void okProc();
void cancelProc();
#endif

Widget         top_shell, jukebox_shell, sound_list, queue_list, spool_list;
Atom           XA_WM_DELETE_WINDOW;
XtAppContext   app_context;
static Boolean closing_top_shell = False;
static int     current_dialog = DIALOG_NONE;
static String  default_file = "";
static String  fallback_resources[] = {
#include "XJukebox.ad.h"
    NULL
};
#define Offset(field) XtOffsetOf(struct _resources_rec, field)
static XtResource resources_desc[] =
  {
    {
      "jukeboxShown", "JukeboxShown", XtRBoolean, sizeof(Boolean),
      Offset(jukebox_shown), XtRImmediate, (XtPointer)FALSE
    }
  };
#undef Offset
static XrmOptionDescRec options[] =
  {
    {
      "-jukeboxShown",   ".jukeboxShown",    XrmoptionNoArg,  "TRUE"
    }
  };
static XtActionsRec actions[] = {
  {
    "WMProtocols",   wmProtocolsProc
  },
  {
    "Ok",            okProc
  },
  {
    "Cancel",        cancelProc
  }
};


/*****************************************************************************/
void ringBell()
{
  XBell(XtDisplay(top_shell), 50);
}


/*****************************************************************************/
#ifdef __STDC__
void setBusy(Boolean busy)
#else
void setBusy(busy)
     Boolean busy;
#endif
{
  static Window         win1 = (Window) 0;
  static Window         win2 = (Window) 0;
  Display              *disp;
  XSetWindowAttributes  xswa;

  disp = XtDisplay(top_shell);
  if (! win1)
    {
      xswa.do_not_propagate_mask = (KeyPressMask | KeyReleaseMask |
				    ButtonPressMask | ButtonReleaseMask |
				    PointerMotionMask);
      xswa.cursor = XCreateFontCursor(disp, XC_watch);
      win1 = XCreateWindow(disp, XtWindow(top_shell),
			   0, 0,
			   (unsigned int) 8192, /* Should be large enough */
			   (unsigned int) 8192,
			   (unsigned int) 0, CopyFromParent,
			   InputOnly, CopyFromParent,
			   CWDontPropagate | CWCursor,
			   &xswa);
      win2 = XCreateWindow(disp, XtWindow(jukebox_shell),
			   0, 0,
			   (unsigned int) 8192,
			   (unsigned int) 8192,
			   (unsigned int) 0, CopyFromParent,
			   InputOnly, CopyFromParent,
			   CWDontPropagate | CWCursor,
			   &xswa);
    }
  if (busy)
    {
      XMapRaised(disp, win1);
      XMapRaised(disp, win2);
    }
  else
    {
      XUnmapWindow(disp, win1);
      XUnmapWindow(disp, win2);
    }
}

/*****************************************************************************/
#ifdef __STDC__
void selectOkProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void selectOkProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  Widget dialog;
  String value;

  dialog = (Widget) client_data;
  value = (String) XawDialogGetValueString(dialog);
  XtDestroyWidget(XtParent(dialog));
  switch (current_dialog)
    {
      case DIALOG_SET_NEW_HOST :
	rptp_connect(value);
	setSpoolUpdate(True);
        break;
      case DIALOG_LOAD_SOUND_FILE :
	load_list(value, &sound_names, &sound_count);
	XawListChange(sound_list, sound_names, sound_count, 0, True);
	default_file = value;
	break;
      case DIALOG_SAVE_SOUND_FILE :
	save_list(value, sound_names, sound_count);
	default_file = value;
	break;
      case DIALOG_GET_SOUND_DIR :
	printf("*** Not implemented : GET_SOUND_DIR : %s\n", value);
	break;
      case DIALOG_LOAD_QUEUE_FILE :
	load_list(value, &queue_names, &queue_count);
	XawListChange(queue_list, queue_names, queue_count, 0, True);
	default_file = value;
	break;
      case DIALOG_SAVE_QUEUE_FILE :
	save_list(value, queue_names, queue_count);
	default_file = value;
	break;
      default :
	printf("*** Congratulations !  You found a bug ! ***\n");
    }
  current_dialog = DIALOG_NONE;
  setBusy(False);
}


/*****************************************************************************/
#ifdef __STDC__
void selectCancelProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void selectCancelProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  Widget dialog;

  dialog = (Widget) client_data;
  XtDestroyWidget(XtParent(dialog));
  current_dialog = DIALOG_NONE;
  setBusy(False);
}


/*****************************************************************************/
#ifdef __STDC__
void popupDialog(Widget button, String message, String default_value, int id)
#else
void popupDialog(button, message, default_value, id)
     Widget button;
     String message;
     String default_value;
     int    id;
#endif
{
  Arg           args[2];
  Widget        popup, dialog;
  Position      x, y;
  Dimension     width, height;

  XtSetArg(args[0], XtNwidth, &width);
  XtSetArg(args[1], XtNheight, &height);
  XtGetValues(button, args, TWO);
  XtTranslateCoords(button,
		    (Position)(width / 2 - 64), (Position)(height / 2 - 32),
		    &x, &y);
  XtSetArg(args[0], XtNx, x);
  XtSetArg(args[1], XtNy, y);
  popup = XtCreatePopupShell((String) "popup",
			     transientShellWidgetClass, button,
			     args, TWO);
  XtSetArg(args[0], XtNlabel, message);
  XtSetArg(args[1], XtNvalue, default_value);
  dialog = XtCreateManagedWidget((String) "dialog",
				 dialogWidgetClass, popup,
				 args, TWO);
  XawDialogAddButton(dialog, "ok", selectOkProc, (XtPointer) dialog);
  XawDialogAddButton(dialog, "cancel", selectCancelProc, (XtPointer) dialog);
  current_dialog = id;
  XtPopup(popup, XtGrabExclusive);
  setBusy(True);
}


/*****************************************************************************/
#ifdef __STDC__
void select_soundProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void select_soundProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  /* Should check double click... (= Add) */
}


/*****************************************************************************/
#ifdef __STDC__
void select_queueProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void select_queueProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  /* Should check double click... (= Play this sound) */
}


/*****************************************************************************/
#ifdef __STDC__
void select_spoolProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void select_spoolProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  /* ... */
}


/*****************************************************************************/
#ifdef __STDC__
void quitProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void quitProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  XtDestroyWidget(top_shell);
  exit(return_code);
}


/*****************************************************************************/
#ifdef __STDC__
void jukeboxProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void jukeboxProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  if (resources.jukebox_shown)
    {
      resources.jukebox_shown = False;
      XtPopdown(jukebox_shell);
     }
  else
    {
      XtPopup(jukebox_shell, XtGrabNone);
      resources.jukebox_shown = True;
    }
}


/*****************************************************************************/
#ifdef __STDC__
void hostProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void hostProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  popupDialog(w, (String) "New rplay host :",
	      (String) rplay_default_host(),
	      DIALOG_SET_NEW_HOST);
  setSpoolUpdate(False);
}


/*****************************************************************************/
#ifdef __STDC__
void aboutProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void aboutProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  Arg           args[5];
  XtCallbackRec callback[2];
  Widget        popup, about, label, button;
  Position      x, y;
  Dimension     width, height;
  char          buffer[100];

  XtSetArg(args[0], XtNwidth, &width);
  XtSetArg(args[1], XtNheight, &height);
  XtGetValues(w, args, TWO);
  XtTranslateCoords(w, (Position)(width / 2 - 64), (Position)(height / 2 - 32),
		    &x, &y);
  XtSetArg(args[0], XtNx, x);
  XtSetArg(args[1], XtNy, y);
  popup = XtCreatePopupShell((String) "popup",
			     transientShellWidgetClass, w,
			     args, TWO);
  about = XtCreateManagedWidget((String) "aboutForm",
				 formWidgetClass, popup,
				 NULL, ZERO);
  sprintf(buffer, "XJukebox %s (C) 1993, Raphael Quinet\n", XJUKEBOX_VERSION);
  XtSetArg(args[0], XtNlabel, buffer);
  XtSetArg(args[1], XtNborderWidth, NULL);
  XtSetArg(args[2], XtNfromHoriz, NULL);
  XtSetArg(args[3], XtNfromVert, NULL);
  label = XtCreateManagedWidget((String) "aboutLabel1",
				labelWidgetClass, about,
				args, FOUR);
  XtSetArg(args[0], XtNlabel, "   <quinet@montefiore.ulg.ac.be>");
  XtSetArg(args[3], XtNfromVert, label);
  XtSetArg(args[4], XtNvertDistance, 0);
  label = XtCreateManagedWidget((String) "aboutLabel2",
				labelWidgetClass, about,
				args, FIVE);
  XtSetArg(args[0], XtNlabel, "Special thanks to :");
  XtSetArg(args[3], XtNfromVert, label);
  XtSetArg(args[4], XtNvertDistance, 10);
  label = XtCreateManagedWidget((String) "aboutLabel3",
				labelWidgetClass, about,
				args, FIVE);
  XtSetArg(args[0], XtNlabel, "Mark Boyns <boyns@hercules.sdsu.edu>");
  XtSetArg(args[3], XtNfromVert, label);
  XtSetArg(args[4], XtNvertDistance, 0);
  label = XtCreateManagedWidget((String) "aboutLabel4",
				labelWidgetClass, about,
				args, FIVE);
  XtSetArg(args[0], XtNlabel, "   for his rplay library");
  XtSetArg(args[3], XtNfromVert, label);
  label = XtCreateManagedWidget((String) "aboutLabel5",
				labelWidgetClass, about,
				args, FIVE);
  XtSetArg(args[0], XtNlabel, "Alain Nissen <nissen@montefiore.ulg.ac.be>");
  XtSetArg(args[3], XtNfromVert, label);
  label = XtCreateManagedWidget((String) "aboutLabel6",
				labelWidgetClass, about,
				args, FIVE);
  XtSetArg(args[0], XtNlabel, "   for his help and encouragements");
  XtSetArg(args[3], XtNfromVert, label);
  label = XtCreateManagedWidget((String) "aboutLabel7",
				labelWidgetClass, about,
				args, FIVE);
  callback[0].callback = selectCancelProc;
  callback[0].closure = (XtPointer) about;
  callback[1].callback = NULL;
  callback[1].closure = NULL;
  XtSetArg(args[0], XtNcallback, callback);
  XtSetArg(args[1], XtNlabel, "Done");
  XtSetArg(args[2], XtNfromHoriz, NULL);
  XtSetArg(args[3], XtNfromVert, label);
  button = XtCreateManagedWidget((String) "doneButton",
				 commandWidgetClass, about,
				 args, FOUR);
  XtPopup(popup, XtGrabExclusive);
  setBusy(True);
}


/*****************************************************************************/
#ifdef __STDC__
void stopProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void stopProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  XawListReturnStruct *item;

  item = XawListShowCurrent(spool_list);
  if (spool_count > 0)
    {
      if (item->list_index != XAW_LIST_NONE)
	rptp_stop_sound(item->list_index);
      else
	rptp_stop_sound(0);
    }
  else
    ringBell();
}


/*****************************************************************************/
#ifdef __STDC__
void pauseProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void pauseProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  XawListReturnStruct *item;

  item = XawListShowCurrent(spool_list);
  if (spool_count > 0)
    {
      if (item->list_index != XAW_LIST_NONE)
	rptp_pause_sound(item->list_index);
      else
	rptp_pause_sound(0);
    }
  else
    ringBell();
}


/*****************************************************************************/
#ifdef __STDC__
void continueProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void continueProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  XawListReturnStruct *item;

  item = XawListShowCurrent(spool_list);
  if (spool_count > 0)
    {
      if (item->list_index != XAW_LIST_NONE)
	rptp_continue_sound(item->list_index);
      else
	rptp_continue_sound(0);
    }
  else
    ringBell();
}


/*****************************************************************************/
#ifdef __STDC__
void addProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void addProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  XawListReturnStruct *item;

  item = XawListShowCurrent(sound_list);
  if ((item->list_index != XAW_LIST_NONE) && (sound_count > 0))
    add_item(item->string, &queue_names, &queue_count);
  else
    ringBell();
  XawListChange(queue_list, queue_names, queue_count, 0, True);
}


/*****************************************************************************/
#ifdef __STDC__
void addallProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void addallProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  int i;

  for (i = 0; i < sound_count; i++)
    add_item(sound_names[i], &queue_names, &queue_count);
  XawListChange(queue_list, queue_names, queue_count, 0, True);
}


/*****************************************************************************/
#ifdef __STDC__
void sfileMenuSelectProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void sfileMenuSelectProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  if (!strcmp(XtName(w), "serverList"))
    {
      setBusy(True);
      rptp_get_sound_list();
    }
  if (!strcmp(XtName(w), "loadList"))
    popupDialog(XtParent(XtParent(w)), (String) "Load list from file :",
		default_file, DIALOG_LOAD_SOUND_FILE);
  if (!strcmp(XtName(w), "saveList"))
    popupDialog(XtParent(XtParent(w)), (String) "Save list into file :",
		default_file, DIALOG_SAVE_SOUND_FILE);
}


/*****************************************************************************/
#ifdef __STDC__
void playProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void playProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  if (queue_count >= 1)
    {
      rptp_play_sound(queue_names[0]);
      delete_item(0, &queue_names, &queue_count);
      XawListChange(queue_list, queue_names, queue_count, 0, True);
    }
  else
    XBell(XtDisplay(w), 50);
}


/*****************************************************************************/
#ifdef __STDC__
void deleteProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void deleteProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  XawListReturnStruct *item;

  item = XawListShowCurrent(queue_list);
  if (item->list_index != XAW_LIST_NONE)
    delete_item(item->list_index, &queue_names, &queue_count);
  else
    if (queue_count >= 1)
      delete_item(queue_count - 1, &queue_names, &queue_count);
    else
      XBell(XtDisplay(w), 50);
  XawListChange(queue_list, queue_names, queue_count, 0, True);
}


/*****************************************************************************/
#ifdef __STDC__
void sortMenuSelectProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void sortMenuSelectProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  XawListReturnStruct *item;

  item = XawListShowCurrent(queue_list);
  if (!strcmp(XtName(w), "moveToTop"))
    {
      if (item->list_index != XAW_LIST_NONE)
	move_item(item->list_index, 0, &queue_names);
      else
	move_item(queue_count - 1, 0, &queue_names);
    }
  if (!strcmp(XtName(w), "moveToBottom"))
    {
      if (item->list_index != XAW_LIST_NONE)
	move_item(item->list_index, queue_count - 1, &queue_names);
      else
	move_item(0, queue_count - 1, &queue_names);
    }
  if (!strcmp(XtName(w), "sortByName"))
    {
      qsort((char *)queue_names, queue_count, sizeof(String), alphacompare);
    }
  if (!strcmp(XtName(w), "sortBySize"))
    {
      printf("*** Not implemented : Sort by size...\n");
    }
  if (!strcmp(XtName(w), "shuffle"))
    {
      printf("*** Not implemented : Shuffle...\n");
    }
  XawListChange(queue_list, queue_names, queue_count, 0, True);
}


/*****************************************************************************/
#ifdef __STDC__
void qfileMenuSelectProc(Widget w, XtPointer client_data, XtPointer call_data)
#else
void qfileMenuSelectProc(w, client_data, call_data)
     Widget    w;
     XtPointer client_data;
     XtPointer call_data;
#endif
{
  if (!strcmp(XtName(w), "directoryList"))
    popupDialog(XtParent(XtParent(w)), (String) "Build list from directory :", "",
		DIALOG_GET_SOUND_DIR);
  if (!strcmp(XtName(w), "loadList"))
    popupDialog(XtParent(XtParent(w)), (String) "Load list from file :",
		default_file, DIALOG_LOAD_QUEUE_FILE);
  if (!strcmp(XtName(w), "saveList"))
    popupDialog(XtParent(XtParent(w)), (String) "Save list into file :",
		default_file, DIALOG_SAVE_QUEUE_FILE);
}


/*****************************************************************************/
#ifdef __STDC__
void wmProtocolsProc(Widget w, XEvent *event, String params[], Cardinal *num_params)
#else
void wmProtocolsProc(w, event, params, num_params)
     Widget    w;
     XEvent   *event;
     String    params[];
     Cardinal *num_params;
#endif
{
  if (event -> xclient.data.l[0] == XA_WM_DELETE_WINDOW)
    {
      if ((w == top_shell) && ! closing_top_shell)
	{
	  closing_top_shell = True;
	  quitProc(w, (XtPointer) NULL, (XtPointer) NULL);
	  closing_top_shell = False;
	}
      if (w == jukebox_shell)
	{
	  resources.jukebox_shown = False;
	  XtPopdown(jukebox_shell);
	}
    }
}


/*****************************************************************************/
#ifdef __STDC__
void okProc(Widget w, XEvent *event, String params[], Cardinal *num_params)
#else
void okProc(w, event, params, num_params)
     Widget    w;
     XEvent   *event;
     String    params[];
     Cardinal *num_params;
#endif
{
  Widget dialog;

  dialog = XtParent(w);
  selectOkProc(w, (XtPointer) dialog, (XtPointer) NULL);
}


/*****************************************************************************/
#ifdef __STDC__
void cancelProc(Widget w, XEvent *event, String params[], Cardinal *num_params)
#else
void cancelProc(w, event, params, num_params)
     Widget    w;
     XEvent   *event;
     String    params[];
     Cardinal *num_params;
#endif
{
  Widget dialog;

  dialog = XtParent(w);
  selectCancelProc(w, (XtPointer) dialog, (XtPointer) NULL);
}


/*****************************************************************************/
#ifdef __STDC__
void create_widgets(Widget top_shell)
#else
void create_widgets(top_shell)
     Widget top_shell;
#endif
{
  Widget        sound_viewport, queue_viewport, spool_viewport;
  Widget        sound_label, queue_label, spool_label;
  Widget        spool_form, jukebox_paned, sound_form, queue_form;
  Widget        quit_button, jukebox_button, host_button, about_button;
  Widget        stop_button, pause_button, continue_button;
  Widget        add_button, addall_button, sfile_button;
  Widget        play_button, delete_button, sort_button, qfile_button;
  Widget        menu, menu_item;

  /* --- Main panel (rplay spool) ------------------------------------------ */

  spool_form = XtCreateManagedWidget((String) "spoolForm",
				     formWidgetClass, top_shell,
				     NULL, ZERO);

  quit_button = XtCreateManagedWidget((String) "quitButton",
                                      commandWidgetClass, spool_form,
                                      NULL, ZERO);
  XtAddCallback(quit_button, XtNcallback, quitProc, NULL);

  jukebox_button = XtCreateManagedWidget((String) "jukeboxButton",
					 commandWidgetClass, spool_form,
					 NULL, ZERO);
  XtAddCallback(jukebox_button, XtNcallback, jukeboxProc, NULL);

  host_button = XtCreateManagedWidget((String) "hostButton",
				      commandWidgetClass, spool_form,
                                      NULL, ZERO);
  XtAddCallback(host_button, XtNcallback, hostProc, NULL);

  about_button = XtCreateManagedWidget((String) "aboutButton",
				       commandWidgetClass, spool_form,
				       NULL, ZERO);
  XtAddCallback(about_button, XtNcallback, aboutProc, NULL);

  spool_label = XtCreateManagedWidget((String) "spoolLabel",
				      labelWidgetClass, spool_form,
				      NULL, ZERO);

  spool_viewport = XtCreateManagedWidget((String) "spoolViewport",
					 viewportWidgetClass, spool_form,
					 NULL, ZERO);

  spool_list = XtCreateManagedWidget((String) "spoolList",
                                     listWidgetClass, spool_viewport,
				     NULL, ZERO);
  XtAddCallback(spool_list, XtNcallback, select_spoolProc, NULL);
  XawListChange(spool_list, spool_names, spool_count, 0, True);

  stop_button = XtCreateManagedWidget((String) "stopButton",
				      commandWidgetClass, spool_form,
                                      NULL, ZERO);
  XtAddCallback(stop_button, XtNcallback, stopProc, NULL);

  pause_button = XtCreateManagedWidget((String) "pauseButton",
				       commandWidgetClass, spool_form,
				       NULL, ZERO);
  XtAddCallback(pause_button, XtNcallback, pauseProc, NULL);

  continue_button = XtCreateManagedWidget((String) "continueButton",
					  commandWidgetClass, spool_form,
					  NULL, ZERO);
  XtAddCallback(continue_button, XtNcallback, continueProc, NULL);

  /* --- Jukebox panel ----------------------------------------------------- */

  jukebox_shell = XtCreatePopupShell((String) "jukebox",
				     topLevelShellWidgetClass, top_shell,
				     NULL, ZERO);

  jukebox_paned = XtCreateManagedWidget((String) "jukeboxPaned",
					panedWidgetClass, jukebox_shell,
					NULL, ZERO);

  sound_form = XtCreateManagedWidget((String) "soundForm",
				     formWidgetClass, jukebox_paned,
				     NULL, ZERO);

  sound_label = XtCreateManagedWidget((String) "soundLabel",
				      labelWidgetClass, sound_form,
				      NULL, ZERO);

  sound_viewport = XtCreateManagedWidget((String) "soundViewport",
					 viewportWidgetClass, sound_form,
					 NULL, ZERO);

  sound_list = XtCreateManagedWidget((String) "soundList",
                                     listWidgetClass, sound_viewport,
				     NULL, ZERO);
  XtAddCallback(sound_list, XtNcallback, select_soundProc, NULL);
  XawListChange(sound_list, sound_names, sound_count, 0, True);

  add_button = XtCreateManagedWidget((String) "addButton",
				      commandWidgetClass, sound_form,
                                      NULL, ZERO);
  XtAddCallback(add_button, XtNcallback, addProc, NULL);

  addall_button = XtCreateManagedWidget((String) "addAllButton",
				      commandWidgetClass, sound_form,
                                      NULL, ZERO);
  XtAddCallback(addall_button, XtNcallback, addallProc, NULL);

  sfile_button = XtCreateManagedWidget((String) "sfileMenuButton",
				       menuButtonWidgetClass, sound_form,
				       NULL, ZERO);


  queue_form = XtCreateManagedWidget((String) "queueForm",
				     formWidgetClass, jukebox_paned,
				     NULL, ZERO);

  queue_label = XtCreateManagedWidget((String) "queueLabel",
				      labelWidgetClass, queue_form,
				      NULL, ZERO);

  queue_viewport = XtCreateManagedWidget((String) "queueViewport",
					 viewportWidgetClass, queue_form,
					 NULL, ZERO);

  queue_list = XtCreateManagedWidget((String) "queueList",
                                     listWidgetClass, queue_viewport,
				     NULL, ZERO);

  XtAddCallback(queue_list, XtNcallback, select_queueProc, NULL);
  XawListChange(queue_list, queue_names, queue_count, 0, True);

  play_button = XtCreateManagedWidget((String) "playButton",
				      commandWidgetClass, queue_form,
                                      NULL, ZERO);
  XtAddCallback(play_button, XtNcallback, playProc, NULL);

  delete_button = XtCreateManagedWidget((String) "deleteButton",
				      commandWidgetClass, queue_form,
                                      NULL, ZERO);
  XtAddCallback(delete_button, XtNcallback, deleteProc, NULL);

  sort_button = XtCreateManagedWidget((String) "sortMenuButton",
				      menuButtonWidgetClass, queue_form,
                                      NULL, ZERO);

  qfile_button = XtCreateManagedWidget((String) "qfileMenuButton",
				      menuButtonWidgetClass, queue_form,
                                      NULL, ZERO);

  /* --- Menus ------------------------------------------------------------- */

  menu = XtCreatePopupShell((String) "menu",
			    simpleMenuWidgetClass, sfile_button,
			    NULL, ZERO);

  menu_item = XtCreateManagedWidget((String) "serverList",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, sfileMenuSelectProc, NULL);

  menu_item = XtCreateManagedWidget((String) "loadList",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, sfileMenuSelectProc, NULL);

  menu_item = XtCreateManagedWidget((String) "saveList",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, sfileMenuSelectProc, NULL);

  menu = XtCreatePopupShell((String) "menu",
			    simpleMenuWidgetClass, sort_button,
			    NULL, ZERO);

  menu_item = XtCreateManagedWidget((String) "moveToTop",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, sortMenuSelectProc, NULL);

  menu_item = XtCreateManagedWidget((String) "moveToBottom",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, sortMenuSelectProc, NULL);

  menu_item = XtCreateManagedWidget((String) "sortByName",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, sortMenuSelectProc, NULL);

/*
  menu_item = XtCreateManagedWidget((String) "sortBySize",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, sortMenuSelectProc, NULL);

  menu_item = XtCreateManagedWidget((String) "shuffle",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, sortMenuSelectProc, NULL);
*/

  menu = XtCreatePopupShell((String) "menu",
			    simpleMenuWidgetClass, qfile_button,
			    NULL, ZERO);

/*
  menu_item = XtCreateManagedWidget((String) "directoryList",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, qfileMenuSelectProc, NULL);
*/

  menu_item = XtCreateManagedWidget((String) "loadList",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, qfileMenuSelectProc, NULL);

  menu_item = XtCreateManagedWidget((String) "saveList",
				    smeBSBObjectClass, menu,
				    NULL, ZERO);
  XtAddCallback(menu_item, XtNcallback, qfileMenuSelectProc, NULL);
}


/*****************************************************************************/
#ifdef __STDC__
void init_application(char *app_name, int argc, char *argv[])
#else
void init_application(app_name, argc, argv)
     char *app_name;
     int   argc;
     char *argv[];
#endif
{
  Display *disp;

  top_shell = XtAppInitialize(&app_context, app_name,
                              options, XtNumber(options),
			      &argc, argv,
                              fallback_resources, NULL, ZERO);
  disp = XtDisplay(top_shell);
  XA_WM_DELETE_WINDOW = XInternAtom(disp, "WM_DELETE_WINDOW", False);
  XtGetApplicationResources(top_shell, (XtPointer) &resources,
                            resources_desc, XtNumber(resources_desc),
			    NULL, ZERO);
  XtAppAddActions(app_context, actions, XtNumber(actions));
  /*
    if (argc != 1)
    error(SYNTAX_ERROR);
  */
  create_widgets(top_shell);
  XtRealizeWidget(top_shell);
  XSetWMProtocols(disp, XtWindow(top_shell), &XA_WM_DELETE_WINDOW, 1);
  XtRealizeWidget(jukebox_shell);
  XSetWMProtocols(disp, XtWindow(jukebox_shell), &XA_WM_DELETE_WINDOW, 1);
  if (resources.jukebox_shown)
    XtPopup(jukebox_shell, XtGrabNone);
}
