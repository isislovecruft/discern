/* File: graph.c
 * 
 * X interface for DISCERN
 * 
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: graph.c,v 1.68 1994/09/22 05:06:52 risto Exp risto $
 */

#include <stdio.h>
#include <math.h>
#include <setjmp.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/AsciiText.h>

#include "defs.h"
#include "Gwin.h"
#include "globals.c"


/************ graphics constants *************/

/* color parameters */
#define UNITCOLORS 0		/* chooses bryw colorscale */
#define WEIGHTCOLORS 1		/* chooses gbryw colorscale (not used) */
#define MINCOLORS 10		/* if fewer free colors, use existing */
#define MAXCOLORS 256		/* max # of colors */
#define MAXCOLGRAN 65535	/* granularity of color values */

/* colormap entry */
typedef struct RGB
  {
    short red, green, blue;	/* hold possible rgb values in XColor struct */
  }
RGB;

/* graphics separator constants */
#define HIDSP 1			/* separation of hidden layers,pixels */
#define VERSP 3			/* generic vertical separation */
#define HORSP 3			/* generic horizontal separation */
#define BOXSP 1			/* vertical space around boxed text */
#define HFMSP 3			/* separation of hfm boxes */
#define PREVSP (3 * HORSP)	/* indentation of prev hidden layers */

/* displaying labels */
#define MORELABEL "more.."	/* if # of labels > maxlabels, this is shown */

#define NWINS NMODULES		/* number of windows */

/* text constants in hfminp */
#define HFMINP_REPS "slot-representations" /* input data begins in hfminp */


/******************** Function prototypes ************************/

/* global functions */
#include "prototypes.h"
extern unsigned sleep __P ((unsigned seconds));

/* functions local to this file */
static void handle_events __P((void));
static void runstop_callback __P((Widget w, XtPointer client_data,
				  XtPointer call_data));
static void toggle_callback __P((Widget w, XtPointer client_data,
				 XtPointer call_data));
static void Read_command __P((Widget w, XEvent *ev, String *params,
			      Cardinal *num_params));
static void clear_callback __P((Widget w, XtPointer client_data,
				XtPointer call_data));
static void quit_callback __P((Widget w, XtPointer client_data,
			       XtPointer call_data));
static void init_rgbtab_noweights __P((void));
static void init_rgbtab_bw __P((void));
static void alloc_col __P((void));
static void clean_color_map __P((int k));
static void create_colormap __P((void));
static int createGC __P((Window New_win, GC *New_GC, Font fid, Pixel FGpix,
			 Pixel theBGpix));
static XFontStruct *loadFont __P((char fontName[]));
static void common_resize __P((int modi, Widget w));
static void init_common_proc_display_params __P((int modi, Widget w));
static void expose_sentpars __P((Widget w, XtPointer client_data,
				 XtPointer call_data));
static void expose_storypars __P((Widget w, XtPointer client_data,
				  XtPointer call_data));
static void expose_sentgen __P((Widget w, XtPointer client_data,
				XtPointer call_data));
static void expose_storygen __P((Widget w, XtPointer client_data,
				 XtPointer call_data));
static void expose_cueformer __P((Widget w, XtPointer client_data,
				  XtPointer call_data));
static void expose_answerprod __P((Widget w, XtPointer client_data,
				   XtPointer call_data));
static void common_proc_resize __P((int modi));
static void resize_sentpars __P((Widget w, XtPointer client_data,
			     XtPointer call_data));
static void resize_storypars __P((Widget w, XtPointer client_data,
			      XtPointer call_data));
static void resize_storygen __P((Widget w, XtPointer client_data,
			     XtPointer call_data));
static void resize_sentgen __P((Widget w, XtPointer client_data,
			    XtPointer call_data));
static void resize_cueformer __P((Widget w, XtPointer client_data,
			      XtPointer call_data));
static void resize_answerprod __P((Widget w, XtPointer client_data,
			       XtPointer call_data));
static void display_labels __P((int modi, int x, int y,
				char *labels[], int nitem));
static void display_boxword __P((int modi, int x, int y, int width, int height,
				 char wordchars[], int index,
				 XFontStruct *fontStruct, GC currGC));
static int imin __P((int a, int b));
static int imax __P((int a, int b));
static void init_hfm_display_params __P((int modi, Widget w));
static void expose_hfm __P((Widget w, XtPointer client_data,
			    XtPointer call_data));
static void resize_hfm __P((Widget w, XtPointer client_data,
			XtPointer call_data));
static void hfmmouse_handler __P ((Widget w, XtPointer client_data,
				   XEvent *p_event));
static void topmouse_handler __P ((int x, int y));
static void midmouse_handler __P ((int x, int y));
static void botmouse_handler __P ((int x, int y));
static int compressed_lines __P ((int indices[], int nindices,
				  int low, int high));
static void display_top_box __P((int modi, int nnet, int x, int y, int width,
				 int height, XFontStruct *fontstruct,
				 FMUNIT units[MAXTOPNET][MAXTOPNET]));
static void display_mid_box __P((int modi, int nnet, int x, int y, int width,
				 int height, XFontStruct *fontstruct,
				 FMUNIT units[MAXMIDNET][MAXMIDNET]));
static void display_bot_box __P((int modi, int nnet, int x, int y, int width,
				 int height, XFontStruct *fontstruct,
				 FMUNIT units[MAXBOTNET][MAXBOTNET],
				 double lweights[MAXBOTNET][MAXBOTNET]
				               [MAXBOTNET][MAXBOTNET]));
static void init_lex_display_params __P((int modi, Widget w, int nnet,
					 int nwords, WORDSTRUCT words[],
					 int nrep,
					 FMUNIT units[MAXLSNET][MAXLSNET]));
static void expose_lex __P((Widget w, XtPointer units, XtPointer call_data));
static void resize_lex __P((Widget w, XtPointer client_data,
			    XtPointer call_data));
static void lexsemmouse_handler __P ((Widget w, XtPointer client_data,
				      XEvent *p_event));
static void display_assocweights __P ((int srcmodi,
				       FMUNIT srcunits[MAXLSNET][MAXLSNET],
				       int nsrcnet, WORDSTRUCT *srcwords,
				       int nsrcrep, int nsrcwords, int tgtmodi,
				       FMUNIT tgtunits[MAXLSNET][MAXLSNET],
				       int ntgtnet, WORDSTRUCT *tgtwords,
				       int ntgtrep, int ntgtwords,
				       int x, int y,
				       double assoc[MAXLSNET][MAXLSNET]
				                   [MAXLSNET][MAXLSNET]));
static void display_title __P((int modi, char name[]));
static void display_log __P((int modi));
static void frameRectangle __P((int modi, int x, int y, int width, int height,
				int colorindex));
static void labelbox __P((int modi, int x, int y, int width, int height,
			  double value, char labels[][MAXWORDL + 1], 
			  int labelcount, XFontStruct *fontstruct,
			  GC fGC, GC bGC));
static void collect_uniq_labels __P((char labels[][MAXWORDL + 1],
				     int *count, char label[], int maxlabels));
static void collect_labels __P((char labels[][MAXWORDL + 1], int *count,
				char label[], int maxlabels));
static int newlabel __P((char labels[][MAXWORDL + 1], int count, char *label));
static int trans_to_color __P((double value, int map));
static void drawLine __P((int modi, int x1, int y1, int x2, int y2, GC currGC,
			  int width));
static void clearRectangle __P((int modi, int x, int y, int width,
				int height));
static void fillRectangle __P((int modi, int x, int y, int width, int height,
			       int colorindex));
static void drawRectangle __P((int modi, int x, int y, int width, int height));
static void drawText __P((int modi, int x, int y, char text[], GC currGC));
static void drawoverText __P((int modi, int x, int y, char text[], GC currGC));


/******************** static variables *********************/

static XColor colors[MAXCOLORS];
static int actual_color_range;	/* number of colors in use */
static int fewesttopi, fewesttopj; /* coordinates for the hfm label */

/* button, command, and graphics widgets */
static Widget
  runstop, step, clear, quit,	/* buttons */
  command,			/* command input widget */
  sentpars, storypars, storygen, sentgen, cueformer,
  answerprod, hfm, lex, sem;	/* network graphics widgets */

/* useful pointers to windows */
static Window
  theMain, runstopWin, commandWin, Win[NWINS];	/* network graphics windows */

/* text constants to be displayed */
static char
  /* labels of the case role assemblies */
 *caselabels[] =
{"AGENT", "ACT", "RECIPIENT", "PAT-ATTR", "PATIENT", "LOCATION"},
  /* labels of the script slot assemblies */
 *slotlabels[] =
{"SCRIPT", "TRACK", "ACTOR", "ITEM", "PLACE", "ROLE1", "ROLE2"},	
  /* names of the network modules */
 *titles[] =
{"SENTENCE PARSER", "STORY PARSER", "STORY GENERATOR", "SENTENCE GENERATOR",
 "CUE FORMER", "ANSWER PRODUCER", "EPISODIC MEMORY", "unused", "LEXICAL MAP",
 "unused", "SEMANTIC MAP", "unused"};	/* input and output lexical maps */
					/* are combined into one window */
					/* so their indices are unused */

/* graphics contexts */
static GC
  titleGC,			/* window name */
  logGC,			/* log line: item and error */
  asmGC,			/* text in assemblies */
  asmerrorGC,			/* errors in assemblies (smaller font) */
  hfmfGC, tracefGC, lexfGC,
  semfGC,			/* foreground labels on maps */
  hfmbGC, tracebGC, lexbGC,
  sembGC,			/* background (reverse) labels */
  linefGC, linebGC,		/* trace lines, foreground and back */
  clearGC,			/* for clearing areas */
  boxGC,			/* for drawing network boxes */
  activityGC;			/* for displaying activations */

/* corresponding fonts */
static XFontStruct
 *titlefontStruct, *logfontStruct, *asmfontStruct, *asmerrorfontStruct,
 *hfmfontStruct, *tracefontStruct, *lexfontStruct, *semfontStruct;

/* Array of rgb values that represents a linear color spectrum */
static RGB rgbtab[MAXCOLORS];
/* the unit value colormap */
static short cmap[MAXCOLORS];

/* actions */
static XtActionsRec command_actions[] =
{				/* need to read the command line  */
  { "read_command", Read_command }	/* after user presses return */
};				/* so define it as an action */


/********************* general initialization ***********************/

void
display_init ()
/* initialize the X display */
{
  Arg args[3];
  char s[MAXSTRL + 1];
  Pixel theBGpix;		/* background color */
  Dimension borderwidth,
    height, width,		/* various widget heights and widths given */
    tot_width;			/* computed total width of display */

  printf ("Initializing graphics...\n");

  XtSetArg (args[0], XtNborderWidth, &borderwidth);
  XtGetValues (main_widget, args, 1);	/* get the current border width  */
  /* create a form with no space between widgets */
  XtSetArg (args[0], XtNdefaultDistance, 0);
  form = XtCreateManagedWidget ("form", formWidgetClass, main_widget, args, 1);

  /* the command button for running and stopping the simulation */
  runstop = XtCreateManagedWidget ("runstop", commandWidgetClass,
				   form, args, 1);
  /* toggle switch: if on, the simulation will stop after each propagation */
  step = XtCreateManagedWidget ("step", toggleWidgetClass, form, args, 1);
  /* the command button for clearing the networks */
  clear = XtCreateManagedWidget ("clear", commandWidgetClass, form, args, 1);
  /* the command button for exiting the program */
  quit = XtCreateManagedWidget ("quit", commandWidgetClass, form, args, 1);

  /* create command line widget: */
  /* first get the height from runstop */
  XtSetArg (args[0], XtNheight, &height);
  /* then figure out the total width of the display by */
  /* getting the widths of each widget, adding them up, and subtracting
     from the total width of the display */
  XtSetArg (args[1], XtNwidth, &width);
  XtGetValues (runstop, args, 2);
  tot_width = width + 2 * borderwidth;
  XtSetArg (args[0], XtNwidth, &width);
  XtGetValues (step, args, 1);
  tot_width += width + 2 * borderwidth;
  XtSetArg (args[0], XtNwidth, &width);
  XtGetValues (clear, args, 1);
  tot_width += width + 2 * borderwidth;
  XtSetArg (args[0], XtNwidth, &width);
  XtGetValues (quit, args, 1);
  tot_width += width + 2 * borderwidth;
  XtSetArg (args[0], XtNheight, height);
  XtSetArg (args[1], XtNwidth,
	    2 * data.netwidth + 2 * borderwidth - tot_width);
  /* display the name of the current inputfile in the command line widget */
  sprintf (s, "file %s", current_inpfile);
  XtSetArg (args[2], XtNstring, s);
  command = XtCreateManagedWidget ("command", asciiTextWidgetClass,
				   form, args, 3);

  /* create the proc network displays */
  /* get the size from resourses */
  XtSetArg (args[0], XtNwidth, data.netwidth);
  XtSetArg (args[1], XtNheight, data.procnetheight);
  sentpars = XtCreateManagedWidget ("sentpars", gwinWidgetClass,
				    form, args, 2);
  storypars = XtCreateManagedWidget ("storypars", gwinWidgetClass,
				     form, args, 2);
  cueformer = XtCreateManagedWidget ("cueformer", gwinWidgetClass,
				     form, args, 2);
  sentgen = XtCreateManagedWidget ("sentgen", gwinWidgetClass,
				   form, args, 2);
  storygen = XtCreateManagedWidget ("storygen", gwinWidgetClass,
				    form, args, 2);
  answerprod = XtCreateManagedWidget ("answerprod", gwinWidgetClass,
				      form, args, 2);

  /* create the hfm and lex displays */
  /* take the height from resources; width is the same for all */
  XtSetArg (args[1], XtNheight, data.lexnetheight);
  lex = XtCreateManagedWidget ("lex", gwinWidgetClass, form, args, 2);
  sem = XtCreateManagedWidget ("sem", gwinWidgetClass, form, args, 2);
  XtSetArg (args[1], XtNheight, data.hfmnetheight);
  hfm = XtCreateManagedWidget ("hfm", gwinWidgetClass, form, args, 2);

  /* callbacks: what to do when a button is pressed */
  XtAddCallback (runstop, XtNcallback, runstop_callback, NULL);
  XtAddCallback (step, XtNcallback, toggle_callback, NULL);
  XtAddCallback (clear, XtNcallback, clear_callback, NULL);
  XtAddCallback (quit, XtNcallback, quit_callback, NULL);

  /* network callbacks: redrawing the state */
  XtAddCallback (sentpars, XtNexposeCallback, expose_sentpars, NULL);
  XtAddCallback (storypars, XtNexposeCallback, expose_storypars, NULL);
  XtAddCallback (cueformer, XtNexposeCallback, expose_cueformer, NULL);
  XtAddCallback (sentgen, XtNexposeCallback, expose_sentgen, NULL);
  XtAddCallback (storygen, XtNexposeCallback, expose_storygen, NULL);
  XtAddCallback (answerprod, XtNexposeCallback, expose_answerprod, NULL);
  XtAddCallback (hfm, XtNexposeCallback, expose_hfm, NULL);
  XtAddCallback (lex, XtNexposeCallback, expose_lex, lunits);
  XtAddCallback (sem, XtNexposeCallback, expose_lex, sunits);

  /* network callbacks for resizing */
  XtAddCallback (sentpars, XtNresizeCallback, resize_sentpars, NULL);
  XtAddCallback (storypars, XtNresizeCallback, resize_storypars, NULL);
  XtAddCallback (cueformer, XtNresizeCallback, resize_cueformer, NULL);
  XtAddCallback (sentgen, XtNresizeCallback, resize_sentgen, NULL);
  XtAddCallback (storygen, XtNresizeCallback, resize_storygen, NULL);
  XtAddCallback (answerprod, XtNresizeCallback, resize_answerprod, NULL);
  XtAddCallback (hfm, XtNresizeCallback, resize_hfm, NULL);
  XtAddCallback (lex, XtNresizeCallback, resize_lex, NULL);
  XtAddCallback (sem, XtNresizeCallback, resize_lex, NULL);

  /* add the command reading action in the table */
  XtAppAddActions (app_con, command_actions, XtNumber (command_actions));

  /* network event handlers for mouse clicks */
  XtAddEventHandler (hfm, ButtonPressMask, FALSE,
		     (XtEventHandler) hfmmouse_handler, NULL);
  XtAddEventHandler (lex, ButtonPressMask, FALSE,
		     (XtEventHandler) lexsemmouse_handler, NULL);
  XtAddEventHandler (sem, ButtonPressMask, FALSE,
		     (XtEventHandler) lexsemmouse_handler, NULL);

  /* figure out the display type and allocate colors */
  create_colormap ();
  /* put the display on screen */
  XtRealizeWidget (main_widget);
  theMain = XtWindow (main_widget);
  runstopWin = XtWindow (runstop);
  commandWin = XtWindow (command);
  /* all keyboard events should go to the command line window */
  XtSetKeyboardFocus (main_widget, command);

  /* calculate some network geometry variables */
  init_common_proc_display_params (SENTPARSMOD, sentpars);
  init_common_proc_display_params (STORYPARSMOD, storypars);
  init_common_proc_display_params (STORYGENMOD, storygen);
  init_common_proc_display_params (SENTGENMOD, sentgen);
  init_common_proc_display_params (CUEFORMMOD, cueformer);
  init_common_proc_display_params (ANSWERPRODMOD, answerprod);
  /* and label the feature map units */
  init_hfm_display_params (HFMWINMOD, hfm);
  init_lex_display_params (LEXWINMOD, lex, nlnet, nlwords,
			   lwords, nlrep, lunits);
  init_lex_display_params (SEMWINMOD, sem, nsnet, nlwords,
			   swords, nsrep, sunits);
  /* use nlwords even on sem map so that internal symbols are not mapped */

  /* set a common font for all buttons and command line */
  XtSetArg (args[0], XtNfont, loadFont (data.commandfont));
  XtSetValues (runstop, args, 1);
  XtSetValues (step, args, 1);
  XtSetValues (clear, args, 1);
  XtSetValues (quit, args, 1);
  XtSetValues (command, args, 1);

  /* load the other fonts */
  titlefontStruct = loadFont (data.titlefont);
  logfontStruct = loadFont (data.logfont);
  asmfontStruct = loadFont (data.asmfont);
  asmerrorfontStruct = loadFont (data.asmerrorfont);
  hfmfontStruct = loadFont (data.hfmfont);
  tracefontStruct = loadFont (data.tracefont);
  lexfontStruct = loadFont (data.lexfont);
  semfontStruct = loadFont (data.semfont);

  /* figure out space needed for the title and labels */
  titleboxhght = titlefontStruct->ascent + titlefontStruct->descent
    + 2 * BOXSP;
  asmboxhght = asmfontStruct->ascent + asmfontStruct->descent + 2 * BOXSP;

  /* get the background color */
  XtSetArg (args[0], XtNbackground, &theBGpix);
  XtGetValues (main_widget, args, 1);

  /* create graphics context for all fonts */
  createGC (theMain, &titleGC, titlefontStruct->fid, data.textColor, theBGpix);
  createGC (theMain, &logGC, logfontStruct->fid, data.textColor, theBGpix);
  createGC (theMain, &asmGC, asmfontStruct->fid, data.textColor, theBGpix);
  createGC (theMain, &asmerrorGC, asmerrorfontStruct->fid, data.textColor,
	    theBGpix);

  /* these are foreground colors */
  createGC (theMain, &hfmfGC, hfmfontStruct->fid, data.textColor, theBGpix);
  createGC (theMain, &tracefGC, tracefontStruct->fid, data.textColor,theBGpix);
  createGC (theMain, &lexfGC, lexfontStruct->fid, data.textColor, theBGpix);
  createGC (theMain, &semfGC, semfontStruct->fid, data.textColor, theBGpix);

  /* and these are background (when the label color needs to be reversed) */
  createGC (theMain, &hfmbGC, hfmfontStruct->fid, theBGpix, theBGpix);
  createGC (theMain, &tracebGC, tracefontStruct->fid, theBGpix, theBGpix);
  createGC (theMain, &lexbGC, lexfontStruct->fid, theBGpix, theBGpix);
  createGC (theMain, &sembGC, semfontStruct->fid, theBGpix, theBGpix);

  /* foreground and background colors for trace map weights */
  createGC (theMain, &linefGC, logfontStruct->fid, data.latweightColor,
	    theBGpix);
  createGC (theMain, &linebGC, logfontStruct->fid, theBGpix, theBGpix);
  /* clearing areas */
  createGC (theMain, &clearGC, logfontStruct->fid, theBGpix, theBGpix);
  /* network boxes */
  createGC (theMain, &boxGC, logfontStruct->fid, data.netColor, theBGpix);
  /* generic context for displaying unit activity */
  createGC (theMain, &activityGC, logfontStruct->fid, theBGpix, theBGpix);

  /* calculate all network geometries and put them on screen */
  resize_sentpars (sentpars, NULL, NULL);
  resize_storypars (storypars, NULL, NULL);
  resize_storygen (storygen, NULL, NULL);
  resize_sentgen (sentgen, NULL, NULL);
  resize_cueformer (cueformer, NULL, NULL);
  resize_answerprod (answerprod, NULL, NULL);
  resize_hfm (hfm, NULL, NULL);
  resize_lex (lex, NULL, NULL);
  resize_lex (sem, NULL, NULL);

  printf ("Graphics initialization complete.\n");
}


/*********************  event handler  ***********************/

static void
handle_events ()
  /* event handling loop */
  /* we need this instead of XtMainLoop because the main task is to run
     the simulation, and only occasionally check for events */
{
  XEvent theEvent;

  while (XtAppPending (app_con))
  /* as long as there are unprocessed events */
    {
      XtAppNextEvent (app_con, &theEvent);
      if (!(theEvent.type == Expose && theEvent.xexpose.count > 0))
	/* only process the last expose event */
	{
	  XtDispatchEvent (&theEvent);
	  XFlush (theDisplay);
	}
    }
}


void
wait_and_handle_events ()
/* this is called after each main step of the simulation
   to stop and wait for "run" if stepping, and to deal with
   any other possible events before displaying simulation results */
{
  if (stepping)
    wait_for_run ();		/* wait until user is ready */
  else
    /* slow down the display this many seconds */
    sleep ((unsigned) abs (data.delay));
  handle_events ();		/* process all pending events before proceeding
				   to display the simulation results */
}


/*********************  button callback routines  ***********************/

static void
runstop_callback (w, client_data, call_data)
  /* "Run/Stop" button press handler:
     if simulation is running, stop it; otherwise, continue simulation.
     from wherever handle_events was called */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  if (simulator_running)
    wait_for_run ();
  else
    start_running ();
}


void
wait_for_run ()
  /* stop the simulation and start processing events */
{
  Arg args[1];

  simulator_running = FALSE;        	/* process events */
  XtSetArg (args[0], XtNlabel, "Run");	/* change label on "Run/Stop" button */
  XtSetValues (runstop, args, 1);
  XFlush (theDisplay);
  while (!simulator_running)		/* process events until Run pressed */
    {
      /* Process one event if any or block */
      XtAppProcessEvent (app_con, XtIMAll);
      /* Process more events, but don't block */
      handle_events ();
    }
}


void
start_running ()
  /* set up to continue or start simulation */
{
  Arg args[1];

  simulator_running = TRUE;		/* allow exit from wait_for_run */
  XtSetArg (args[0], XtNlabel, "Stop");	/* change label on "Run/Stop" button */
  XtSetValues (runstop, args, 1);
}


static void
toggle_callback (w, client_data, call_data)
  /* "Step" button press handler: turn stepping on or off */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  stepping = !stepping;
}


static void
Read_command (w, ev, params, num_params)
  /* this action is executed after the user presses return:
     read a command from the command widget and process it */
  /* standard action parameters, not used here */
     Widget w; XEvent *ev; String *params; Cardinal *num_params;
{
  Arg args[1];
  String str = NULL;			/* command string */
  char commandstr[MAXSTRL + 1],		/* command and */
    rest[MAXSTRL + 1];			/*  its parameters */

  /* get the command line from screen */
  XtSetArg (args[0], XtNstring, &str);
  XtGetValues (command, args, 1);

  if (strlen (str))
    {
      /* chop it into command and params */
      printf ("%s\n", str);
      sscanf (str, "%s", commandstr);
      strcpy (rest, str + strlen (commandstr));

      /* clear the command widget */
      XtSetArg (args[0], XtNstring, "");
      XtSetValues (command, args, 1);

      process_command (NULL, commandstr, rest);
    }
  else
    printf ("\n%s", promptstr);
}


static void
clear_callback (w, client_data, call_data)
  /* "Clear" button press handler: clear the display and network activations,
     start a fresh command processing loop */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  extern jmp_buf loop_stories_env;

  printcomment ("\n", "Clearing networks", "\n");
  clear_networks_display ();
  nohfmmouseevents = FALSE;		/* allow mouse events again */
  nolexmouseevents = FALSE;
  longjmp (loop_stories_env, 0);
}


void
clear_networks_display ()
  /* clear the network activations, traces, logs, and the display */
{
  int modi;

  /* clear the processing modules and their display windows */
  for (modi = 0; modi < NPROCMODULES; modi++)
    {
      proc_clear_network (modi);
      XClearArea (theDisplay, Win[modi], 0, 0, 0, 0, True);
    }

  /* clear episodic memory and its display */
  hfm_clear_values ();
  hfm_clear_prevvalues ();
  clear_traces ();
  sprintf (net[HFMWINMOD].log, "%s", "");
  XClearArea (theDisplay, Win[HFMWINMOD], 0, 0, 0, 0, True);

  /* clear the lexical map and its display */
  lex_clear_values (lunits, nlnet);
  lex_clear_prevvalues (lunits, nlnet);
  sprintf (net[LEXWINMOD].log, "%s", "");
  XClearArea (theDisplay, Win[LEXWINMOD], 0, 0, 0, 0, True);

  /* clear the semantic map and its display */
  lex_clear_values (sunits, nsnet);
  lex_clear_prevvalues (sunits, nsnet);
  sprintf (net[SEMWINMOD].log, "%s", "");
  XClearArea (theDisplay, Win[SEMWINMOD], 0, 0, 0, 0, True);

  /* update the output immediately before data changes */
  XFlush (theDisplay);
  handle_events ();
}


static void
quit_callback (w, client_data, call_data)
  /* "Quit" button press handler: exit the program */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  close_display ();
  exit (EXIT_NORMAL);
}


void
close_display ()
  /* free fonts and close the display */
{
  XFreeFont (theDisplay, titlefontStruct);
  XFreeFont (theDisplay, logfontStruct);
  XFreeFont (theDisplay, asmfontStruct);
  XFreeFont (theDisplay, asmerrorfontStruct);
  XFreeFont (theDisplay, hfmfontStruct);
  XFreeFont (theDisplay, tracefontStruct);
  XFreeFont (theDisplay, lexfontStruct);
  XFreeFont (theDisplay, semfontStruct);
  XCloseDisplay (theDisplay);
}


/*********************  colormap allocation  ***********************/

static void
create_colormap ()
  /* allocate colors for activation values: depending on the display type
     and available colors, allocate a continuous spectrum the best you can */
{
  if (!(visual->class == GrayScale || visual->class == StaticGray))
    /* we have color display */
    init_rgbtab_noweights ();
  else
    /* black and white screen */
    init_rgbtab_bw ();
  alloc_col ();
}


static void
init_rgbtab_noweights ()
/* calculates values for the linear color spectrum black-red-yellow-white
   and stores them in rbgtab */
{
  double multiplier;
  int rangeby3, i;

  /* divide the range into three sections:
     black-red, red-yellow, yellow-white */
  rangeby3 = MAXCOLORS / 3;
  multiplier = ((double) MAXCOLGRAN) / rangeby3;
  for (i = 0; i < MAXCOLORS; i++)
    {
      rgbtab[i].green = 0;
      rgbtab[i].blue = 0;
      if (i < rangeby3)
	rgbtab[i].red = (int) (i * multiplier);
      else
	/* second section: red to yellow */
	{
	  rgbtab[i].red = MAXCOLGRAN;
	  if (i < 2 * rangeby3)
	    rgbtab[i].green = (int) ((i - rangeby3) * multiplier);
	  else
	    /* third section: yellow to white */
	    {
	      rgbtab[i].green = MAXCOLGRAN;
	      rgbtab[i].blue = (int) ((i - 2 * rangeby3) * multiplier);
	    }
	}
    }
}


static void
init_rgbtab_bw ()
/* calculates values for the linear gray scale and stores them in rbgtab */
{
  double multiplier;
  int i;

  /* straight scale from black to white */
  multiplier = ((double) MAXCOLGRAN) / MAXCOLORS;
  for (i = 0; i < MAXCOLORS; i++)
    rgbtab[i].green =  rgbtab[i].blue = rgbtab[i].red
      = (int) (i * multiplier);
}


static void
alloc_col ()
/* allocate colors from rgbtab until no more free cells,
   using existing colors if they match what we want */
{
  int start = MAXCOLORS / 2;		/* starting offset in an alloc sweep */
  int d = MAXCOLORS;			/* increment in an alloc sweep */
  int j,				/* index to the linear spectrum */
    k;					/* number of colors allocated */

  for (j = 0; j < MAXCOLORS; j++)
    cmap[j] = NONE;
  k = 0;

  /* add colors to cmap, keep them uniformly distributed in the range,
     and gradually make the spectrum more refined */
  while (d > 1)
    {
      /* add colors to cmap with d as the current distance
	 between new colors in the spectrum and start as the first location */
      j = start;
      while (j < MAXCOLORS)		/* completed a sweep of new colors */
	{
	  colors[k].flags = DoRed | DoGreen | DoBlue;	/* use all planes */
	  colors[k].red = rgbtab[j].red;
	  colors[k].green = rgbtab[j].green;
	  colors[k].blue = rgbtab[j].blue;
	  
	  cmap[j] = k;
	  if (XAllocColor (theDisplay, colormap, &(colors[k])) == 0)
	    {
	      cmap[j] = NONE;
	      clean_color_map(k);
	      return;
	    }
	  k++;				/* allocated one new color */
	  j += d;			/* next location in the spectrum */
	}
      d /= 2;				/* set up a tighter distance */
      start /= 2;			/* start lower in the spectrum */
    }
  clean_color_map(k);	                /* set # of colors, clean up */
}

static void
clean_color_map(k)
/* set the number of colors, print message, and clean up the map */
     int k;				/* number of colors allocated */
{
  int i, m;			/* counters for cleaning up cmap */

  /* colors[k-1] is the last valid colorcell */
  actual_color_range = k;

  if (actual_color_range < MINCOLORS)
    {
      fprintf (stderr, "Warning: obtained only %d colors\n", k);
      fprintf (stderr, "(consider using a private colormap)\n");
    }
  else
    printf ("Obtained %d colors.\n", k);

  /* clean up cmap; move all entries to the beginning */
  m = 0;
  while (m < MAXCOLORS && cmap[m] != NONE)
    ++m;
  for (i = m + 1; i < MAXCOLORS; i++)
    if (cmap[i] == NONE)	/* no colorcell */
      continue;
    else
      cmap[m] = cmap[i], ++m;
}


/*********************  GCs, fonts, resizing  ***********************/

static int
createGC (New_win, New_GC, fid, theFGpix, theBGpix)
  /* create a new graphics context for the given window with given
     font and foreground and background colors */
     Window New_win;		/* window for the GC */
     GC *New_GC;		/* return pointer to the created GC here */
     Font fid;			/* font id  */
     Pixel theFGpix, theBGpix;	/* foreground and background colors */
{
  XGCValues GCValues;		/* graphics context parameters; not used */

  *New_GC = XCreateGC (theDisplay, New_win, (unsigned long) 0, &GCValues);

  if (*New_GC == 0)		/* could not create */
    return (FALSE);
  else
    {
      /* set font, foreground and background */
      XSetFont (theDisplay, *New_GC, fid);
      XSetForeground (theDisplay, *New_GC, theFGpix);
      XSetBackground (theDisplay, *New_GC, theBGpix);
      return (TRUE);
    }
}


static XFontStruct *
loadFont (fontName)
  /* load a given font */
     char fontName[];		/* name of the font */
{
  XFontStruct *fontStruct;	/* return font here */

  if ((fontStruct = XLoadQueryFont (theDisplay, fontName)) == NULL)
    {
      fprintf (stderr, "Cannot load font: %s, using fixed\n", fontName);
      if ((fontStruct = XLoadQueryFont (theDisplay, "fixed")) == NULL)
	{
	  fprintf (stderr, "Cannot load fixed font\n");
	  exit (EXIT_X_ERROR);
	}
    }
  return (fontStruct);
}


static void
common_resize (modi, w)
  /* when resizing any net, first get the new window width and height */
     int modi;			/* module number */
     Widget w;			/* and its widget */
{
  Arg args[2];
  Dimension width, height;

  /* get the current width and height from the server */
  XtSetArg (args[0], XtNwidth, &width);
  XtSetArg (args[1], XtNheight, &height);
  XtGetValues (w, args, 2);
  /* and store them for further calculations */
  net[modi].width = width;
  net[modi].height = height;
}


/********************* processing module operations ***********************/

static void
init_common_proc_display_params (modi, w)
  /* initialize a number of parameters common for all processing modules */
     int modi;			/* module number */
     Widget w;			/* and its widget */
{
  int i;

  Win[modi] = XtWindow (w);	/* get a pointer to the window */
  /* calculate the number of slots at the input and output */
  for (i = 0; i < ninputs[modi]; i++)
    inputs[modi][i] = BLANKINDEX;	/* initially all blank words */
  for (i = 0; i < noutputs[modi]; i++)
    targets[modi][i] = BLANKINDEX;	/* initially all blank words */

  /* number of assembly columns on the display */
  if (modi == ANSWERPRODMOD)	/* answer producer has its input assemblies */
    net[modi].columns = nslot;	/*   in two layers: story rep is longer */
  else
    net[modi].columns = imax (ninputs[modi], noutputs[modi]);
}


/********************* expose event handlers */

static void
expose_sentpars (w, client_data, call_data)
  /* expose event handler for sentence parser: redraw the window */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = SENTPARSMOD;

  XClearWindow (theDisplay, Win[modi]);
  display_title (modi, titles[modi]);
  display_labels (modi, net[modi].tgtx,
		  net[modi].tgty + net[modi].uhght + asmboxhght,
		  caselabels, ncase);
  display_current_proc_net (modi);
}


static void
expose_storypars (w, client_data, call_data)
  /* expose event handler for story parser: redraw the window */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = STORYPARSMOD;

  XClearWindow (theDisplay, Win[modi]);
  display_title (modi, titles[modi]);
  display_labels (modi, net[modi].inpx, titleboxhght,
		  caselabels, ncase);
  display_labels (modi, net[modi].tgtx,
		  net[modi].tgty + net[modi].uhght + asmboxhght,
		  slotlabels, nslot);
  display_current_proc_net (modi);
}


static void
expose_storygen (w, client_data, call_data)
  /* expose event handler for story generator: redraw the window */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = STORYGENMOD;

  XClearWindow (theDisplay, Win[modi]);
  display_title (modi, titles[modi]);
  display_labels (modi, net[modi].inpx,
		  net[modi].inpy + net[modi].uhght + asmboxhght,
		  slotlabels, nslot);
  display_labels (modi, net[modi].outx, titleboxhght,
		  caselabels, ncase);
  display_current_proc_net (modi);
}


static void
expose_sentgen (w, client_data,	call_data)
  /* expose event handler for sentence generator: redraw the window */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = SENTGENMOD;

  XClearWindow (theDisplay, Win[modi]);
  display_title (modi, titles[modi]);
  display_labels (modi, net[modi].inpx,
		  net[modi].inpy + net[modi].uhght + asmboxhght,
		  caselabels, ncase);
  display_current_proc_net (modi);
}


static void
expose_cueformer (w, client_data, call_data)
  /* expose event handler for cue former: redraw the window */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = CUEFORMMOD;

  XClearWindow (theDisplay, Win[modi]);
  display_title (modi, titles[modi]);
  display_labels (modi, net[modi].inpx, titleboxhght,
		  caselabels, ncase);
  display_labels (modi, net[modi].tgtx,
		  net[modi].tgty + net[modi].uhght + asmboxhght,
		  slotlabels, nslot);
  display_current_proc_net (modi);
}


static void
expose_answerprod (w, client_data, call_data)
  /* expose event handler for answer producer: redraw the window */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = ANSWERPRODMOD;

  XClearWindow (theDisplay, Win[modi]);
  display_title (modi, titles[modi]);
  /* input is in two layers: cue former and episodic memory */
  display_labels (modi, net[modi].inpx,
		  net[modi].inpy + net[modi].uhght + asmboxhght,
		  caselabels, ncase);
  display_labels (modi, net[modi].inp1x,
		  net[modi].inp1y + net[modi].uhght + asmboxhght,
		  slotlabels, nslot);
  display_labels (modi, net[modi].outx, titleboxhght,
		  caselabels, ncase);
  display_current_proc_net (modi);
}


/********************* current activations */

void
display_current_proc_net (modi)
  /* display all activations and the current log */
     int modi;			/* module number */
{
  int inplabels, outlabels;	/* where to place the labels of reps */

  /* first figure out where the labels should go */
  if (modi == SENTPARSMOD || modi == STORYPARSMOD || modi == CUEFORMMOD)
    {
      inplabels = ABOVE;
      outlabels = BELOW2;
    }
  else
    {
      inplabels = BELOW;
      outlabels = ABOVE2;
    }

  /* display the error line first (in case the display is slow) */
  display_log (modi);

  /* with sentence parser, next display the word sequence */
  if (modi == SENTPARSMOD)
    display_sequence (modi, net[modi].inpx, net[modi].inpy);

  /* input layer */
  if (modi != ANSWERPRODMOD)
    display_labeled_layer (modi, ninputs[modi], inprep[modi],
			   inputs[modi], net[modi].inpx, net[modi].inpy,
			   inplabels);
  else
    {
      display_labeled_layer (modi, ncase, inprep[modi], inputs[modi],
			     net[modi].inpx, net[modi].inpy, inplabels);
      display_labeled_layer (modi, nslot, &inprep[modi][ncaserep],
			     &inputs[modi][ncase], net[modi].inp1x,
			     net[modi].inp1y, BELOW);
    }
  
  /* previous hidden layer */
  if (modi != CUEFORMMOD && modi != ANSWERPRODMOD)
    display_assembly (modi, net[modi].prevx, net[modi].prevy,
		      prevhidrep[modi], nhidrep[modi]);

  /* hidden layer */
  display_assembly (modi, net[modi].hidx, net[modi].hidy,
		    hidrep[modi], nhidrep[modi]);

  /* with the sentence parser, display the word sequence right before output */
  if (modi == SENTGENMOD)
    display_sequence (modi, net[modi].tgtx, net[modi].tgty);

  /* output and target layer */
  display_labeled_layer (modi, noutputs[modi], outrep[modi], targets[modi],
			 net[modi].outx, net[modi].outy, outlabels);
  display_layer (modi, noutputs[modi], tgtrep[modi],
		 net[modi].tgtx, net[modi].tgty, nsrep);
  
}


/********************* resizing */

static void
common_proc_resize (modi)
  /* calculate horizontal geometry common to all processing module displays */
     int modi;			/* module number */
{
  /* width of the boxes representing unit activities */
  net[modi].uwidth =
    /* either the I/O columns fill the window or the previous hidden layer */
    imin ((net[modi].width - 2 * HORSP) / (net[modi].columns * nsrep),
	  (net[modi].width - 2 * HORSP - PREVSP) / nhidrep[modi]);
  net[modi].hsp = nsrep * net[modi].uwidth;     /* space between assemblies */
  net[modi].marg = HORSP;			/* horizontal margin */

  /* x-coordinates of the left sides of each layer, centered in the window */
  if (modi == ANSWERPRODMOD)	/* answer producer has two input layers */
    {
      net[modi].inpx = (net[modi].width - net[modi].hsp * ncase) / 2;
      net[modi].inp1x = (net[modi].width - net[modi].hsp * nslot) / 2;
    }
  else
    net[modi].inpx =		/* input layer for other modules */
      (net[modi].width - net[modi].hsp * ninputs[modi]) / 2;
  net[modi].hidx =		/* hidden layer */
    (net[modi].width - nhidrep[modi] * net[modi].uwidth) / 2;
  net[modi].prevx =		/* previous hidden layer is displaced right */
    net[modi].hidx + PREVSP;
  net[modi].outx =		/* output layer */
    (net[modi].width - net[modi].hsp * noutputs[modi]) / 2;
  net[modi].tgtx =		/* target pattern, same x as output */
    net[modi].outx;
}


static void
resize_sentpars (w, client_data, call_data)
  /* resize event handler for sentence parser: recalculate geometry */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = SENTPARSMOD;

  common_resize (modi, sentpars);	/* get new window size */
  common_proc_resize (modi);		/* calculate x-coordinates */
  /* height of the boxes representing unit activities */
  net[modi].uhght = imax (0, (net[modi].height - titleboxhght -
			      3 * asmboxhght - HIDSP - 3 * VERSP) / 5);
  /* y-coordinates of the tops of each layer */
  net[modi].inpy = titleboxhght + asmboxhght;
  net[modi].prevy = net[modi].inpy + net[modi].uhght + VERSP;
  net[modi].hidy = net[modi].prevy + net[modi].uhght + HIDSP;
  net[modi].outy = net[modi].hidy + net[modi].uhght + VERSP;
  net[modi].tgty = net[modi].outy + net[modi].uhght;
}


static void
resize_storypars (w, client_data, call_data)
  /* resize event handler for story parser: recalculate geometry */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = STORYPARSMOD;

  common_resize (modi, storypars);	/* get new window size */
  common_proc_resize (modi);		/* calculate x-coordinates */
  /* height of the boxes representing unit activities */
  net[modi].uhght = imax (0, (net[modi].height - titleboxhght -
			      4 * asmboxhght - HIDSP - 3 * VERSP) / 5);
  /* y-coordinates of the tops of each layer */
  net[modi].inpy = titleboxhght + 2 * asmboxhght;
  net[modi].prevy = net[modi].inpy + net[modi].uhght + VERSP;
  net[modi].hidy = net[modi].prevy + net[modi].uhght + HIDSP;
  net[modi].outy = net[modi].hidy + net[modi].uhght + VERSP;
  net[modi].tgty = net[modi].outy + net[modi].uhght;
}


static void
resize_storygen (w, client_data, call_data)
  /* resize event handler for story generator: recalculate geometry */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = STORYGENMOD;

  common_resize (modi, storygen);	/* get new window size */
  common_proc_resize (modi);		/* calculate x-coordinates */
  /* height of the boxes representing unit activities */
  net[modi].uhght = imax (0, (net[modi].height - titleboxhght -
			      4 * asmboxhght - HIDSP - 3 * VERSP) / 5);
  /* y-coordinates of the tops of each layer */
  net[modi].tgty = titleboxhght + 2 * asmboxhght;
  net[modi].outy = net[modi].tgty + net[modi].uhght;
  net[modi].hidy = net[modi].outy + net[modi].uhght + VERSP;
  net[modi].prevy = net[modi].hidy + net[modi].uhght + HIDSP;
  net[modi].inpy = net[modi].prevy + net[modi].uhght + VERSP;
}


static void
resize_sentgen (w, client_data, call_data)
  /* resize event handler for sentence generator: recalculate geometry */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = SENTGENMOD;

  common_resize (modi, sentgen);	/* get new window size */
  common_proc_resize (modi);		/* calculate x-coordinates */
  /* height of the boxes representing unit activities */
  net[modi].uhght = imax (0, (net[modi].height - titleboxhght -
			      3 * asmboxhght - HIDSP - 3 * VERSP) / 5);
  /* y-coordinates of the tops of each layer */
  net[modi].tgty = titleboxhght + asmboxhght;
  net[modi].outy = net[modi].tgty + net[modi].uhght;
  net[modi].hidy = net[modi].outy + net[modi].uhght + VERSP;
  net[modi].prevy = net[modi].hidy + net[modi].uhght + HIDSP;
  net[modi].inpy = net[modi].prevy + net[modi].uhght + VERSP;
}


static void
resize_cueformer (w, client_data, call_data)
  /* resize event handler for cue former: recalculate geometry */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = CUEFORMMOD;

  common_resize (modi, cueformer);	/* get new window size */
  common_proc_resize (modi);		/* calculate x-coordinates */
  /* height of the boxes representing unit activities */
  net[modi].uhght = imax (0, (net[modi].height - titleboxhght -
			      4 * asmboxhght - 3 * VERSP) / 4);
  /* y-coordinates of the tops of each layer */
  net[modi].inpy = titleboxhght + 2 * asmboxhght;
  net[modi].hidy = net[modi].inpy + net[modi].uhght + VERSP;
  net[modi].outy = net[modi].hidy + net[modi].uhght + VERSP;
  net[modi].tgty = net[modi].outy + net[modi].uhght;
}


static void
resize_answerprod (w, client_data, call_data)
  /* resize event handler for answer producer: recalculate geometry */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = ANSWERPRODMOD;

  common_resize (modi, answerprod);	/* get new window size */
  common_proc_resize (modi);		/* calculate x-coordinates */
  /* height of the boxes representing unit activities */
  net[modi].uhght = imax (0, (net[modi].height - titleboxhght -
			      6 * asmboxhght - HIDSP - 3 * VERSP) / 5);
  /* y-coordinates of the tops of each layer */
  net[modi].tgty = titleboxhght + 2 * asmboxhght;
  net[modi].outy = net[modi].tgty + net[modi].uhght;
  net[modi].hidy = net[modi].outy + net[modi].uhght + VERSP;
  net[modi].inp1y = net[modi].hidy + net[modi].uhght + VERSP;
  net[modi].inpy =  net[modi].inp1y + net[modi].uhght + 2 * asmboxhght + HIDSP;
}


/********************* proc display subroutines */

void
display_sequence (modi, x, y)
  /* write out the word sequence so far */
     int modi,		/* module number (sentence parser or generator) */
     x, y;		/* top left corner of the input/output assembly */
{
  /* first clear out the old sequence from the display */
  clearRectangle (modi, 0, titleboxhght, x, asmboxhght);
  drawText (modi,
	    /* calculate bottom left corner of the text */
	    x - net[modi].marg - XTextWidth (asmfontStruct, net[modi].sequence,
					     strlen (net[modi].sequence)),
	    y - asmboxhght + asmfontStruct->ascent + BOXSP,
	    net[modi].sequence, asmGC);
  XFlush (theDisplay);
}


void
display_error (modi, outrep, nas, target, words, nrep, step)
  /* write out the current average error per output unit */
     int modi;			/* module number */
     double outrep[];		/* output layer activities */
     int nas,			/* number of assemblies */
     target[];			/* target word indices */
     WORDSTRUCT words[];	/* lexicon */
     int nrep,			/* word rep dimension */
     step;			/* step in the sequence */
{
  int i, j;
  double sum = 0.0;

  /* calculate the error sum and average it per unit */
  for (i = 0; i < nas; i++)
    for (j = 0; j < nrep; j++)
      sum += fabs (words[target[i]].rep[j] - outrep[i * nrep + j]);
  sprintf (net[modi].log, "Step %d: Eavg %.3f", step, sum / (nas * nrep));
  display_log (modi);		/* write out the result */
}


static void
display_labels (modi, x, y, labels, nitem)
  /* write out the names of the assemblies in a layer */
     int modi, x, y;		/* module number, top left of the labels */
     char *labels[];		/* labels */
     int nitem;			/* number of assemblies */
{
  int i;

  for (i = 0; i < nitem; i++)
    /* enclose the name inside a box */
    display_boxword (modi, x + i * net[modi].hsp, y,
		     net[modi].hsp, asmboxhght,
		     labels[i], TRUE, asmfontStruct, asmGC);
}


void
display_labeled_layer (modi, nas, rep, nums, x, y, labeloffset)
  /* display a layer of activations and the labels of the closest words */
     int modi, nas;		/* module number, number of assemblies */
     double rep[];		/* activations in the layer */
     int nums[],		/* indices of the correct words */
       x, y,			/* top left of the layer */
       labeloffset;		/* vertical displacement of the labels */
{
  int i, j;
  char s[2 * MAXWORDL + 4 + 1];	/* label: may contain correct label also */
  int nearest;			/* index of the nearest rep in the lexicon */
  int output_ok = TRUE;		/* whether the pattern is actual activation */

  /* check if the output pattern is not activation but a cleared pattern */
  if (rep == outrep[modi])
    {
      output_ok = FALSE;
      for (i = 0; i < nas && output_ok == FALSE; i++)
	for (j = 0; j < nsrep; j++)
	  if (outrep[modi][i * nsrep + j] != 0.0)
	    {
	      output_ok = TRUE;
	      break;
	    }
    }
  
  /* display the unit activations */
  display_layer (modi, nas, rep, x, y, nsrep);
  /* display the labels of the closest words and correct words */
  for (i = 0; i < nas; i++)
    {
      /* get the index of the closest word representation in the lexicon */
      if (output_ok)
	nearest = find_nearest (&rep[i * nsrep], swords, nsrep, nswords);
      else
	/* if the pattern has just been cleared, use targets instead */
	nearest =
	  find_nearest (&tgtrep[modi][i * nsrep], swords, nsrep, nswords);
      if (nearest == nums[i] || text_question)
	/* if it is correct or we are processing a question from the
	   keyboard, only output the label of the closest word */
	{
	  sprintf (s, "%s", swords[nearest].chars);
	  display_boxword (modi, x + i * net[modi].hsp, y + labeloffset,
			   net[modi].hsp, asmboxhght,
			   swords[nearest].chars, nearest != BLANKINDEX,
			   asmfontStruct, asmGC);
	}
      else
	/* closest word is not the correct one; output the closest label
	   followed by the correct label in parenthesis, enclosed in ** */
	{
	  sprintf (s, "*%s(%s)*", swords[nearest].chars,
		   swords[nums[i]].chars);
	  display_boxword (modi, x + i * net[modi].hsp, y + labeloffset,
			   net[modi].hsp, asmboxhght,
			   s, TRUE, asmerrorfontStruct, asmerrorGC);
	}
    }
  /* update the input/output word sequence */
  /* only if the pattern in the input/output is real activation */
  if (((modi == SENTPARSMOD && rep == inprep[modi] && nums[0] != BLANKINDEX) ||
       (modi == SENTGENMOD && rep == outrep[modi] && output_ok)))
    sprintf (net[modi].newitem, "%s", s);
}


void
display_layer (modi, nas, layer, x, y, nrep)
  /* display a layer of unit activities */
     int modi, nas;		/* module number, number of assemblies */
     double layer[];		/* activities */
     int x, y,			/* top left corner of the layer */
     nrep;			/* number of units in word representation */
{
  int i;

  for (i = 0; i < nas; i++)
    /* display one assembly at a time */
    display_assembly (modi, x + i * net[modi].hsp, y, &layer[i * nrep], nrep);
}


void
display_assembly (modi, x, y, assembly, nrep)
  /* display the activities of the units in one assembly */
     int modi,			/* module number */
     x, y;			/* top left corner of the assembly */
     double assembly[];		/* activities */
     int nrep;			/* number of units in assembly */
{
  int i;

  for (i = 0; i < nrep; i++)
    /* display one unit at a time */
    /* translate activation to color using the black-red-yellow-white scale */
    fillRectangle (modi, x + i * net[modi].uwidth, y,
		   net[modi].uwidth, net[modi].uhght,
		   trans_to_color (assembly[i], UNITCOLORS));
  /* draw a boundary around the assembly */
  drawRectangle (modi, x, y,  nrep * net[modi].uwidth, net[modi].uhght);
  XFlush (theDisplay);
}


static void
display_boxword (modi, x, y, width, height, wordchars, dodisplay, fontStruct, 
		 currGC)
  /* write out a string inside a box */
     int modi,			/* module number */
       x, y,			/* top left corner of the box */
       width, height;		/* of the box */
     char wordchars[];		/* the string to be written */
     int dodisplay;		/* whether to display the word */
     XFontStruct *fontStruct;	/* font to be used */
     GC currGC;			/* write it in this context */
{
  int i;			/* last char in the string */
  char s[2 * MAXWORDL + 4 + 1];	/* copy the string here */

  /* first clear the area of the box */
  clearRectangle (modi, x, y, width, height);

  if (dodisplay)		/* if really writing the word */
    {
      /* if necessary, cut the text from the right to make it fit */
      /* first copy it to something we can always modify */
      sprintf (s, "%s", wordchars);
      for (i = strlen (s);
	   XTextWidth (fontStruct, s, strlen (s)) > width;
	   s[--i] = '\0')
	;
      /* write the word neatly centered inside the box area */
      drawoverText (modi,
		    x + (width - XTextWidth (fontStruct, s, strlen (s))) / 2,
		    y + height - BOXSP - asmfontStruct->descent, s, currGC);
    }
  
  /* draw the box around the word */
  drawRectangle (modi, x, y, width, height);
  XFlush (theDisplay);
}


static int
imin (a, b)
  /* return the smaller of two ints */
     int a, b;
{
  return ((a > b) ? b : a);
}


static int
imax (a, b)
  /* return the larger of two ints */
     int a, b;
{
  return ((a > b) ? a : b);
}


/********************* hfm operations ***********************/

static void
init_hfm_display_params (modi, w)
/* find the image units for each story representation, clear the values */
     int modi;			/* module number */
     Widget w;			/* and its widget */
{
  /* save the previous state of displaying and logging parameters */
  int displaying_save = displaying,
    babbling_save = babbling,
    print_mistakes_save = print_mistakes;
  int i, j, k,
  fewest,				/* smallest number of labels on unit */
  slots[MAXSLOT];			/* story representation indices */
  char toplabel[MAXSTRL + 1],		/* labels for the image units */
  midlabel[MAXSTRL + 1],		/*  of a story at the three levels */
  botlabel[MAXSTRL + 1];
  char rest[MAXSTRL + 1];		/* words of the slot-filler rep. */
  FILE *fp;				/* story representation+label file */
  IMAGEDATA image;			/* image unit indices and value */

  Win[modi] = XtWindow (w);		/* get a pointer to the window */

  /* figure out labels for the units in the episodic memory */
  /* open the story representation and label file */
  printf ("Reading hfmlabels from %s...", hfminpfile);
  open_file (hfminpfile, "r", &fp, REQUIRED);

  /* turn off display and log output temporarily */
  displaying = FALSE;
  babbling = FALSE;
  print_mistakes = FALSE;
  /* ignore everything until data */
  read_till_keyword (fp, HFMINP_REPS, REQUIRED);

  /* clear up the possible previous labels */
  hfm_clear_labels ();

  /* read the labels for the three levels */
  for (i = 0;
       fscanf (fp, "%s %s %s", toplabel, midlabel, botlabel) != EOF;
       i++)
    {
      if (strlen (toplabel) >= MAXWORDL ||
	  strlen (midlabel) >= MAXWORDL ||
	  strlen (botlabel) >= MAXWORDL)
	{
	  fprintf (stderr,
		   "Length of hfm label %s %s %s exceeds array size\n",
		   toplabel, midlabel, botlabel);
	  exit (EXIT_SIZE_ERROR);
	}
      /* read the rest of the line */
      fgetline (fp, rest, MAXSTRL);
      /* convert the words in the assemblies into indices */
      if (list2indices (slots, rest, nslot, INP_SLOTS) != nslot)
	{
	  fprintf (stderr, "Wrong number of %s in %s size\n",
		   INP_SLOTS, hfminpfile);
	  exit (EXIT_DATA_ERROR);
	}
      /* form the input vector for the episodic memory */
      for (j = 0; j < ninputs[modi]; j++)
	for (k = 0; k < nsrep; k++)
	  inprep[modi][j * nsrep + k] = swords[slots[j]].rep[k];
      /* present it to the memory for classification */
      classify (modi, &image);
      /* add the labels to the image units' list of labels
         if they aren't already there and if the list is not full */
      collect_uniq_labels (topunits[image.topi][image.topj].labels,
			   &topunits[image.topi][image.topj].labelcount,
			   toplabel, MAXFMLABELS);
      collect_uniq_labels (midunits[image.topi][image.topj]
			           [image.midi][image.midj].labels,
			   &midunits[image.topi][image.topj]
			            [image.midi][image.midj].labelcount,
			   midlabel, MAXFMLABELS);
      collect_labels (botunits[image.topi][image.topj]
		              [image.midi][image.midj]
		              [image.boti][image.botj].labels,
		      &botunits[image.topi][image.topj]
		               [image.midi][image.midj]
		               [image.boti][image.botj].labelcount,
		      botlabel, MAXFMLABELS);
    }
  fclose (fp);
  printf ("Done.\n");

  /* restore the displaying and logging parameters */
  displaying = displaying_save;
  babbling = babbling_save;
  print_mistakes = print_mistakes_save;
  /* clear the episodic memory */
  hfm_clear_values ();
  hfm_clear_prevvalues ();

  /* figure out where the top-level unit with the fewest labels is
     (for positioning the top- and middle-level maps */
  fewest = LARGEINT;
  for (i = 0; i < ntopnet; i++)
    for (j = 0; j < ntopnet; j++)
      if (topunits[i][j].labelcount < fewest)
	{
	  fewesttopi = i;
	  fewesttopj = j;
	  fewest = topunits[i][j].labelcount < fewest;
	}
}


void
display_new_hfm_labels ()
/* read the labels and clear the hfm display (for later redisplay) */
/* called in main.c after hfmlabelfile changes */
{
  init_hfm_display_params (HFMWINMOD, hfm);
  XClearArea (theDisplay, Win[HFMWINMOD], 0, 0, 0, 0, True);
  XFlush (theDisplay);
}


/********************* expose event handler */
static void
expose_hfm (w, client_data, call_data)
  /* expose event handler for the hfm: redraw the window */
  /* standard callback parameters, not used here */
     Widget w; XtPointer client_data, call_data;
{
  int modi = HFMWINMOD;
  XClearWindow (theDisplay, Win[modi]);
  display_title (modi, titles[modi]);
  hfm_clear_prevvalues ();
  display_hfm_values ();
  display_log (modi);
}


/********************* resizing */
static void
resize_hfm (w, client_data, call_data)
  /* resize event handler for the hfm: recalculate geometry */
  /* standard callback parameters, not used here */
     Widget w;
     XtPointer client_data, call_data;
{
  int modi = HFMWINMOD;
  common_resize (modi, hfm);	/* get new window size */

  /* heights of the boxes representing unit activities */
  net[modi].uhght =
    (net[modi].height - titleboxhght - VERSP - (ntopnet * nmidnet - 1) * HFMSP)
    / (ntopnet * nmidnet * ntopnet);
  net[modi].midhght = net[modi].uhght * ntopnet / nmidnet;
  net[modi].bothght = net[modi].midhght * nmidnet / nbotnet;

  /* widths of the boxes representing unit activities */
  net[modi].uwidth =
    (net[modi].width - 2 * HORSP - (ntopnet * nmidnet - 1) * HFMSP)
    / (ntopnet * nmidnet * ntopnet);
  net[modi].midwidth = net[modi].uwidth * ntopnet / nmidnet;
  net[modi].botwidth = net[modi].midwidth * nmidnet / nbotnet;

  /* horizontal margin from the edge of the window */
  net[modi].marg = (net[modi].width - ntopnet * nmidnet * ntopnet * 
		    net[modi].uwidth - (ntopnet * nmidnet - 1) * HFMSP) / 2;
}


/*********************  mouse button press handlers  */
static void
hfmmouse_handler (w, client_data, p_event)
/* decides which handler to call  */
     Widget w;
     XtPointer client_data;
     XEvent *p_event;
{
  int x = p_event->xbutton.x,		/* mouse coordinates */
      y = p_event->xbutton.y;
  int modi = HFMWINMOD;			/* module number */
  int toplowx, toplowy,			/* coordinates of the top-level disp */
    top2lowx, top2lowy;			/* coordinates of the top 2 levels */
  /* invert the vertical axis, following the display convention in the book */
  int invfewesttopj = (ntopnet - 1) - fewesttopj;

  /* exit if mouseevents are not currently allowed */
  if (nohfmmouseevents)
    return;
  
  /* only accept points within the network display */
  if (x >= net[modi].marg && y >= titleboxhght &&
      x <= net[modi].marg + ntopnet * ntopnet * nmidnet * net[modi].midwidth +
           (ntopnet * nmidnet - 1) * HFMSP -
           (nmidnet * net[modi].midwidth - nbotnet * net[modi].botwidth) &&
      y <= titleboxhght + ntopnet * ntopnet * nmidnet * net[modi].midhght +
           (ntopnet * nmidnet - 1) * HFMSP -
           (nmidnet * net[modi].midhght - nbotnet * net[modi].bothght))
    {
      /* calculate the upper left corner of top & mid-level display */
      top2lowx = net[modi].marg + (fewesttopi * nmidnet) *
	ntopnet * net[modi].uwidth + (fewesttopi * nmidnet) * HFMSP;
      top2lowy = titleboxhght + (invfewesttopj * nmidnet) *
	ntopnet * net[modi].uhght + (invfewesttopj * nmidnet) * HFMSP;

      /* send the event to bottom maps if it is not within the top2-box */
      if (!(x >= top2lowx &&
	    x <= top2lowx + ntopnet * nmidnet * net[modi].midwidth +
	                   (ntopnet * nmidnet - 1) * HFMSP &&
	    y >= top2lowy &&
	    y <= top2lowy + ntopnet * nmidnet * net[modi].midhght + 
	                   (ntopnet * nmidnet - 1) * HFMSP))
	botmouse_handler (x - net[modi].marg, y - titleboxhght);
      else
	{
	  /* calculate the upper left corner of top-level display */
	  toplowx = net[modi].marg +
	    (fewesttopi * nmidnet + fewesttopi) * ntopnet * net[modi].uwidth +
	      (fewesttopi * nmidnet + fewesttopi) * HFMSP;
	  toplowy = titleboxhght +
	    (invfewesttopj*nmidnet + invfewesttopj) * ntopnet*net[modi].uhght +
	      (invfewesttopj * nmidnet + invfewesttopj) * HFMSP;
	  /* send the event to middle maps if it is not within the topbox
	     or the HFMSPACE around it */
	  if (!(x >= toplowx &&
		x <= toplowx + ntopnet * net[modi].uwidth + HFMSP &&
		y >= toplowy &&
		y <= toplowy + ntopnet * net[modi].uhght + HFMSP))
	    midmouse_handler (x - top2lowx, y - top2lowy);
	  else
	    topmouse_handler (x - toplowx, y - toplowy);
	}
    }
}


static void
topmouse_handler (x, y)
/* handles the button press events for the toplevel map */
  int x, y;				/* mouse coordinates within topbox */
{
  int modi = HFMWINMOD;			/* module number */
  int uniti, unitj;			/* representation of this unit shown */
  int i, j;
  double sum = 0.0;			/* error sum */
  int closest[MAXSLOT];			/* closest word indices */
  int nindices = 0;			/* number of noncompressed lines */
    
  /* calculate which unit is requested */
  uniti = imin (ntopnet - 1, x / net[modi].uwidth);
  unitj = ntopnet - 1 - imin (ntopnet - 1, y / net[modi].uhght);

  /* turn it and it alone on in the map */
  hfm_clear_values ();
  topunits[uniti][unitj].value = 1.0;
  display_hfm_values ();
      
  /* display the labels of the closest words in the lexicon */
  for (i = 0; i < ninputs[modi]; i++)
    closest[i] = find_nearest (&topunits[uniti][unitj].comp[i * nsrep],
			       swords, nsrep, nswords);
  sprintf (net[modi].log, "%s", "");
  for (i = 0; i < ninputs[modi]; i++)
    /* if this wordrep has compressed lines, put it in parenthesis */
    if (compressed_lines (midindices[uniti][unitj],
			  nmidindices[uniti][unitj],
			  i * nsrep, (i + 1) * nsrep))
      sprintf (net[modi].log, "%s (%s)", net[modi].log,
	       swords[closest[i]].chars);
    else
      sprintf (net[modi].log, "%s %s", net[modi].log,
	       swords[closest[i]].chars);
  
  /* calculate the error in the map representation */
  for (i = 0; i < noutputs[modi]; i++)
    for (j = 0; j < nsrep; j++)
      /* only count lines that were not compressed */
      if (!compressed_lines (midindices[uniti][unitj],
			     nmidindices[uniti][unitj],
			     i * nsrep + j,
			     i * nsrep + j + 1))
	{
	  sum += fabs (swords[closest[i]].rep[j] -
		       topunits[uniti][unitj].comp[i * nsrep + j]);
	  nindices++;
	}
  if (nindices > 0)
    sprintf (net[modi].log, "%s: Davg %.3f/%d",
	     net[modi].log, sum / nindices, nindices);
  else
    sprintf (net[modi].log, "%s: Davg 0/0", net[modi].log);
  
  display_log (modi);
}


static void
midmouse_handler (x, y)
/* handles the button press events for the midlevel map */
  int x, y;				/* mouse coordinates within topbox */
{
  int modi = HFMWINMOD;			/* module number */
  int topuniti, topunitj, uniti, unitj;	/* representation of this unit shown */
  int i, j;
  double sum = 0.0;			/* error sum */
  int closest[MAXSLOT];			/* closest word indices */
  int nindices = 0;			/* number of noncompressed lines */
  double midrep[MAXSLOT * MAXREP];	/* output representation of the unit */
    
  /* calculate which unit is requested (approximately) */
  uniti = imin (ntopnet * nmidnet - 1,
		(int) (x / (net[modi].midwidth +
		      ((double) (ntopnet - 1) * HFMSP) /(ntopnet * nmidnet))));
  unitj = imin (ntopnet * nmidnet - 1,
		(int) (y / (net[modi].midhght +
		      ((double) (ntopnet - 1) * HFMSP) /(ntopnet * nmidnet))));
  topuniti = uniti / nmidnet;
  topunitj = ntopnet - 1 - unitj / nmidnet;
  uniti = uniti % nmidnet;
  unitj = nmidnet - 1 - unitj % nmidnet;

  /* turn it and it alone on in the map */
  hfm_clear_values ();
  midunits[topuniti][topunitj][uniti][unitj].value = 1.0;
  display_hfm_values ();
      
  /* form the representation vector */
  for (i = 0; i < noutrep[modi]; i++)
    midrep[i] = topunits[topuniti][topunitj].comp[i];
  for (i = 0; i < nmidindices[topuniti][topunitj]; i++)
    midrep[midindices[topuniti][topunitj][i]] =
      midunits[topuniti][topunitj][uniti][unitj].comp[i];

  /* display the labels of the closest words in the lexicon */
  for (i = 0; i < ninputs[modi]; i++)
    closest[i] = find_nearest (&midrep[i * nsrep], swords, nsrep, nswords);
  sprintf (net[modi].log, "%s", "");
  for (i = 0; i < ninputs[modi]; i++)
    /* if this wordrep has compressed lines, put it in parenthesis */
    if (compressed_lines (botindices[topuniti][topunitj][uniti][unitj],
			  nbotindices[topuniti][topunitj][uniti][unitj],
			  i * nsrep, (i + 1) * nsrep))
      sprintf (net[modi].log, "%s (%s)", net[modi].log,
	       swords[closest[i]].chars);
    else
      sprintf (net[modi].log, "%s %s", net[modi].log,
	       swords[closest[i]].chars);
  
  /* calculate the error in the map representation */
  for (i = 0; i < noutputs[modi]; i++)
    for (j = 0; j < nsrep; j++)
      /* only count lines that were not compressed */
      if (!compressed_lines (botindices[topuniti][topunitj][uniti][unitj],
			     nbotindices[topuniti][topunitj][uniti][unitj],
			     i * nsrep + j,
			     i * nsrep + j + 1))
	{
	  sum += fabs (swords[closest[i]].rep[j] - midrep[i * nsrep +j]);
	  nindices++;
	}
  if (nindices > 0)
    sprintf (net[modi].log, "%s: Davg %.3f/%d",
	     net[modi].log, sum / nindices, nindices);
  else
    sprintf (net[modi].log, "%s: Davg 0/0", net[modi].log);
  
  display_log (modi);
}


static void
botmouse_handler (x, y)
/* handles the button press events for the botlevel map */
  int x, y;				/* mouse coordinates within topbox */
{
  int modi = HFMWINMOD;			/* module number */
  int topuniti, topunitj,
    miduniti, midunitj,
    uniti, unitj, tmpunitj;		/* representation of this unit shown */
  int i, j;
  double sum = 0.0;			/* error sum */
  int closest[MAXSLOT];			/* closest word indices */
  double botrep[MAXSLOT * MAXREP];	/* output representation of the unit */
    
  /* calculate which unit is requested (approximately) */
  /* the y-axis is inverted in the top and mid-levels,
     and the axes are flipped at the bottom level in the discern display */
  uniti = imin (ntopnet * nmidnet * nbotnet - 1,
		(int) (x /
		(net[modi].botwidth + ((double) (ntopnet * nmidnet - 1) *
				      (HFMSP + nmidnet * net[modi].midwidth -
				       nbotnet * net[modi].botwidth))
		/ (ntopnet * nmidnet * nbotnet))));
  unitj = imin (ntopnet * nmidnet * nbotnet - 1,
		(int) (y /
		(net[modi].bothght + ((double) (ntopnet * nmidnet - 1) *
				      (HFMSP + nmidnet * net[modi].midhght -
				       nbotnet * net[modi].bothght))
		 / (ntopnet * nmidnet * nbotnet))));
  topuniti = uniti / (nmidnet * nbotnet);
  topunitj = ntopnet - 1 - unitj / (nmidnet * nbotnet);
  miduniti = (uniti % (nmidnet * nbotnet)) / nbotnet;
  midunitj = nmidnet - 1 - (unitj % (nmidnet * nbotnet)) / nbotnet;
  tmpunitj = (uniti % (nmidnet * nbotnet)) % nbotnet;
  uniti = (unitj % (nmidnet * nbotnet)) % nbotnet;
  unitj = tmpunitj;

  /* turn it and it alone on in the map */
  hfm_clear_values ();
  botunits[topuniti][topunitj][miduniti][midunitj][uniti][unitj].value = 1.0;
  display_hfm_values ();
      
  /* form the representation vector */
  for (i = 0; i < noutrep[modi]; i++)
    botrep[i] = topunits[topuniti][topunitj].comp[i];
  for (i = 0; i < nmidindices[topuniti][topunitj]; i++)
    botrep[midindices[topuniti][topunitj][i]] =
      midunits[topuniti][topunitj][miduniti][midunitj].comp[i];
  for (i = 0;
       i < nbotindices[topuniti][topunitj][miduniti][midunitj];
       i++)
    botrep[botindices[topuniti][topunitj][miduniti][midunitj][i]] =
      botunits[topuniti][topunitj][miduniti][midunitj][uniti][unitj].comp[i];

  /* display the labels of the closest words in the lexicon */
  for (i = 0; i < ninputs[modi]; i++)
    closest[i] = find_nearest (&botrep[i * nsrep], swords, nsrep, nswords);
  sprintf (net[modi].log, "%s", "");
  for (i = 0; i < ninputs[modi]; i++)
    sprintf (net[modi].log, "%s %s", net[modi].log,
	     swords[closest[i]].chars);

  /* calculate the error in the map representation */
  for (i = 0; i < noutputs[modi]; i++)
    for (j = 0; j < nsrep; j++)
      sum += fabs (swords[closest[i]].rep[j] - botrep[i * nsrep + j]);
  sprintf (net[modi].log, "%s: Davg %.3f/%d",
	   net[modi].log, sum / noutrep[modi], noutrep[modi]);
  display_log (modi);
}


static int
compressed_lines (indices, nindices, low, high)
/* check whether there are indices within the given range */
int indices[],			/* compression index list */
  nindices,			/* length of the list */
  low, high;			/* range */
{
  int j;
  for (j = 0; j < nindices; j++)
    if (indices[j] >= low && indices[j] < high)
      return (TRUE);
  return (FALSE);
}


/********************* hfm display subroutines */
void
display_hfm_values ()
/* display the map activations */
{
  int topi, topj, midi, midj;

  /* It would be nice to display all bottom-level and middle-level maps, but
     there is no easy way to fit them on the display. So, the top-level map
     with fewest number of labels is found (above in init_hfm_display_params),
     and the middle-level maps are shown over its bottom-level maps, and the
     top-level map is displayed over its middle-level maps. In the current
     DISCERN example those maps do not receive any input, so nothing is lost */

  /* first the bottom level maps */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      for (midi = 0; midi < nmidnet; midi++)
	for (midj = 0; midj < nmidnet; midj++)
	  display_hfm_bot (topi, topj, midi, midj);
  /* then the mid-level maps over the least busy quadrangle */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      display_hfm_mid (topi, topj);
  /* and the top-level map over the least busy middle-level map */
  display_hfm_top ();
}


void
display_hfm_top ()
/* display the activations of the top level map */
{
  int modi = HFMWINMOD;
  /* invert the vertical axis, following the display convention in the book */
  int invfewesttopj = (ntopnet - 1) - fewesttopj;

  /* display the map over the least busy quadrangle, over the least busy
     middle-level map */
  display_top_box (modi,
		   ntopnet,
		   net[modi].marg + (fewesttopi * nmidnet + fewesttopi) *
		                             ntopnet * net[modi].uwidth +
		                   (fewesttopi * nmidnet + fewesttopi) * HFMSP,
		   titleboxhght + (invfewesttopj * nmidnet + invfewesttopj) *
		                                  ntopnet * net[modi].uhght +
		            (invfewesttopj * nmidnet + invfewesttopj) * HFMSP,
		   net[modi].uwidth,
		   net[modi].uhght,
		   hfmfontStruct,
		   topunits);
}


void
display_hfm_mid (topi, topj)
/* display activations of the middle level map */
     int topi, topj;			/* map indices */
{
  int modi = HFMWINMOD;
  /* invert the vertical axis, following the display convention in the book */
  int invfewesttopj = (ntopnet - 1) - fewesttopj,
    invtopj = (ntopnet - 1) - topj;

  /* display the map over the least busy bottom-level quadrangle,
     at the position corresponding to its top-level unit */
  display_mid_box (modi,
		   nmidnet,
		   net[modi].marg + (fewesttopi * nmidnet + topi) *
		                       ntopnet * net[modi].uwidth +
		                    (fewesttopi * nmidnet + topi) * HFMSP,
		   titleboxhght + (invfewesttopj * nmidnet + invtopj) *
		                            ntopnet * net[modi].uhght +
		                  (invfewesttopj * nmidnet + invtopj) * HFMSP,
		   net[modi].midwidth,
		   net[modi].midhght,
		   hfmfontStruct,
		   midunits[topi][topj]);
}


void
display_hfm_bot (topi, topj, midi, midj)
/* display activations of the bottom level map */
     int topi, topj, midi, midj;	/* map indices */
{
  int modi = HFMWINMOD;
  /* invert the vertical axis, following the display convention in the book */
  int invtopj = (ntopnet - 1) - topj,
    invmidj = (nmidnet - 1) - midj;

  display_bot_box (modi,
		   nbotnet,
		   net[modi].marg + (topi * nmidnet + midi) *
		                 ntopnet * net[modi].uwidth +
		                    (topi * nmidnet + midi) * HFMSP,
		   titleboxhght + (invtopj * nmidnet + invmidj) *
		                      ntopnet * net[modi].uhght +
		                  (invtopj * nmidnet + invmidj) * HFMSP,
		   net[modi].botwidth,
		   net[modi].bothght,
		   tracefontStruct,
		   botunits[topi][topj][midi][midj],
		   latweights[topi][topj][midi][midj]);
}


static void
display_top_box (modi, nnet, x, y, width, height, fontstruct, units)
/* display activations and labels of the top-level units */
     int modi, nnet,			   /* hfm module number, size */
     x, y,				   /* top left of the map display */
     width, height;			   /* size of the top-level unit */
     XFontStruct *fontstruct;		   /* label font */
     FMUNIT units[MAXTOPNET][MAXTOPNET];   /* map units */
{
  int topi, topj;
  int invtopj;			/* inverts the vertical axis */

  for (topi = 0; topi < nnet; topi++)
    for (topj = 0; topj < nnet; topj++)
      /* only display the activation if it has changed */
      if (units[topi][topj].value != units[topi][topj].prevvalue)
	{
	  /* invert the vertical axis */
	  invtopj = (nnet - 1) - topj;
	  /* color the box representing unit according to activation */
	  frameRectangle (modi,
			  x + topi * width,
			  y + invtopj * height,
			  width,
			  height,
			  trans_to_color (units[topi][topj].value,UNITCOLORS));
	  /* display the list of labels on the box */
	  labelbox (modi,
		    x + topi * width, y + invtopj * height, width, height,
		    units[topi][topj].value, units[topi][topj].labels,
		    units[topi][topj].labelcount, fontstruct,
		    hfmfGC, hfmbGC);
	}
  XFlush (theDisplay);
}


static void
display_mid_box (modi, nnet, x, y, width, height, fontstruct, units)
/* display activations and labels of the units in a middle-level map */
     int modi, nnet,			   /* hfm module number, size */
     x, y,				   /* top left of the map display */
     width, height;			   /* size of the top-level unit */
     XFontStruct *fontstruct;		   /* label font */
     FMUNIT units[MAXTOPNET][MAXTOPNET];   /* map units */
{
  int midi, midj;
  int invmidj;			/* inverts the vertical axis */

  for (midi = 0; midi < nnet; midi++)
    for (midj = 0; midj < nnet; midj++)
      /* only display the activation if it has changed */
      if (units[midi][midj].value != units[midi][midj].prevvalue)
	{
	  /* invert the vertical axis */
	  invmidj = (nnet - 1) - midj;
	  /* color the box representing unit according to activation */
	  frameRectangle (modi,
			  x + midi * width,
			  y + invmidj * height,
			  width,
			  height,
			  trans_to_color (units[midi][midj].value,UNITCOLORS));
	  /* display the list of labels on the box */
	  labelbox (modi,
		    x + midi * width, y + invmidj * height, width, height,
		    units[midi][midj].value, units[midi][midj].labels,
		    units[midi][midj].labelcount, fontstruct,
		    hfmfGC, hfmbGC);
	}
  XFlush (theDisplay);
}


static void
display_bot_box (modi, nnet, x, y, width, height, fontstruct, units, lweights)
/* display activations, labels, and lateral weights of a bottom level map */
     int modi, nnet,			   /* hfm module number, size */
     x, y,				   /* top left of the map display */
     width, height;			   /* size of the top-level unit */
     XFontStruct *fontstruct;		   /* label font */
     FMUNIT units[MAXBOTNET][MAXBOTNET];   /* map units */
                                           /* lateral weights */
     double lweights[MAXBOTNET][MAXBOTNET][MAXBOTNET][MAXBOTNET];
{
  int i, j, lll, mmm;
  int i2, j2, lll2, mmm2;	/* flips the coordinate axes */
  double x0, x1, y0, y1, length;
  GC currGC;

  /* first display all activations before the labels; the labels are
     often larger than the boxes and we don't want to fillrectangle them over*/
  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      /* only display the activation if it has changed */
      if (units[i][j].value != units[i][j].prevvalue)
	{
	  /* flip the coordinate axes, following the convention in the book */
	  i2 = j;
	  j2 = i;
	  frameRectangle (modi, x + i2 * width, y + j2 * height, width, height,
			  trans_to_color (units[i][j].value, UNITCOLORS));
	}

  /* then display the labels */
  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      /* only display if the activation has changed */
      if (units[i][j].value != units[i][j].prevvalue)
	{
	  /* flip the coordinate axes */
	  i2 = j;
	  j2 = i;
	  labelbox (modi, x + i2 * width, y + j2 * height, width, height,
		    units[i][j].value, units[i][j].labels,
		    units[i][j].labelcount, fontstruct,
		    tracefGC, tracebGC);
	}

  /* after activations and labels, display the lateral connections over them */
  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      /* only display if the activation has changed */
      if (units[i][j].value != units[i][j].prevvalue)
	{
	  /* if activation is large, use reverse colors */
	  if (units[i][j].value > data.reversevalue)
	    currGC = linebGC;
	  else
	    currGC = linefGC;
	  /* flip the coordinate axes */
	  i2 = j;
	  j2 = i;
	  for (lll = 0; lll < nnet; lll++)
	    for (mmm = 0; mmm < nnet; mmm++)
	      /* only show the excitatory weights */
	      if (lweights[i][j][lll][mmm] > 0.0)
		{
		  /* flip the coordinate axes */
		  lll2 = mmm;
		  mmm2 = lll;
		  /* figure out start and end point of the connection */
		  x0 = x + (i2 + 0.5) * net[modi].botwidth;
		  x1 = x + (lll2 + 0.5) * net[modi].botwidth;
		  y0 = y + (j2 + 0.5) * net[modi].bothght;
		  y1 = y + (mmm2 + 0.5) * net[modi].bothght;
		  /* and its length */
		  length =sqrt ((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
		  if (length > 0.0)
		    /* draw scaling length and width according to activation */
		    drawLine (modi,
			      (int) x0,
			      (int) y0,
			      (int) (x0 + lweights[i][j][lll][mmm] /
				     gammaexc * data.tracelinescale * 0.5 *
				     net[modi].botwidth * (x1 - x0) / length),
			      (int) (y0 + lweights[i][j][lll][mmm] /
				     gammaexc * data.tracelinescale * 0.5 *
				     net[modi].bothght * (y1 - y0) / length),
			      currGC,
			      (int) (lweights[i][j][lll][mmm] *
				     data.tracewidthscale * net[modi].width));
		}
	}
  XFlush (theDisplay);
}


void
display_hfm_error (modi, image)
  /* write out the error in the map representation */
     int modi;			/* module number */
     IMAGEDATA image;		/* image unit indices and trace activation */
{
  char bunitlabel[MAXSTRL + 1];	/* label on the trace image unit */
  int i, j;
  double sum = 0.0;

  if (modi == RETMOD && image.value < aliveact)
    /* nothing was retrieved */
    sprintf (net[HFMWINMOD].log, "No image found");
  else
    {
      /* indicate whether storage or retrieval */
      if (modi == STOREMOD)
	sprintf (net[HFMWINMOD].log, "S");
      else
	sprintf (net[HFMWINMOD].log, "R");
      /* display the indices if there is no label on the trace image unit */
      if (botunits[image.topi][image.topj]
	          [image.midi][image.midj]
	          [image.boti][image.botj].labelcount == 0)
	sprintf (bunitlabel, "(%d,%d)", image.boti, image.botj);
      else
	/* otherwise display the first label in the list */
	sprintf (bunitlabel, "%s",
		 botunits[image.topi][image.topj]
		         [image.midi][image.midj]
		         [image.boti][image.botj].labels[0]);
      /* compose the log message from all 3 image unit labels */
      sprintf (net[HFMWINMOD].log, "%s %s-%s-%s", net[HFMWINMOD].log,
	       topunits[image.topi][image.topj].labels[0],
	       midunits[image.topi][image.topj]
	               [image.midi][image.midj].labels[0],
	       bunitlabel);

      /* calculate the error in the map representation */
      for (i = 0; i < noutputs[HFMWINMOD]; i++)
	for (j = 0; j < nsrep; j++)
	  sum += fabs (swords[targets[modi][i]].rep[j] -
		       outrep[modi][i * nsrep + j]);

      sprintf (net[HFMWINMOD].log, "%s: Eavg %.3f",
	       net[HFMWINMOD].log, sum / noutrep[HFMWINMOD]);
    }
  display_log (HFMWINMOD);
}


/********************* lexicon operations ***********************/

static void
init_lex_display_params (modi, w, nnet, nwords, words, nrep, units)
/* assign labels for the image units of each word, clear the values */
     int modi;				/* module number */
     Widget w;				/* and its widget */
     int nnet,				/* map size */
     nwords;				/* number of words */
     WORDSTRUCT words[];		/* lexicon (lexical or semantic) */
     int nrep;				/* dimension of the word rep */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
{
  int i, j, k;				/* map and word indices */
  int besti, bestj;			/* indices of the image unit */
  double best, foo;			/* best and worst response found */

  Win[modi] = XtWindow (w);		/* get a pointer to the window */

  /* clear possible previous labels */
  lex_clear_labels (units, nnet);

  /* assign labels to maximally responding units for each word */
  for (k = 0; k < nwords; k++)
    {
      /* find the max responding unit */
      best = foo = LARGEFLOAT;
      for (i = 0; i < nnet; i++)
	for (j = 0; j < nnet; j++)
	  {
	    /* response proportional to distance of weight and input vectors */
	    distanceresponse (&units[i][j], words[k].rep, nrep, distance,NULL);
	    /* find best and worst and best indices */
	    updatebestworst (&best, &foo, &besti, &bestj, &units[i][j], i, j,
			     fsmaller, fgreater);
	  }
      /* update the label list of that unit */
      collect_labels (units[besti][bestj].labels,
		      &units[besti][bestj].labelcount,
		      words[k].chars, MAXFMLABELS);
    }

  /* clear the activations */
  lex_clear_values (units, nnet);
  lex_clear_prevvalues (units, nnet);
}


void
display_new_lex_labels (modi, nnet, nwords, words, nrep, units)
/* read the labels and clear the map display (for later redisplay) */
/* called in main.c after repfile changes */
     int modi;				/* module number */
     int nnet,				/* map size */
     nwords;				/* number of words */
     WORDSTRUCT words[];		/* lexicon (lexical or semantic) */
     int nrep;				/* dimension of the word rep */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
{
  Widget w = ((modi == LEXWINMOD) ? lex : sem);

  init_lex_display_params (modi, w, nnet, nwords, words, nrep, units);
  XClearArea (theDisplay, Win[modi], 0, 0, 0, 0, True);
  XFlush (theDisplay);
}


/********************* expose event handler */
static void
expose_lex (w, units, call_data)
  /* expose event handler for a lexicon map: redraw the window */
  /* standard callback parameters; widget is actually used here */
     Widget w; XtPointer units, call_data;
{
  /* figure out the module number and size from the widget given */
  int
    modi = ((w == lex) ? LEXWINMOD : SEMWINMOD),
    nnet = ((w == lex) ? nlnet : nsnet);

  XClearWindow (theDisplay, Win[modi]);
  display_title (modi, titles[modi]);
  lex_clear_prevvalues (units, nnet);
  display_lex (modi, units, nnet);
  display_log (modi);
}


/********************* resizing */
static void
resize_lex (w, client_data, call_data)
  /* resize event handler for a lexicon map: recalculate geometry */
  /* standard callback parameters; widget is actually used here */
     Widget w; XtPointer client_data, call_data;
{
  /* figure out the module number and size from the widget given */
  int
    modi = ((w == lex) ? LEXWINMOD : SEMWINMOD),
    nnet = ((w == lex) ? nlnet : nsnet);

  common_resize (modi, w);	/* get new window size */

  /* height and width of the boxes representing unit activities */
  net[modi].uhght = (net[modi].height - titleboxhght - VERSP) / nnet;
  net[modi].uwidth = (net[modi].width - 2 * HORSP) / nnet;

  /* horizontal margin from the edge of the window */
  net[modi].marg = (net[modi].width - nnet * net[modi].uwidth) / 2;
}


/*********************  mouse button press handler  */
static void
lexsemmouse_handler (w, client_data, p_event)
/* handles the button press events from the lexical and semantic map */
     Widget w;
     XtPointer client_data;
     XEvent *p_event;
{
  int x = p_event->xbutton.x,		/* mouse coordinates */
      y = p_event->xbutton.y;
  
  /* exit if mouseevents are not currently allowed */
  if (nolexmouseevents)
    return;
  
  if (w == lex)
    display_assocweights (LEXWINMOD, lunits, nlnet, lwords, nlrep, nlwords,
			  SEMWINMOD, sunits, nsnet, swords, nsrep, nswords,
			  x, y, lsassoc);
  else
    display_assocweights (SEMWINMOD, sunits, nsnet, swords, nsrep, nswords,
			  LEXWINMOD, lunits, nlnet, lwords, nlrep, nlwords,
			  x, y, slassoc);
}


static void
display_assocweights (srcmodi, srcunits, nsrcnet, srcwords, nsrcrep, nsrcwords,
		      tgtmodi, tgtunits, ntgtnet, tgtwords, ntgtrep, ntgtwords,
		      x, y, assoc)
/* display the associative weights of the unit clicked, and show the label
   of the word rep in the lexicon nearest to the image unit */
   int srcmodi, nsrcnet,		/* source module and map size */
     nsrcrep, nsrcwords,		/* source wordrepsize and #of words */
     tgtmodi, ntgtnet,			/* target module and map size */
     ntgtrep, ntgtwords,		/* target wordrepsize and #of words */
     x, y;				/* location of the mouse click */
   WORDSTRUCT *srcwords, *tgtwords;	/* source and target lexicon */
   FMUNIT srcunits[MAXLSNET][MAXLSNET], 		 /* source map */
     tgtunits[MAXLSNET][MAXLSNET];	    		 /* target map */
   double assoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET]; /* assoc weights */
   
{
  int i, j;				/* weight index counter */
  int uniti, unitj;			/* weights of this unit are shown */
  int besti, bestj;			/* indices of the image unit */
  double best = (-1), foo = (-1);	/* best and worst response found */
  
  /* only accept points within the network display */
  if (x >= net[srcmodi].marg && y >= titleboxhght &&
      x <= net[srcmodi].marg + nlnet * net[srcmodi].uwidth && 
      y <= titleboxhght + nlnet * net[srcmodi].uhght)
    {
      /* calculate which unit is requested */
      /* the indices are reversed in discern display */
      unitj = (x - net[srcmodi].marg) / net[srcmodi].uwidth;
      uniti = (y - titleboxhght) / net[srcmodi].uhght;

      /* turn it and it alone on in the source map */
      for (i = 0; i < nsrcnet; i++)
	for (j = 0; j < nsrcnet; j++)
	  {
	    srcunits[i][j].prevvalue = srcunits[i][j].value;
	    srcunits[i][j].value = 0.0;
	  }
      srcunits[uniti][unitj].value = 1.0;
      display_lex (srcmodi, srcunits, nsrcnet);
      
      /* find and display the label of the closest word in the lexicon */
      sprintf (net[srcmodi].log, "Source unit: %s",
	       srcwords[find_nearest (srcunits[uniti][unitj].comp,
				      srcwords, nsrcrep, nsrcwords)].chars);
      display_log (srcmodi);

      /* establish values to be displayed as activations on the assoc map */
      /* find and display the label of the closest word in the lexicon */
      for (i = 0; i < ntgtnet; i++)
	for (j = 0; j < ntgtnet; j++)
	  {
	    tgtunits[i][j].prevvalue = tgtunits[i][j].value;
	    tgtunits[i][j].value = assoc[uniti][unitj][i][j];
	    updatebestworst (&best, &foo, &besti, &bestj, &tgtunits[i][j],
			     i, j, fgreater, fsmaller);
	  }
      display_lex (tgtmodi, tgtunits, ntgtnet);
      sprintf (net[tgtmodi].log, "Assoc weights: %s",
	       tgtwords[find_nearest (tgtunits[besti][bestj].comp,
				      tgtwords, ntgtrep, ntgtwords)].chars);
      display_log (tgtmodi);
    }
}


/********************* lexicon display subroutines */
void
display_lex (modi, units, nnet)
/* display the map activations */
     int modi,				/* module number */
     nnet;				/* map size */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
{
  int i, j, jj;
  XFontStruct *fontstruct;		/* label font */
  GC currfGC, currbGC;			/* dark and light label color */

  /* both semantic input and output go to the same window */
  if (modi == SINPMOD || modi == SOUTMOD)
    modi = SEMWINMOD;
  /* both lexical input and output go to the same window */
  if (modi == LINPMOD || modi == LOUTMOD)
    modi = LEXWINMOD;

  /* lexical and semantic fonts and colors could be different */
  if (modi == LEXWINMOD)
    {
      fontstruct = lexfontStruct;
      currfGC = lexfGC;
      currbGC = lexbGC;
    }
  else
    {
      fontstruct = semfontStruct;
      currfGC = semfGC;
      currbGC = sembGC;
    }

  /* i and j are switched to match the figure in the book */
  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      /* only display if the activation has changed */
      if (units[i][j].value != units[i][j].prevvalue)
	{
	  /* color the box representing unit according to activation */
	  frameRectangle (modi, net[modi].marg + j * net[modi].uwidth,
			  titleboxhght + i * net[modi].uhght,
			  net[modi].uwidth, net[modi].uhght,
			  trans_to_color (units[i][j].value, UNITCOLORS));
	  /* display the labels in both horizontally adjacent units as well
	     because they might be larger than their box and were just
	     wiped out by coloring the box */
	  for (jj = j - 1; jj <= j + 1; jj++)
	    if (jj >= 0 && jj < nnet)
	      /* display the list of labels on the box */
	      labelbox (modi, net[modi].marg + jj * net[modi].uwidth,
			titleboxhght + i * net[modi].uhght,
			net[modi].uwidth, net[modi].uhght,
			units[i][jj].value, units[i][jj].labels,
			units[i][jj].labelcount, fontstruct,
			currfGC, currbGC);
	}
  XFlush (theDisplay);
}


void
display_lex_error (modi)
  /* write out the error in the map representation */
  int modi;			/* module number */
{
  int i, nearest,
  winmodi = modi;		/* window module number */
  double sum = 0.0;		/* total error */
  WORDSTRUCT *words;		/* lexicon (lexical or semantic) */
  int nrep;			/* rep dimension (lexical or semantic) */
  int nwords;			/* number of words (lexical or semantic) */

  /* first select the right lexicon and establish the output window */
  winmodi = select_lexicon (modi, &words, &nrep, &nwords);

  /* first figure out whether this is input map or output (associative) map */
  if (modi == LINPMOD || modi == SINPMOD)
    sprintf (net[winmodi].log, "Input");
  else
    sprintf (net[winmodi].log, "Assoc");

  /* calculate the representation error */
  for (i = 0; i < nrep; i++)
    sum += fabs (words[targets[modi][0]].rep[i] - outrep[modi][i]);

  /* find out whether the word is correct */
  nearest = find_nearest (outrep[modi], words, nrep, nwords);
  if (nearest == targets[modi][0] || text_question)
    sprintf (net[winmodi].log, "%s %s: Eavg %.3f",
	     net[winmodi].log, words[nearest].chars, sum / nrep);
  else
    /* if not, display the correct word as well */
    sprintf (net[winmodi].log, "%s *%s(%s)*: Eavg %.3f",
	     net[winmodi].log,
	     words[nearest].chars, words[targets[modi][0]].chars,
	     sum / nrep);

  display_log (winmodi);
}


/********************* general routines ***********************/

static void
display_title (modi, name)
/* write the name of the module on top center of the graphics window */
     int modi;				/* module number */
     char name[];			/* module name string */
{
  drawText (modi,
  (net[modi].width - XTextWidth (titlefontStruct, name, strlen (name))) / 2,
	    BOXSP + titlefontStruct->ascent,
	    name, titleGC);
  XFlush (theDisplay);
}


static void
display_log (modi)
/* write the log line on top left of the graphics window */
     int modi;				/* module number */
{ 
  clearRectangle (modi, 0, 0, net[modi].width, titleboxhght);
  drawText (modi, net[modi].marg, BOXSP + titlefontStruct->ascent,
	    net[modi].log, logGC);
  /* if we did not overwrite the title, rewrite it */
  if (net[modi].marg +
      XTextWidth (logfontStruct, net[modi].log, strlen (net[modi].log)) <
      (net[modi].width -
       XTextWidth (titlefontStruct, titles[modi], strlen (titles[modi]))) / 2)
    display_title (modi, titles[modi]);
}


static void
frameRectangle (modi, x, y, width, height, colorindex)
/* draw a filled rectangle with a given color and draw a frame around it */
     int modi,				/* module number */
     x, y,				/* top left of the rectangle */
     width, height,			/* dimensions of the rectangle */
     colorindex;			/* fill color */
{
  fillRectangle (modi, x, y, width, height, colorindex);
  drawRectangle (modi, x, y, width, height);
}


static void
labelbox (modi, x, y, width, height, value, labels, labelcount, fontstruct,
	  fGC, bGC)
/* write the label list on the box representing feature map unit */
     int modi,				/* module number */
     x, y,				/* top left of the box */
     width, height;			/* dimensions of the box */
     double value;			/* activation of the unit */
     char labels[][MAXWORDL + 1];	/* label list */
     int labelcount;			/* number of labels on the list */
     XFontStruct *fontstruct;		/* label font */
     GC fGC, bGC;			/* dark and light colors */
{
  int k;				/* label counter */
  GC currGC;				/* label color */

  /* if activation is large, use reverse colors */
  if (value > data.reversevalue)
    currGC = bGC;
  else
    currGC = fGC;
  /* write each label in the list, one after another in the box */
  for (k = 0; k < labelcount; k++)
    drawoverText (modi,
		  (int) (x + 0.5 * (width - XTextWidth (fontstruct,
							labels[k],
							strlen (labels[k])))),
		  (int) (y + 0.5 * height + (k + 1 - 0.5 * labelcount) *
			 fontstruct->ascent),
		  labels[k],
		  currGC);
}


static void
collect_uniq_labels (labels, count, label, maxlabels)
/* if the label is not in the list and there is still room, add it to list */
     char labels[][MAXWORDL + 1];	/* label list */
     int *count;			/* number of labels */
     char label[];			/* label string */
     int maxlabels;			/* max number of labels that fit */
{
  if (newlabel (labels, *count, label))
    collect_labels (labels, count, label, maxlabels);
}


static void
collect_labels (labels, count, label, maxlabels)
/* if there is still room in the list, add the label to it */
     char labels[][MAXWORDL + 1];	/* label list */
     int *count;			/* number of labels */
     char label[];			/* label string */
     int maxlabels;			/* max number of labels that fit */
{
  /* always display maxfmlabels labels, even if they don't fit the box */
  if (*count == MAXFMLABELS)
    /* list is full; make the last label MORELABEL */
    sprintf (labels[(*count) - 1], MORELABEL);
  else
    /* otherwise add it to the list and increment labelcount */
    sprintf (labels[(*count)++], label);
}


static int
newlabel (labels, count, label)
/* check if the label is already in the list of labels for the unit */
     char labels[][MAXWORDL + 1];	/* label list */
     int count;				/* number of labels */
     char label[];			/* label string */
{
  int i;

  for (i = 0; i < count; i++)
    if (!strcmp (labels[i], label))
      return (FALSE);
  return (TRUE);
}


static int
trans_to_color (value, map)
/* translate an activation value into color index */
/* only unitcolors supported in this version */
     double value;			/* activation */
     int map;				/* selects a colormap */
{
  if (map == UNITCOLORS)
    /* map the number [0,1] to corresponding color */
    return ((int) (((actual_color_range - 1) * value) + 0.499999));
  else
    {
      fprintf (stderr, "Wrong colorscale (only unitcolors supported)\n");
      exit (EXIT_X_ERROR);
    }
}


/********************* low-level operations ***********************/

static void
drawLine (modi, x1, y1, x2, y2, currGC, width)
/* draw a line according to given graphics context */
     int modi;
     int x1, y1, x2, y2;
     GC currGC;
     int width;
{
  XSetLineAttributes (theDisplay, currGC, width,
		      LineSolid, CapRound, JoinRound);
  XDrawLine (theDisplay, Win[modi], currGC, x1, y1, x2, y2);
}


static void
clearRectangle (modi, x, y, width, height)
/* draw a rectangle in the background color */
     int modi, x, y, width, height;
{
  XFillRectangle (theDisplay, Win[modi], clearGC, x, y, width, height);
}


static void
fillRectangle (modi, x, y, width, height, colorindex)
/* draw a filled rectangle in given color */
     int modi, x, y, width, height, colorindex;
{
  XSetForeground (theDisplay, activityGC, colors[cmap[colorindex]].pixel);
  XFillRectangle (theDisplay, Win[modi], activityGC, x, y, width, height);
}


static void
drawRectangle (modi, x, y, width, height)
/* draw a rectangle in the box color */
     int modi, x, y, width, height;
{
  XDrawRectangle (theDisplay, Win[modi], boxGC, x, y, width, height);
}


static void
drawText (modi, x, y, text, currGC)
/* draw text image according to given graphics context */
     int modi, x, y;
     char text[];
     GC currGC;
{
  XDrawImageString (theDisplay, Win[modi], currGC, x, y, text, strlen (text));
}


static void
drawoverText (modi, x, y, text, currGC)
/* overwrite text on screen according to give graphics context */
     int modi, x, y;
     char text[];
     GC currGC;
{
  XDrawString (theDisplay, Win[modi], currGC, x, y, text, strlen (text));
}
