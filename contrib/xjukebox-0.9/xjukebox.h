#include <stdio.h>
#include <string.h>
#include <rplay.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#define XJUKEBOX_VERSION        "0.9"

#define MODE_IDLE               0
#define MODE_READING_SOUNDS     1
#define MODE_READING_SPOOL      2
#define MODE_DISCONNECTED       3

#define DIALOG_NONE             0
#define DIALOG_SET_NEW_HOST     1
#define DIALOG_LOAD_SOUND_FILE  2
#define DIALOG_SAVE_SOUND_FILE  3
#define DIALOG_GET_SOUND_DIR    4
#define DIALOG_LOAD_QUEUE_FILE  5
#define DIALOG_SAVE_QUEUE_FILE  6

#define NO_INTERVAL ((XtIntervalId) -1)

#define SPOOL_UNKNOWN   0
#define SPOOL_PLAY      1
#define SPOOL_PAUSE     2
#define SPOOL_WAIT      3

typedef struct
  {
    int          sid;        /* Sound id (given by rplayd) */
    char         host[16];   /* Where did the play command come from ? */
    int          state;      /* SPOOL_PLAY, SPOOL_PAUSE, SPOOL_WAIT, ... */
    int          vol;        /* Volume */
    int          pri;        /* Priority */
    int          count;      /* Number of sounds */
    int          seconds;    /* Duration */
    int          remain;     /* Remaining time */
    char        *sound;      /* File name */
  }
  spool_info;

typedef struct
  {
    char        *sound;      /* File name */
    int          size;       /* Size (given by "find ...") */
    int          idx;        /* Used for shuffling the sounds */
  }
  queue_info;

struct _resources_rec  /* Most of these are not used yet.  :-) */
  {
    Boolean   jukebox_shown;
    Boolean   update_spool;
    int       refresh_rate;
    Boolean   show_hostname;
    Boolean   show_volume;
  }
  resources;
