/* File: main.c
 *
 * Main story processing loop, initializations, and statistics for DISCERN.
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: main.c,v 1.62 1994/09/20 10:46:59 risto Exp $
 */

#include <stdio.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Cardinals.h>

#include "defs.h"
#define DEFINE_GLOBALS		/* so global variables get defined here only */
#include "globals.c"


/************ general constants *************/

/* commands */
#define C_READ "read"
#define C_READ_AND_PARAPHRASE "read-and-paraphrase"
#define C_QUESTION "question"
#define C_TEXT_QUESTION "text-question"
#define C_CLEAR_NETWORKS "clear-networks"
#define C_STOP "stop"
#define C_FILE "file"
#define C_ECHO "echo"
#define C_INIT_STATS "init-stats"
#define C_PRINT_STATS "print-stats"
#define C_LIST_PARAMS "list-params"
#define C_QUIT "quit"
#define C_LREPFILE "lrepfile"
#define C_SREPFILE "srepfile"
#define C_PROCFILE "procfile"
#define C_HFMFILE "hfmfile"
#define C_HFMINPFILE "hfminpfile"
#define C_LEXFILE "lexfile"
#define C_DISPLAYING "displaying"
#define C_NSLOT "nslot"
#define C_NCASE "ncase"
#define C_CHAIN "chain"
#define C_WITHLEX "withlex"
#define C_WITHHFM "withhfm"
#define C_BABBLING "babbling"
#define C_PRINT_MISTAKES "print_mistakes"
#define C_LOG_LEXICON "log_lexicon"
#define C_IGNORE_STOPS "ignore_stops"
#define C_DELAY "delay"
#define C_TOPSEARCH "topsearch"
#define C_MIDSEARCH "midsearch"
#define C_TRACENC "tracenc"
#define C_TSETTLE "tsettle"
#define C_EPSILON "epsilon"
#define C_ALIVEACT "aliveact"
#define C_MINACT "minact"
#define C_MAXACT "maxact"
#define C_GAMMAEXC "gammaexc"
#define C_GAMMAINH "gammainh"
#define C_WITHINERR "withinerr"
#define C_COMMENT_CHAR '#'

/* keywords expected in the input files */
#define SENTDELIM "."			/* sentence delimiter in input files */
#define QUESTDELIM "?"			/* question delimiter in input files */
#define BLANKLABEL "_"			/* blank rep symbol in input files */
#define REPS_INST "instances"		/* reps instance list */
#define PROCSIMU_REPS "word-representations"	/* reps, procsimu word rep */
#define PROCSIMU_REPSIZE "nwordrep"	/* reps size */

/* option keywords */
#define OPT_HELP "-help"
#define OPT_GRAPHICS "-graphics"
#define OPT_NOGRAPHICS "-nographics"
#define OPT_OWNCMAP "-owncmap"
#define OPT_NOOWNCMAP "-noowncmap"
#define OPT_DELAY "-delay"

/* other name defaults */
#define APP_CLASS "Discern"		/* class of this application */
#define DEFAULT_INITFILENAME "init"	/* initialization file */
#define DEFAULT_INPUTFILENAME "input-example" /* input file */
#define SEMANTIC_KEYWORD "semantic"	/* name of semantic reps */
#define LEXICAL_KEYWORD "lexical"	/* name of lexical reps */

/******************** Function prototypes ************************/

/* global functions */
#include "prototypes.h"
extern int strcasecmp __P ((const char *s1, const char *s2));

/* functions local to this file */
static void create_toplevel_widget __P ((int argc, char **argv));
static void process_remaining_arguments __P ((int argc, char **argv));
static void process_display_options __P ((int argc, char **argv));
static void process_nodisplay_options __P ((int *argc, char **argv));
static char *get_option __P ((XrmDatabase db,
			      char *app_name, char *app_class,
			      char *res_name, char *res_class));
static void usage __P ((char *app_name));
static void loop_stories __P((void));
static void process_inpfile __P((FILE *fp));
static void init_system __P((void));
static int system_initialized __P((char *commandstr));
static void reps_init __P((char *lexsem, char *repfile, WORDSTRUCT words[],
			   int *nwords, int *nrep));
static void read_story_data __P((FILE *fp));
static void read_qa_data __P((FILE *fp));
static void read_text_question __P((char *rest));
static void list_params __P((void));
static void sent2indices __P ((SENTSTRUCT *sent, char rest[]));
static int text2floats __P((double itemarray[], int nitems, char nums[]));
static int wordindex __P((char wordstring[]));
static void read_int_par __P((char *rest, char *commandstr, int *variable));
static void read_float_par __P((char *rest, char *commandstr,
				double *variable));
static char *rid_sspace __P((char rest[]));


/******************** static variables ******************** */

/* space for the lexica (including blank word -1) */
static WORDSTRUCT
   lwordsarray[MAXWORDS + 1],
   swordsarray[MAXWORDS + 1];

/* define the geometry of the display */
static String fallback_resources[] =
{
  "*runstop.left: 	ChainLeft",
  "*runstop.right: 	ChainLeft",
  "*runstop.top:	ChainTop",
  "*runstop.bottom:	ChainTop",
  "*step.fromHoriz: 	runstop",
  "*step.left:	 	ChainLeft",
  "*step.right: 	ChainLeft",
  "*step.top:		ChainTop",
  "*step.bottom:	ChainTop",
  "*clear.fromHoriz: 	step",
  "*clear.left: 	ChainLeft",
  "*clear.right: 	ChainLeft",
  "*clear.top:		ChainTop",
  "*clear.bottom:	ChainTop",
  "*quit.fromHoriz: 	clear",
  "*quit.left:	 	ChainLeft",
  "*quit.right: 	ChainLeft",
  "*quit.top:		ChainTop",
  "*quit.bottom:	ChainTop",
  "*command.fromHoriz: 	quit",
  "*command.left: 	ChainLeft",
  "*command.right: 	ChainRight",
  "*command.top:	ChainTop",
  "*command.bottom:	ChainTop",
  "*sentpars.fromVert: 	runstop",
  "*sentpars.top:	ChainTop",
  "*storypars.fromVert:	sentpars",
  "*cueformer.fromVert:	storypars",
  "*lex.fromVert: 	cueformer",
  "*sem.fromVert: 	lex",
  "*sentgen.fromHoriz:	sentpars",
  "*sentgen.fromVert:	runstop",
  "*sentgen.top:	ChainTop",
  "*storygen.fromHoriz:	storypars",
  "*storygen.fromVert:	sentgen",
  "*answerprod.fromHoriz: cueformer",
  "*answerprod.fromVert: storygen",
  "*hfm.fromHoriz: 	lex",
  "*hfm.fromVert: 	answerprod",

  /* define the color defaults */
  "*foreground:	        white",
  "*background:		black",
  "*borderColor:	white",

  /* commands can be entered by hitting return */
  "*command*translations: #override\\n\
	<Key>Return: read_command()",
  "*command*editType:	edit",
  NULL
};

/* these are the possible command line options */
static XrmOptionDescRec options[] = 
{
  {OPT_HELP, ".help", XrmoptionNoArg, "true"},
  {OPT_GRAPHICS, ".bringupDisplay", XrmoptionNoArg, "true"},
  {OPT_NOGRAPHICS, ".bringupDisplay", XrmoptionNoArg, "false"},
  {OPT_OWNCMAP, ".owncmap", XrmoptionNoArg, "true"},
  {OPT_NOOWNCMAP, ".owncmap", XrmoptionNoArg, "false"},
  {OPT_DELAY, ".delay", XrmoptionSepArg, NULL},
};

/* the default values for the application-specific resources;
   see defs.h for component descriptions */
static XtResource resources[] =
{
  {"bringupDisplay", "BringupDisplay", XtRBoolean, sizeof (Boolean),
   XtOffset (RESOURCE_DATA_PTR, bringupdisplay), XtRImmediate,
   (XtPointer) True},
  {"owncmap", "Owncmap", XtRBoolean, sizeof (Boolean),
   XtOffset (RESOURCE_DATA_PTR, owncmap), XtRImmediate, (XtPointer) False},
  {"delay", "Delay", XtRInt, sizeof (int),
   XtOffset (RESOURCE_DATA_PTR, delay), XtRString, "0"},

  {"netwidth", "Netwidth", XtRDimension, sizeof (Dimension),
   XtOffset (RESOURCE_DATA_PTR, netwidth), XtRString, "512"},
  {"procnetheight", "Procnetheight", XtRDimension, sizeof (Dimension),
   XtOffset (RESOURCE_DATA_PTR, procnetheight), XtRString, "140"},
  {"hfmnetheight", "Hfmnetheight", XtRDimension, sizeof (Dimension),
   XtOffset (RESOURCE_DATA_PTR, hfmnetheight), XtRString, "514"},
  {"lexnetheight", "Lexnetheight", XtRDimension, sizeof (Dimension),
   XtOffset (RESOURCE_DATA_PTR, lexnetheight), XtRString, "255"},

  {"tracelinescale", "Tracelinescale", XtRFloat, sizeof (float),
   XtOffset (RESOURCE_DATA_PTR, tracelinescale), XtRString, "1.5"},
  {"tracewidthscale", "Tracewidthscale", XtRFloat, sizeof (float),
   XtOffset (RESOURCE_DATA_PTR, tracewidthscale), XtRString, "0.01"},
  {"reversevalue", "Reversevalue", XtRFloat, sizeof (float),
   XtOffset (RESOURCE_DATA_PTR, reversevalue), XtRString, "0.3"},

  {"textColor", "TextColor", XtRPixel, sizeof (Pixel),
   XtOffset (RESOURCE_DATA_PTR, textColor), XtRString, "green"},
  {"netColor", "NetColor", XtRPixel, sizeof (Pixel),
   XtOffset (RESOURCE_DATA_PTR, netColor), XtRString, "red"},
  {"latweightColor", "LatweightColor", XtRPixel, sizeof (Pixel),
     XtOffset (RESOURCE_DATA_PTR, latweightColor),  XtRString, "white"},

  {"commandfont", "Commandfont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, commandfont), XtRString, "7x13bold"},
  {"titlefont", "Titlefont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, titlefont), XtRString, "8x13bold"},
  {"logfont", "Logfont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, logfont), XtRString, "6x10"},
  {"asmfont", "Asmfont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, asmfont), XtRString, "6x10"},
  {"asmerrorfont", "Asmerrorfont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, asmerrorfont), XtRString, "5x8"},
  {"hfmfont", "Hfmfont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, hfmfont), XtRString, "7x13"},
  {"tracefont", "Tracefont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, tracefont), XtRString, "5x8"},
  {"lexfont", "Lexfont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, lexfont), XtRString, "5x8"},
  {"semfont", "Semfont", XtRString, sizeof (String),
   XtOffset (RESOURCE_DATA_PTR, semfont), XtRString, "5x8"},

  /* command line options */
  {"help", "Help", XtRBoolean, sizeof (Boolean),
   XtOffset (RESOURCE_DATA_PTR, help), XtRImmediate, (XtPointer) False},
};

/* interrupt handling */

/* for AIX ^C handling, we need to use setsig instead of signal */
/* and call sigrelse before longjmp (see below) */
#if defined(_AIX) || defined(___AIX)
extern void sigset __P ((int Signal, void (*Function) ()));
#define signal(a,b) sigset(a,b)
extern int sigrelse __P ((int Signal));
#endif

jmp_buf loop_stories_env;	/* jump here from after interrupt */
void sig_handler ()
/* handles control-c by jumping to loop-stories like clear-networks */
{
#if defined(_AIX) || defined(___AIX)
  sigrelse (SIGINT);
#endif
  longjmp (loop_stories_env, 1);
}


/*********************  main command processing ******************************/

void
main (argc, argv)
/* initialize X display, system parameters, and start the command loop */
     int argc;
     char **argv;
{
  int xargc;			/* saved argc & argv */
  char **xargv;
  int i;

  /* save command line args so we can parse them later */
  xargc = argc;
  xargv = (char **) XtMalloc (argc * sizeof (char *));
  for (i = 0; i < argc; i++)
    xargv[i] = argv[i];

  /* try to open the display */
  XtToolkitInitialize ();
  app_con = XtCreateApplicationContext ();
  XtAppSetFallbackResources (app_con, fallback_resources);
  theDisplay = XtOpenDisplay (app_con, NULL, NULL, APP_CLASS,
			      options, XtNumber (options), &argc, argv);
  if (theDisplay != NULL)
    /* create the top-level widget and get application resources */
    create_toplevel_widget(xargc, xargv);
  else
    fprintf (stderr, "No display: running in text mode.\n");

  process_remaining_arguments (argc, argv);
  init_system ();
  loop_stories ();
  exit (EXIT_NORMAL);
}


static void
loop_stories ()
/* process commands from a file or from the keyboard */
{
  if (setjmp (loop_stories_env))
    {
      /* longjmp gets here */
    }
  else
    {
      /* return from setjmp */
    }
  printf ("%s", promptstr);
  if (displaying)
    while (TRUE)
      {
	/* if the Xdisplay is up, process events until the user hits "Run" */
	/* user hitting "Quit" will terminate the program */
	wait_for_run ();
	/* user hit "Run"; start reading commands from current input file */
	process_command (NULL, "file", current_inpfile);
      }
  else
    /* no Xdisplay; read commands from the keyboard */
    process_inpfile (stdin);
}


static void
process_inpfile (fp)
/* read commands from an input file */
     FILE *fp;			/* pointer to the input file */
{
  int c;			/* temporarily holds one char */
  char 
    commandstr[MAXSTRL + 1],	/* command string */
    rest[MAXSTRL + 1];		/* parameters */

  while ((c = fgetc (fp)) != EOF)
    {
      /* reprint the prompt if return from the keyboard */
      if (c == '\n' && fp == stdin)
	{
	  printf ("%s", promptstr);
	  continue;
	}
      /* otherwise ignore white space */
      if (c == ' ' || c == '\t' || c == '\n')
	continue;
      /* there is real input; process it as a command */
      ungetc (c, fp);
      fscanf (fp, "%s", commandstr);
      fgetline (fp, rest, MAXSTRL);
      process_command (fp, commandstr, rest);
    }
}


void
process_command (fp, commandstr, rest)
/* process one command */
     FILE *fp;		/* file where command was read from; NULL=display */
     char *commandstr, *rest;	/* command and its parameters */
{
  double bestvalue, foo;	/* activation of the max responding unit */
  char s[MAXSTRL + 1], ss[MAXSTRL + 1];
  FILE *fp2;			/* pointer to a new input file */

  if (!system_initialized (commandstr))
    {
      fprintf (stderr, "Initialization incomplete or out of order\n");
      exit (EXIT_DATA_ERROR);
    }

  /* these commands are meaningful only in files and from stdin */
  else if (fp == NULL && (!strcmp (commandstr, C_READ) ||
		     !strcmp (commandstr, C_READ_AND_PARAPHRASE) ||
		     !strcmp (commandstr, C_QUESTION)))
    {
      sprintf (s, "command %s ignored in display mode, sorry", commandstr);
      printcomment ("", s, "\n");
    }
  else if (!strcmp (commandstr, C_READ))
    /* read in a story, maybe store in the memory, do not paraphrase */
    {
      printcomment ("", "Read story", "\n");
      read_story_data (fp);	/* read in the story data */
      parse_story ();		/* process it through the networks */
      if (withhfm)
	/* store in the memory */
	presentmem (PARATASK, STOREMOD, story.slots, &foo);
      if (babbling)
	putchar ('\n');
    }
  else if (!strcmp (commandstr, C_READ_AND_PARAPHRASE))
    /* read in a story, maybe store in the memory, paraphrase */
    {
      printcomment ("", "Read and paraphrase story", "\n");
      read_story_data (fp);	/* read in the story data */
      parse_story ();		/* process it through the networks */
      if (withhfm)
	/* store in the memory */
	presentmem (PARATASK, STOREMOD, story.slots, &foo);
      gener_story ();		/* generate a paraphrase */
      if (babbling)
	putchar ('\n');
    }
  else if (!strcmp (commandstr, C_QUESTION))
    /* parse and answer a question */
    {
      printcomment ("", "Answer question", "\n");
      read_qa_data (fp);	/* read in the question/answer data */
      formcue ();		/* parse the question and form cue */
      if (withhfm)
	/* retrieve the appropriate story */
	presentmem (QATASK, RETMOD, qa.slots, &bestvalue);
      if (!(withhfm && bestvalue < aliveact))	/* if one found */
	produceanswer ();	/* form an answer and output it */
      if (babbling)
	putchar ('\n');
    }
  else if (!strcmp (commandstr, C_TEXT_QUESTION))
    /* process a question that the user typed in */
    {
      printcomment ("", "Answer text question", "\n");
      text_question = TRUE;	/* answer processing done without a target */
      read_text_question (rest);/* read in the question data */
      formcue ();		/* parse the question and form cue */
      if (withhfm)
	/* retrieve the appropriate story */
	presentmem (QATASK, RETMOD, qa.slots, &bestvalue);
      if (!(withhfm && bestvalue < aliveact))	/* if one found */
	produceanswer ();	/* form an answer and output it */
      text_question = FALSE;
      if (babbling)
	putchar ('\n');
    }
  else if (!strcmp (commandstr, C_CLEAR_NETWORKS))
    /* clear the traces and clean the screen */
    {
      printcomment ("", "Clearing networks", "\n");
      if (displaying)
	clear_networks_display ();	/* this will clear traces and more */
      else
	clear_traces ();
    }
  else if (!strcmp (commandstr, C_STOP))
    /* stop the simulation and wait for "Run" or return */
    {
      if (!ignore_stops)	/* stopping can be turned off */
	{
	  if (displaying)
	    printcomment ("", "Stopping (click Run to continue)", "\n");
	  else
	    printcomment ("", "Stopping (hit Return to continue)", "");
	  if (displaying)
	    wait_for_run ();	/* wait for "Run" click */
	  else
	    while (getchar () != '\n');
	}
      else
	printcomment ("", "Stop command ignored", "\n");
    }
  else if (!strcmp (commandstr, C_FILE))
    /* read commands from a file */
    {
      sscanf (rest, "%s", s);	/* read the filename */
      if (open_file (s, "r", &fp2, NOT_REQUIRED)) /* if open succeeeds */
	{
	  if (fp == NULL || fp == stdin) /* currently reading commands */
	    {			/*  from the Xdisplay or keyboard */
	      if (displaying)
		start_running ();	          /* change from interactive */
	      sprintf (current_inpfile, "%s", s); /* into reading from file */
	    }
	  sprintf (ss, "Reading input from %s...", s);
	  printcomment ("", ss, "\n");
	  process_inpfile (fp2);	/* process commands from the file */
	  fclose (fp2);
	  sprintf (ss, "Finished reading input from %s.", s);
	  printcomment ("", ss, "\n");
	  if (displaying && fp == NULL)
	    simulator_running = FALSE;	/* stay in the event handler */
	}
    }
  else if (!strcmp (commandstr, C_ECHO))
    /* print text on log output */
    {
      if (babbling || print_mistakes)	/* only if not turned off */
	if (strlen (rest))
	  printf ("%s%s%s\n", BEGIN_COMMENT, rest + 1, END_COMMENT);
    }
  else if (!strcmp (commandstr, C_INIT_STATS))
    /* zero out the stats variables */
    {
      printcomment ("", "Initializing statistics", "\n");
      init_stats ();
    }
  else if (!strcmp (commandstr, C_PRINT_STATS))
    /* print the performance statistics on the log output */
    {
      print_stats ();
    }
  else if (!strcmp (commandstr, C_LIST_PARAMS))
    /* list current values of parameters, input files */
    {
      list_params ();
    }
  else if (!strcmp (commandstr, C_QUIT))
    /* exit from the program */
    {
      if (displaying)
	close_display ();
      exit (EXIT_NORMAL);
    }
  else if (!strcmp (commandstr, C_LREPFILE))
    /* read in the lexical representations */
    {
      sscanf (rest, "%s", lrepfile);	/* get the filename */
      reps_init (LEXICAL_KEYWORD, lrepfile, lwords, &nlwords, &nlrep);
      if (displaying)
	display_new_lex_labels(LEXWINMOD, nlnet, nlwords,
			       lwords, nlrep, lunits);
    }
  else if (!strcmp (commandstr, C_SREPFILE))
    /* read in the semantic representations */
    {
      sscanf (rest, "%s", srepfile);	/* get the filename */
      reps_init (SEMANTIC_KEYWORD, srepfile, swords, &nswords, &nsrep);
      if (displaying)
	display_new_lex_labels(SEMWINMOD, nsnet, nswords,
			       swords, nsrep, sunits);
    }
  else if (!strcmp (commandstr, C_PROCFILE))
    /* read in the processing modules */
    {
      sscanf (rest, "%s", procfile);	/* get the filename */
      proc_init ();
    }
  else if (!strcmp (commandstr, C_HFMFILE))
    /* read in the hierarchical feature maps */
    {
      sscanf (rest, "%s", hfmfile);	/* get the filename */
      hfm_init ();
    }
  else if (!strcmp (commandstr, C_HFMINPFILE))
    /* read in the labels for hierarchical feature map display */
    {
      sscanf (rest, "%s", hfminpfile);	/* get the filename */
      if (displaying)
	display_new_hfm_labels();
    }
  else if (!strcmp (commandstr, C_LEXFILE))
    /* read in the proc modules */
    {
      sscanf (rest, "%s", lexfile);	/* get the filename */
      lex_init ();
    }
  else if (!strcmp (commandstr, C_DISPLAYING))
    /* start or close the X window display */
    {
      read_int_par (rest, commandstr, &displaying);	/* get int value */
      if (displaying)
	{
	  if (form == NULL)	/* display does not exist at all.. */
	    {
	      printcomment ("", "Clearing networks", "\n");
	      display_init ();	/* ..so bring it up from scratch */
	    }
	  else			/* display exists */
	    XtMapWidget (main_widget);	/* so make sure it is on screen */
	}
      else if (!displaying && form != NULL)	/* display exists */
	{
	  XtUnmapWidget (main_widget);	/* so hide it (but don't destroy) */
	}
      /* decide what to do when user hits ^C */
      if (displaying)
	/* do not catch control-c */
	signal (SIGINT, SIG_DFL);
      else
	/* catch control-c */
	signal (SIGINT, sig_handler);
      longjmp (loop_stories_env, 1);
    }
  else if (commandstr[0] == C_COMMENT_CHAR);	/* a comment, ignore */
  else if (!strcmp (commandstr, C_NSLOT))
    /* set the number of script representation slots */
    {
      read_int_par (rest, commandstr, &nslot);	/* get int value */
      if (nslot > MAXSLOT || nslot <= 0)
	{
	  fprintf (stderr, "Value of nslot exceeds array size\n");
	  exit (EXIT_SIZE_ERROR);
	}
      nslotrep = nslot * nsrep;	/* set also number of units in rep */
    }
  else if (!strcmp (commandstr, C_NCASE))
    /* set number of case representation assemblies */
    {
      read_int_par (rest, commandstr, &ncase);	/* get int value */
      if (ncase > MAXCASE || ncase <= 0)
	{
	  fprintf (stderr, "Value of ncase exceeds array size\n");
	  exit (EXIT_SIZE_ERROR);
	}
      ncaserep = ncase * nsrep;	/* set also number of units in rep */
    }
  else if (!strcmp (commandstr, C_CHAIN))
    /* whether input for a net is taken from output of another or cleaned up */
    read_int_par (rest, commandstr, &chain);
  else if (!strcmp (commandstr, C_WITHLEX))
    /* whether lexicon is included in the simulation */
    read_int_par (rest, commandstr, &withlex);
  else if (!strcmp (commandstr, C_WITHHFM))
    /* whether episodic memory is included in the simulation */
    read_int_par (rest, commandstr, &withhfm);
  else if (!strcmp (commandstr, C_BABBLING))
    /* whether log output is verbose */
    read_int_par (rest, commandstr, &babbling);
  else if (!strcmp (commandstr, C_PRINT_MISTAKES))
    /* whether errors are printed at log output even when not babbling */
    read_int_par (rest, commandstr, &print_mistakes);
  else if (!strcmp (commandstr, C_LOG_LEXICON))
    /* whether lexicon translation is printed in the log output */
    read_int_par (rest, commandstr, &log_lexicon);
  else if (!strcmp (commandstr, C_IGNORE_STOPS))
    /* whether stop commands in an input file are ignored */
    read_int_par (rest, commandstr, &ignore_stops);
  else if (!strcmp (commandstr, C_DELAY))
    /* sleep this many seconds at each handle_events */
    read_int_par (rest, commandstr, &data.delay);
  else if (!strcmp (commandstr, C_TOPSEARCH))
    /* search threshold at the script level */
    read_float_par (rest, commandstr, &topsearch);
  else if (!strcmp (commandstr, C_MIDSEARCH))
    /* search threshold at the track level */
    read_float_par (rest, commandstr, &midsearch);
  else if (!strcmp (commandstr, C_TRACENC))
    /* size of the trace on a trace map */
    read_int_par (rest, commandstr, &tracenc);
  else if (!strcmp (commandstr, C_TSETTLE))
    /* number of settling iterations in trace maps */
    read_int_par (rest, commandstr, &tsettle);
  else if (!strcmp (commandstr, C_EPSILON))
    /* min activation for a trace to be stored on a unit */
    read_float_par (rest, commandstr, &epsilon);
  else if (!strcmp (commandstr, C_ALIVEACT))
    /* min activation for a trace to be retrieved */
    read_float_par (rest, commandstr, &aliveact);
  else if (!strcmp (commandstr, C_MINACT))
    /* lower threshold for trace map sigmoid */
    read_float_par (rest, commandstr, &minact);
  else if (!strcmp (commandstr, C_MAXACT))
    /* upper threshold for trace map sigmoid */
    read_float_par (rest, commandstr, &maxact);
  else if (!strcmp (commandstr, C_GAMMAEXC))
    /* excitatory lateral connection weight on trace feature maps */
    read_float_par (rest, commandstr, &gammaexc);
  else if (!strcmp (commandstr, C_GAMMAINH))
    /* inhibitory lateral connection weight on trace feature maps */
    read_float_par (rest, commandstr, &gammainh);
  else if (!strcmp (commandstr, C_WITHINERR))
    /* statistics collected within this close to the correct value */
    read_float_par (rest, commandstr, &withinerr);
  else
    {
      sprintf (s, "Command %s not recognized", commandstr);
      printcomment ("", s, "\n");
    }
  /* if interactive, print out the prompt after processing each command */
  if (fp == stdin || fp == NULL)
    printf ("%s", promptstr);
}


/*********************  initializations ******************************/

/**************** X interface, command line */

static void
create_toplevel_widget (argc, argv)
  /* retrieve resources, create a colormap, and start the top-level widget */
  int argc;
  char **argv;
{
  Widget dummy;			/* dummy top-level widget */
  Arg args[10];
  int scr;			/* temporary screen (for obtaining colormap) */
  int n = 0;			/* argument counter */

  /* Create a dummy top-level widget to retrieve resources */
  /* (necessary to get the right netcolor etc with owncmap) */
  dummy = XtAppCreateShell (NULL, APP_CLASS, applicationShellWidgetClass,
			    theDisplay, NULL, ZERO);
  XtGetApplicationResources (dummy, &data, resources,
			     XtNumber (resources), NULL, ZERO);
  scr = DefaultScreen (theDisplay);
  visual = DefaultVisual (theDisplay, scr);
  
  /* Select colormap; data.owncmap was specified in resources
     or as an option */
  if (data.owncmap)
    {
      colormap = XCreateColormap (theDisplay, DefaultRootWindow (theDisplay),
				  visual, AllocNone);
      XtSetArg (args[n], XtNcolormap, colormap);
      n++;
    }
  else
    colormap = DefaultColormap (theDisplay, scr);
  XtDestroyWidget (dummy);
  
  /* Create the real top-level widget */
  XtSetArg (args[n], XtNargv, argv);
  n++;
  XtSetArg (args[n], XtNargc, argc);
  n++;
  main_widget = XtAppCreateShell (NULL, APP_CLASS, applicationShellWidgetClass,
				  theDisplay, args, n);
  XtGetApplicationResources (main_widget, &data, resources,
			     XtNumber (resources), NULL, ZERO);
}


static void
process_remaining_arguments (argc, argv)
  /* parse nongraphics options, simufile and inputfile, setup displaying */
  int argc;
  char **argv;
{
  int i;

  /* if opendisplay was successful, all options were parsed into "data" */
  /* otherwise, we have to get the options from command-line argument list */
  if (theDisplay != NULL)
    process_display_options (argc, argv);
  else
    process_nodisplay_options (&argc, argv);

  /* figure out the initfile and input file names */
  sprintf (initfile, "%s", "");
  sprintf (current_inpfile, "%s", "");
  for (i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      {
	fprintf (stderr, "Unknown option %s\n", argv[i]);
	usage (argv[0]);
	exit (EXIT_COMMAND_ERROR);
      }
    else if (!strlen (initfile))	/* first argument is initfile */
      sprintf (initfile, "%s", argv[i]);
    else if (!strlen (current_inpfile))	/* second is inputfile */
      sprintf (current_inpfile, "%s", argv[i]);
    else
      {
	fprintf (stderr, "Too many arguments\n");
	usage (argv[0]);
	exit (EXIT_COMMAND_ERROR);
      }
  if (!strlen (initfile))		/* if no initfile given */
    sprintf (initfile, "%s", DEFAULT_INITFILENAME);
  if (!strlen (current_inpfile))		/* if no inputfile given */
    sprintf (current_inpfile, "%s", DEFAULT_INPUTFILENAME);
}


static void
process_display_options (argc, argv)
/* get the non-graphics-related options from the "data" structure */
  int argc;
  char **argv;
{
  /* quick user help */
  if (data.help)
    {
      usage (argv[0]);
      exit (EXIT_NORMAL);
    }
}


static void
process_nodisplay_options (argc, argv)
/* get the non-graphics-related options from the command string */
  int *argc;
  char **argv;
{
  char *res_str;		/* string value of an option */
  XrmDatabase db = NULL;	/* resource database for options */

  XrmParseCommand (&db, options, XtNumber (options), APP_CLASS, argc, argv);

  /* quick user help */
  res_str = get_option (db, argv[0], APP_CLASS, "help", "Help");
  if (res_str != NULL)
    if (!strcasecmp ("true", res_str))
      {
	usage (argv[0]);
	exit (EXIT_NORMAL);
      }
}


static char *
get_option (db, app_name, app_class, res_name, res_class)
  /* return the pointer to the string value of the resource */
  XrmDatabase db;			/* resource database */
  char *res_name, *res_class,		/* resource name and class */
    *app_name, *app_class;		/* application name and class */
{
  XrmValue value;			/* value of the resource */
  char *type,				/* resource type */
    name[MAXSTRL + 1], class[MAXSTRL + 1];/* full name and class of resource */

  sprintf (name, "%s.%s", app_name, res_name);
  sprintf (class, "%s.%s", app_class, res_class);
  XrmGetResource(db, name, class, &type, &value);
  return (value.addr);
}


static void
usage (app_name)
  /* print out the list of options and arguments */
  char *app_name;			/* name of the program */
{
  char s[MAXSTRL + 1];

  sprintf(s, "%s %s", OPT_DELAY, "<sec>");
  fprintf (stderr, "Usage: %s [options] [init file] [input file]\n\
where the options are\n\
  %-20s  Prints this message\n\
  %-20s  Bring up graphics display\n\
  %-20s  Text output only\n\
  %-20s  Use a private colormap\n\
  %-20s  Use the existing colormap\n\
  %-20s  Delay in updating the screen (in seconds)\n",
	   app_name, OPT_HELP, OPT_GRAPHICS, OPT_NOGRAPHICS,
	   OPT_OWNCMAP, OPT_NOOWNCMAP, s);
}


/********************* system setup */

static void
init_system ()
/* set up blanks, read configuration commands from the init file,
   set up graphics if displaying */
{
  FILE *fp;			/* pointer to the init file */
  
  setbuf (stdout, NULL);	/* output one word at a time */
  lwords = lwordsarray + 1;	/* reserve index -1 for the blank */
  swords = swordsarray + 1;	/*  for lexical and semantic words.. */
  init_stats_blanks ();		/*  ..and for the statics tables */

  printf ("Initializing DISCERN from %s:\n", initfile);
  open_file (initfile, "r", &fp, REQUIRED);
  process_inpfile (fp);		/* read commands from initfile */
  fclose (fp);

  /* decide whether to bring up display */
  if (theDisplay && data.bringupdisplay)
    displaying = TRUE;
  else
    displaying = FALSE;

  /* initialize graphics */
  if (displaying)
    display_init ();

  if (!displaying)
    /* catch control-c; equivalent to the "clear" button in graphics mode */
    signal (SIGINT, sig_handler);
  printf ("System initialization complete.\n");
}


static int
system_initialized (commandstr)
/* check whether the initialization commands are given in the right order */
     char *commandstr;		/* command */
{

  /* these commands can appear anywhere */
  if (strcmp (commandstr, C_ECHO) && strcmp (commandstr, C_BABBLING) &&
      strcmp (commandstr, C_LIST_PARAMS) && strcmp (commandstr, C_QUIT) &&
      commandstr[0] != C_COMMENT_CHAR)
    /* first we need the data sizes */
    if (nslot <= 0 || ncase <= 0)
      {
	if (strcmp (commandstr, C_NSLOT) && strcmp (commandstr, C_NCASE))
	  return (FALSE);
      }
    /* next the representation files */
    else if (!strlen (lrepfile) || !strlen (srepfile))
      {
	if (strcmp (commandstr, C_LREPFILE) && strcmp (commandstr, C_SREPFILE))
	  return (FALSE);
      }
    /* then the simulation files */
    else if (!strlen (procfile) || !strlen (hfmfile) ||
	     !strlen (hfminpfile) || !strlen (lexfile))
      {
	if (strcmp (commandstr, C_PROCFILE) && strcmp (commandstr, C_HFMFILE)&&
	    strcmp (commandstr, C_HFMINPFILE) && strcmp (commandstr,C_LEXFILE))
	  return (FALSE);
      }
  return (TRUE);
}


/********************* reps */

static void
reps_init (lexsem, repfile, words, nwords, nrep)
/* read the word labels and representations from a file */
/* this is called once for lexical and once for semantic words */
     char *lexsem,		/* either "lexical" or "semantic" */
      *repfile;			/* name of the representation file */
     WORDSTRUCT words[];	/* the lexicon data structure */
     int *nwords,		/* return number of words */
      *nrep;			/* return representation dimension */
{
  int i;
  FILE *fp;
  char instancestring[MAXSTRL + 1],/* temporarily holds list of instances */
  wordstring[MAXSTRL + 1],	/* temporarily holds the word */
  repstring[MAXSTRL + 1];	/* temporarily holds the representations */

  /* first set up the blank word */
  sprintf (words[BLANKINDEX].chars, "%s", BLANKLABEL);
  sprintf (words[BLANKINDEX].chars, "%s", BLANKLABEL);
  for(i = 0; i < MAXREP; i++)	/* make sure its rep is all-0 */
    words[BLANKINDEX].rep[i] = 0.0;

  printf ("Reading %s representations from %s...", lexsem, repfile);
  open_file (repfile, "r", &fp, REQUIRED);

  if (!strcmp (lexsem, SEMANTIC_KEYWORD))
    /* if this is semantic file, read the list of instances and hold it */
    {
      /* find the instances */
      read_till_keyword (fp, REPS_INST, REQUIRED);
      fgetline (fp, instancestring, MAXSTRL);
    }

  /* get representation size */
  read_till_keyword (fp, PROCSIMU_REPSIZE, REQUIRED);
  fscanf (fp, "%d", nrep);
  if (*nrep > MAXREP || *nrep <= 0)
    {
      fprintf (stderr, "%s exceeds array size\n", PROCSIMU_REPSIZE);
      exit (EXIT_SIZE_ERROR);
    }
  
  /* find the representations */
  read_till_keyword (fp, PROCSIMU_REPS, REQUIRED);
  for (i = 0; i < MAXWORDS + 1; i++)
    {
      if (fscanf (fp, "%s", wordstring) == EOF)   /* read label */
	break;
      else if (i >= MAXWORDS)
	{
	  fprintf (stderr, "Number of %s words exceeds array size\n",
		   lexsem);
	  exit (EXIT_SIZE_ERROR);
	}
      else if (strlen (wordstring) > MAXWORDL)
	{
	  fprintf (stderr, "Length of word %s exceeds array size\n",
		   wordstring);
	  exit (EXIT_SIZE_ERROR);
	}
      sprintf(words[i].chars, "%s", wordstring);

      /* remember the period and question mark indices for easy checking */
      /* (they are is used to end sentences and questions in input files) */
      if (!strcmp (words[i].chars, SENTDELIM))
	sentdelimindex = i;
      else if (!strcmp (words[i].chars, QUESTDELIM))
	questdelimindex = i;

      /* read the representation components */
      fgetline (fp, repstring, MAXSTRL);
      /* convert the string to numbers and load */
      if (text2floats (words[i].rep, *nrep, repstring) != *nrep)
	{
	  fprintf (stderr, "Wrong number of components for %s\n",
		   words[i].chars);
	  exit (EXIT_DATA_ERROR);
	}
    }
  *nwords = i;			/* set the number of words */
  fclose (fp);

  if (!strcmp (lexsem, SEMANTIC_KEYWORD))
    {
      /* after the lexicon has been set up, we can set up instance indices */
      /* convert the word strings into indices */
      ninstances = list2indices (instances, instancestring, *nwords,REPS_INST);
      /* we also need to set the number of units in case and story rep */
      /* now when we know the dimension of the semantic rep */
      ncaserep = ncase * (*nrep);
      nslotrep = nslot * (*nrep);
      printf ("%d reps (%d instances).\n", *nwords, ninstances);
    }
  else
    printf ("%d reps.\n", *nwords);
}


/********************* I/O commands ******************************/

static void
read_story_data (fp)
/* read a story specification */
     FILE *fp;			/* input file */
{
  int k,
    included;			/* temporarily holds whether sent included */
  char rest[MAXSTRL + 1];	/* one line in the story */

  /* read the story representation */
  fgetline (fp, rest, MAXSTRL);
  /* convert words into indices and store in story.slots */
  if (list2indices (story.slots, rest, nslot, INP_SLOTS) != nslot)
    {
      fprintf (stderr, "Wrong number of %s\n", INP_SLOTS);
      exit (EXIT_DATA_ERROR);
    }

  /* read the sentences */
  for (k = 0; fscanf (fp, "%d", &included) == 1; k++)
    {
      /* the sentence begins with a number (representing included) */
      if (k >= MAXSENT)
	{
	  fprintf (stderr, "Number of sentences exceeds array size\n");
	  exit (EXIT_SIZE_ERROR);
	}
      story.sents[k].included = included;
      /* read the sentence, convert to indices and store */
      fgetline (fp, rest, MAXSTRL);
      sent2indices (&story.sents[k], rest);
    }
  story.nsent = k;
}


static void
read_qa_data (fp)
/* read a question and answer specification */
     FILE *fp;			/* input file */
{
  char rest[MAXSTRL + 1];	/* question and answer line */

  /* target story representation */
  fgetline (fp, rest, MAXSTRL);
  if (list2indices (qa.slots, rest, nslot, INP_SLOTS) != nslot)
    {
      fprintf (stderr, "Wrong number of %s\n", INP_SLOTS);
      exit (EXIT_DATA_ERROR);
    }

  /* read the question, convert to indices and store */
  fgetline (fp, rest, MAXSTRL);
  sent2indices (&qa.question, rest);

  /* read the answer, convert to indices and store */
  fgetline (fp, rest, MAXSTRL);
  sent2indices (&qa.answer, rest);
}


static void
read_text_question (rest)
/* read a user-typed question */
     char *rest;		/* question string */
{
  int i;

  /* the target story is not known */
  for (i = 0; i < nslot; i++)
    qa.slots[i] = BLANKINDEX;

  /* convert question text into indices and store */
  sent2indices (&qa.question, rest);

  /* target case roles and answer words and caseroles are not known,use blank*/
  for (i = 0; i < ncase; i++)
    qa.question.caseroles[i] = BLANKINDEX;
  for (i = 0; i < MAXSEQ; i++)
    qa.answer.words[i] = BLANKINDEX;
  for (i = 0; i < ncase; i++)
    qa.answer.caseroles[i] = BLANKINDEX;
  /* we don't know how long the answer will be so prepare for the worst */
  qa.answer.nseq = MAXSEQ;
}


static void
list_params ()
/* print the current configuration file names and parameters on log output */
{
  printf ("initfile   = %s\n", initfile);
  printf ("lrepfile   = %s\n", lrepfile);
  printf ("srepfile   = %s\n", srepfile);
  printf ("procfile   = %s\n", procfile);
  printf ("hfmfile    = %s\n", hfmfile);
  printf ("hfminpfile = %s\n", hfminpfile);
  printf ("lexfile    = %s\n", lexfile);
  printf ("inputfile  = %s\n", current_inpfile);

  printf ("nslot=%d, ncase=%d\n",
	  nslot, ncase);
  printf ("chain=%d, withlex=%d, withhfm=%d\n",
	  chain, withlex, withhfm);
  printf ("displaying=%d, delay=%d\n",
	  displaying, data.delay);
  printf ("babbling=%d, print_mistakes=%d, log_lexicon=%d\n",
	  babbling, print_mistakes, log_lexicon);
  printf ("topsearch=%.3f, midsearch=%.3f\n",
	  topsearch, midsearch);
  printf ("tracenc=%d, tsettle=%d, epsilon=%f, aliveact=%.3f\n",
	  tracenc, tsettle, epsilon, aliveact);
  printf ("minact=%.3f, maxact=%.3f, gammaexc=%.3f, gammainh=%.3f\n",
	  minact, maxact, gammaexc, gammainh);
  printf ("withinerr=%.3f\n",
	  withinerr);
}


/********************* text/index conversions ****************************/

int
list2indices (itemarray, rest, maxitems, listname)
/* convert at most maxitems words in the rest list to indices,
   return the number of items */
     int itemarray[];		/* array where the indices will be stored */
     int maxitems;		/* size of the array */
     char *rest;		/* the input string */
     char listname[];		/* name of the variable list */
{
  int j = 0, index;		/* index of the word in the lexicon */
  char wordstring[MAXSTRL + 1];	/* string for one word */

  while (TRUE)
    {
      rest = rid_sspace (rest);	/* first remove blanks */
      if (rest != NULL)		/* if there is anything left */
	{
	  if (j >= maxitems)
	    {
	      fprintf (stderr, "Number of %s exceeds array size\n",
		       listname);
	      exit (EXIT_SIZE_ERROR);
	    }
	  sscanf (rest, "%s", wordstring);	/* get the next word */
	  rest += strlen (wordstring);		/* remove from the string */
	  index = wordindex (wordstring);	/* get or set its index */
	  itemarray[j++] = index;		/* put it in the list */
	}
      else
	break;			/* if the string has been exhausted */
    }
  return (j);			/* number of words read */
}


static void
sent2indices (sent, rest)
/* convert the words in the sentence + caserole definition into indices */
     SENTSTRUCT *sent;		/* structure for the sentence data */
     char rest[];		/* the input string */
{
  int j,			/* word counter */
    index;			/* index of the current word */
  char wordstring[MAXSTRL + 1];	/* string for one word */

  sprintf (wordstring, "%s", "");
  /* read input word sequence until delimiter and check that it fits array */
  for (j = 0;
       strcasecmp (wordstring, SENTDELIM) &&
       strcasecmp (wordstring, QUESTDELIM) &&
       (j < MAXSEQ + 1);
       j++)
    {
      rest = rid_sspace (rest);			/* first get rid of blanks */
      if (rest != NULL)				/* if there is anything left */
	{
	  if (j >= MAXSEQ)
	    {
	      fprintf (stderr,
		       "Number of words in sequence exceeds array size\n");
	      /* give the user a break, don't exit */
	      if (text_question)
		break;
	      else
		exit (EXIT_SIZE_ERROR);
	    }
	  sscanf (rest, "%s", wordstring);	/* get the next word */
	  rest += strlen (wordstring);		/* remove from the string */

	  /* establish its index */
	  index = wordindex (wordstring);
	  if (index == NONE && text_question)
	    /* word not in lexicon; don't include it */
	    j--;
	  else
	    /* make it part of the input data */
	    sent->words[j] = index;
	}
      else
	break;
    }
  /* set up number of words in this sentence */
  sent->nseq = j;

  if (!text_question)
    {
      /* read case-role assignment and check that it does not exceed array */
      for (j = 0; j < MAXCASE + 1; j++)
	{
	  rest = rid_sspace (rest);		/* first get rid of blanks */
	  if (rest != NULL)			/* if there is anything left */
	    {
	      if (j >= MAXCASE)
		{
		  fprintf (stderr, "Number of cases exceeds array size\n");
		  exit (EXIT_SIZE_ERROR);
		}
	      sscanf (rest, "%s", wordstring);/* get the next word */
	      rest += strlen (wordstring);	/* remove from the string */
	      
	      /* establish index of the word */
	      index = wordindex (wordstring);
	      /* make part of input data */
	      sent->caseroles[j] = index;
	    }
	  else
	    break;			/* if the string has been exhausted */
	}
      /* if it was not a keyboard question, check the number of cases */
      if (j != ncase)
	{
	  fprintf (stderr, "Wrong number of case roles\n");
	  exit (EXIT_DATA_ERROR);
	}
    }
}


static int
text2floats (itemarray, nitems, nums)
/* convert the string of numbers to floats and stores in an array, returning
   the number of floats read, or nitems + 1 if there where too many */
     double itemarray[];	/* array where the numbers will be stored */
     int nitems;		/* max number of items */
     char nums[];		/* the input string */
{
  int j = 0;
  char onenum[MAXSTRL + 1];	/* string for one float */

  while (j < nitems)
    {
      nums = rid_sspace (nums);	/* first get rid of blanks */
      if (nums != NULL)		/* if there is anything left */
	{
	  sscanf (nums, "%s", onenum);  /* get the next number */
	  nums += strlen (onenum);	/* remove from the string */
	  if (sscanf (onenum, "%lf", &itemarray[j]) != 1)
	    break;			/* if could not get a number */
	  else
	    j++;
	}
      else
	break;			/* if the string has been exhausted */
    }
  /* check if there was more stuff in the input string that was not read */
  if (j == nitems && strlen (nums))
    {
      nums = rid_sspace (nums);	/* first get rid of blanks */
      if (nums != NULL)		/* if there is anything left */
	j++;
    }
  return (j);
}


int
text2ints (itemarray, nitems, nums)
/* convert the string of numbers to ints and stores in an array, returning
   the number of ints read, or nitems + 1 if there where too many */
     int itemarray[];		/* array where the numbers will be stored */
     int nitems;		/* max number of items */
     char nums[];		/* the input string */
{
  int j = 0;
  char onenum[MAXSTRL + 1];	/* string for one float */

  while (j < nitems)
    {
      nums = rid_sspace (nums);	/* first get rid of blanks */
      if (nums != NULL)		/* if there is anything left */
	{
	  sscanf (nums, "%s", onenum);  /* get the next number */
	  nums += strlen (onenum);	/* remove from the string */
	  if (sscanf (onenum, "%d", &itemarray[j]) != 1)
	    break;			/* if could not get a number */
	  else
	    j++;
	}
      else
	break;			/* if the string has been exhausted */
    }
  /* check if there was more stuff in the input string that was not read */
  if (j == nitems && strlen (nums))
    {
      nums = rid_sspace (nums);	/* first get rid of blanks */
      if (nums != NULL)		/* if there is anything left */
	j++;
    }
  return (j);
}


/********************* lexicon entries *******************************/

static int
wordindex (wordstring)
/* find the lexicon index of a word given as a string */
     char wordstring[];		/* text word */
{
  int i;

  for (i = BLANKINDEX; i < nswords; i++) /* scan through the lexicon */
    if (!strcasecmp (wordstring, swords[i].chars))
      return (i);		/* if the word found, return its index */
  fprintf (stderr, "Word %s not found in the lexicon\n", wordstring);
  /* when text question, just print the message and continue */
  if (text_question)
    return (NONE);
  else
    exit (EXIT_DATA_ERROR);
}


int
find_nearest (rep, words, nrep, nwords)
/* find the index of the nearest representation in the lexicon */
     double rep[];		/* word representation */
     WORDSTRUCT words[];	/* lexicon (lexical or semantic) */
     int nrep,			/* rep dimension */
       nwords;			/* number of words in lexicon */
{
  int i,
    bestindex = BLANKINDEX;	/* index of the closest rep so far */
  double lbest,			/* closest distance so far */
    dist;			/* last distance */

  lbest = LARGEFLOAT;
  /* linearly search through the lexicon */
  /* starting from the blank */
  for (i = BLANKINDEX; i < nwords; i++)
    {
      dist = distance (NULL, rep, words[i].rep, nrep);
      if (dist < lbest)		/* keep track of the best one so far */
	{
	  bestindex = i;
	  lbest = dist;
	}
    }
  return (bestindex);
}


int
select_lexicon (modi, words, nrep, nwords)
/* based on the module number, selects either lexical or semantic lexicon
   returns the module number used for the window */
int modi;				/* module number */
WORDSTRUCT **words;			/* lexicon pointer*/
int *nrep,				/* representation size */
  *nwords;				/* number of words */
{
  if (modi != LINPMOD && modi != LOUTMOD)
    /* it is semantic */
    {
      *words = swords;
      *nrep = nsrep;
      *nwords = nswords;
      return (SEMWINMOD);
    }
  else
    /* it is lexical */
    {
      *words = lwords;
      *nrep = nlrep;
      *nwords = nlwords;
      return (LEXWINMOD);
    }
}


/********************* general I/O routines ****************************/

int
read_till_keyword (fp, keyword, required)
/* read text in the file until you find the given keyword
   in the beginning of the line; if the keyword is required but
   not found, terminate program; otherwise return if it was found or not */
     FILE *fp;			/* pointer to the file */
     char keyword[];		/* the keyword string */
     int required;		/* whether to abort if keyword not found */
{
  char s[MAXSTRL + 1];

  while (TRUE)
    if (fscanf (fp, "%s", s) == EOF)
      /* file ran out, keyword was not found */
      if (required)
	{
	  fprintf (stderr, "Could not find keyword: %s\n", keyword);
	  exit (EXIT_DATA_ERROR);
	}
      else
	/* that's ok; return the signal indicating that it was not found */
	return (FALSE);
    else
      /* something was read from the file */
      {
	/* if it was the keyword, return success signal */
	if (!strcasecmp (s, keyword))
	  return (TRUE);
	/* otherwise get rid of the rest of the line */
	fgl (fp);
      }
}


void
readfun (fp, place, par1, par2)
/* function for reading a float from a file */
/* given as a parameter for reading in weights
   (there are other functions for writing and randomizing weights) */
     FILE *fp;			/* weight file */
     double *place,		/* return the float read */
       par1, par2;		/* unused */
{
  if (fscanf (fp, "%lf", place) != 1)
    {
      fprintf (stderr, "Error reading weights\n");
      exit (EXIT_DATA_ERROR);
    }
}


static void
read_int_par (rest, commandstr, variable)
/* read and set an integer simulation parameter */
     char *rest,		/* value is in this string */
       *commandstr;		/* name of the parameter */
     int *variable;		/* the parameter variable */
{
  char s[MAXSTRL + 1];

  sscanf (rest, "%d", variable);	/* read the value */
  sprintf (s, "Set %s = %d", commandstr, *variable);
  printcomment ("", s, "\n");
}


static void
read_float_par (rest, commandstr, variable)
/* read and set a float simulation parameter */
     char *rest,		/* value is in this string */
       *commandstr;		/* name of the parameter */
     double *variable;		/* the parameter variable */
{
  char s[MAXSTRL + 1];

  sscanf (rest, "%lf", variable);	/* read the value */
  sprintf (s, "Set %s = %f", commandstr, *variable);
  printcomment ("", s, "\n");
}


static char *
rid_sspace (rest)
/* read the blanks off the string */
     char *rest;		/* string to be cleaned up */
{
  int i = 0;
  while (i < strlen (rest) && (rest[i] == ' ' || rest[i] == '\t'))
    i++;
  if (i < strlen (rest))	/* if there is anything left */
    return (rest + i);
  else
    return (NULL);
}


void
fgetline (fp, s, lim)
/* read a line of at most lim characters from the file fp into string s */
     FILE *fp;
     char *s;
     int lim;
{
  int c = 0;
  int i = 0;

  while (--lim > 0 && (c = getc (fp)) != EOF && c != '\n')
    s[i++] = (char) c;
  s[i] = '\0';
  if (lim == 0 && c != EOF && c != '\n')
    {
      fprintf (stderr, "Line character limit exceeded\n");
      exit (EXIT_SIZE_ERROR);
    }
}


void
fgl (fp)
/* get rid of the rest of the line and the newline in the end */
     FILE *fp;
{
  int c;

  while ((c = getc (fp)) != EOF && c != '\n')
    ;
}


void
printcomment (beginning, s, ending)
/* print out a comment in log output */
     char *beginning,		/* used sometimes for a newline */
      *s,			/* the comment itself */
      *ending;			/* used sometimes for a newline */
{
  if (babbling)
    /* BEGIN_COMMENT, END_COMMENT are strings enclosing the comment like [] */
    printf ("%s%s%s%s%s", beginning, BEGIN_COMMENT, s, END_COMMENT, ending);
}


int
open_file (filename, mode, fpp, required)
/* open a file, exit or return FALSE if failure */
     char *filename;		/* file name */
     FILE **fpp;		/* return the file pointer */
     char *mode;		/* open mode: "r", "a", "w" */
     int required;		/* whether to exit if cannot open */
{
  if ((*fpp = fopen (filename, mode)) == NULL)
    {
      fprintf (stderr, "Cannot open %s\n", filename);
      if (required)
	exit (EXIT_FILE_ERROR);
      else
	return (FALSE);
    }
  return (TRUE);
}


/*********************  distance routines ******************************/

double
distance (foo, v1, v2, nrep)
/* compute the Euclidean distance of two nrep dimensional vectors */
     double v1[], v2[];		/* the vectors */
     int nrep;			/* their dimensionality */
     int *foo;			/* not used */
{
  double sum = 0.0;
  int i;

  for (i = 0; i < nrep; i++)
    sum += (v1[i] - v2[i]) * (v1[i] - v2[i]);
  return (sqrt (sum));
}


double
seldistance (indices, v1, v2, ncomp)
/* compute the Euclidean distance of "subvectors" of two vectors,
   that is, select only a subset of components */
/* this routine is used in the hierarchical feature maps */
     int indices[];		/* include these components */
     double v1[], v2[];		/* the vectors */
     int ncomp;			/* their dimensionality */
{
  double sum = 0.0;
  int i;

  /* count only selected components in the distance */
  for (i = 0; i < ncomp; i++)
    sum = sum + (v1[indices[i]] - v2[i]) * (v1[indices[i]] - v2[i]);
  return (sqrt (sum));
}


/********************* feature map search routines  ********************/

void
distanceresponse (unit, inpvector, ninpvector, responsefun, indices)
/* compute the response of the feature map unit to the input vector
   proportional to distance (distance function given as parameter) */
     FMUNIT *unit;                      /* unit on the map */
     double inpvector[];			/* input vector values */
     int ninpvector;			/* dimension of the inputvector */
     int indices[];			/* include these components */
     /* the function determining the unit response */
     double (*responsefun) __P((int *, double *, double *, int));
{
  /* save previous value to avoid redisplay if it does not change */
  unit->prevvalue = unit->value;
  /* calculate the response of this unit to the external input */
  unit->value = (*responsefun) (indices, inpvector, unit->comp, ninpvector);
}


void
updatebestworst (best, worst, besti, bestj, unit, i, j, better, worse)
/* check if this unit is the best or the worst so far, and if so,
   update the best indices */
     double *best, *worst;		/* activity of max and min resp unit */
     int *besti, *bestj;		/* indices of the max resp unit */
     FMUNIT *unit;                      /* unit on the map */
     int i, j;				/* indices of the unit */
     /* these functions are either < or > depending on how the activity
        is represented (as distance or actual activation) */
     int (*better) __P((double, double)),
     (*worse) __P((double, double));
{
  /* check if this unit's response is best so far encountered */
  /* if the current unit is better, store it; depending on whether
     we are looking for the highest or lowest activity, better
     is either < or > */
  if ((*better) (unit->value, *best))
    {
      *besti = i;
      *bestj = j;
      *best = unit->value;
    }
  /* otherwise check if it is the worst response so far */
  if ((*worse) (unit->value, *worst))
    *worst = unit->value;
}


int
clip (index, limit, comparison)
/* if index is beyond limit, reduce it to the limit */
     int index, limit;
     int (*comparison) __P((int n, int m)); /* <= if upper limit, */
                                            /* >= if lower limit */
{
  if ((*comparison) (index, limit))
    return (index);
  else
    return (limit);
}


int
ige (num1, num2)
/* integer greater or equal (for limit comparison) */
     int num1, num2;
{
  return (num1 >= num2);
}


int
ile (num1, num2)
/* integer less or equal (for limit comparison) */
     int num1, num2;
{
  return (num1 <= num2);
}


int
fsmaller (x, y)
/* used for comparing unit activities in search for the
   maximally and minimally responding units */
     double x, y;
{
  return (x < y);
}


int
fgreater (x, y)
/* used for comparing unit activities in search for the
   maximally and minimally responding units */
     double x, y;
{
  return (x > y);
}
