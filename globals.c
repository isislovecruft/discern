/* File: globals.c
 *
 * Defines global variables for the DISCERN system.
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: globals.c,v 1.31 1994/09/19 03:16:06 risto Exp $
 */

#ifdef DEFINE_GLOBALS		/* Defined in the main.c file */
/* these definitions are in effect in main.c */

/************ general stuff *************/

/* lexical and semantic lexica (for checks); index [-1] is used for blank */
WORDSTRUCT *lwords, *swords;	/* actual array pointers */
int nlwords, nswords,		/* number of lexical and semantic words */
  nlrep, nsrep;			/* dimension of lexical and semantic reps */
int sentdelimindex,		/* index of period representation */
  questdelimindex;		/* index of question mark rep */
int instances[MAXWORDS];	/* indices of the instance words */
int ninstances;			/* number of instances */

/* story and qa data */
STORYSTRUCT story;
QASTRUCT qa;
int nslot = NONE,		/* number of assemblies in story rep */
  ncase = NONE;			/* number of assemblies in sentence rep */
int 
  inputs[NMODULES][MAXCASE + MAXSLOT],	/* indices of current inputs */
  targets[NMODULES][MAXCASE + MAXSLOT], /* indices of current targets*/
  ninputs[NMODULES],		/* # of word reps at input */
  noutputs[NMODULES];		/* # of word reps at output */

/* input and output layers of the modules */
double
  inprep[NMODULES][MAXIOREP],	/* input layer activities */
  outrep[NMODULES][MAXIOREP],	/* output layer activities */
  tgtrep[NMODULES][MAXIOREP];	/* target activities */
int ninprep[NMODULES],		/* input layer dimension */
  noutrep[NMODULES];		/* output layer dimension */

/* active reps on the pathways */
double swordrep[MAXREP],	/* semantic words in and out of lexicon */
  caserep[MAXCASE * MAXREP],	/* sentence case-role reps */
  slotrep[MAXSLOT * MAXREP];	/* story reps */
int ncaserep, nslotrep;		/* dimensions of sentence and story reps */

/* file names */
char initfile[MAXFILENAMEL + 1],/* initialization file */
  lrepfile[MAXFILENAMEL + 1],	/* lexical representations */
  srepfile[MAXFILENAMEL + 1],	/* semantic representations */
  procfile[MAXFILENAMEL + 1],	/* proc simulation file */
  hfmfile[MAXFILENAMEL + 1],	/* hfm simulation file */
  hfminpfile[MAXFILENAMEL + 1],	/* hfm labels */
  lexfile[MAXFILENAMEL + 1],	/* lexicon simulation file */
  current_inpfile[MAXFILENAMEL + 1];/* input commands and stories */

/* simulation parameters */
int chain,			/* whether modules feed input to each other */
  withlex, withhfm,		/* whether lexicon and hfm are included */
  displaying,			/* whether X display is up */
  babbling,			/* print verbose log output */
  print_mistakes,		/* print only errors in the log */
  log_lexicon,			/* print also lexicon translation in log */
  ignore_stops,			/* do not stop simulation when told so */
  text_question;		/* flag to process question without target */

/* simulation flags */
int simulator_running,		/* flag: process events or run */
  stepping,			/* stop after every major propagation */
  nohfmmouseevents = FALSE,	/* whether mouse events are disabled */
  nolexmouseevents = FALSE;

double withinerr;		/* defines "close enough" error for stats*/
char *promptstr = DISCERNPROMPT;/* prompt in the terminal I/O */

/* graphics data */
XtAppContext app_con;		/* application context */
Display *theDisplay;		/* display pointer */
Visual *visual;			/* type of display */
Colormap colormap;		/* colormap definition */
Widget main_widget, form;	/* main widget and organization */
NETSTRUCT net[NMODULES];	/* outline of network geometry etc. */
int asmboxhght, titleboxhght;	/* size of text boxes */
RESOURCE_DATA data;		/* resource data structure */


/************ proc stuff *************/

int nhidrep[NPROCMODULES];	/* hidden layer dimension */

/* units and weights */
double
  prevhidrep[NPROCMODULES][MAXHIDREP],		/* prevhid layer activities */
  hidrep[NPROCMODULES][MAXHIDREP],		/* hidden layer activities */
  hidbias[NPROCMODULES][MAXHIDREP],		/* hidden unit biases */
  outbias[NPROCMODULES][MAXIOREP],		/* output unit biases */
  wih[NPROCMODULES][MAXIOREP][MAXHIDREP], 	/* input to hidden weights */
  who[NPROCMODULES][MAXIOREP][MAXHIDREP], 	/* hidden to output weights */
  wph[NPROCMODULES][MAXHIDREP][MAXHIDREP];  	/* prevhid to hidden weights */


/************ lexicon stuff *************/

int nlnet, nsnet;		/* size (side) of lex and sem maps */

FMUNIT lunits[MAXLSNET][MAXLSNET], /* lexical feature map */
  sunits[MAXLSNET][MAXLSNET];	/* semantic feature map */

double
  lsassoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET], /* lex->sem assoc */
  slassoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET]; /* sem->lex assoc */


/************ hfm stuff *************/

int ntopnet,			/* size (side) of top-level maps */
  nmidnet,			/* size (side) of middle-level maps */
  nbotnet;			/* size (side) of bottom-level maps  */

double topsearch,		/* top-level search threshold */
  midsearch;			/* middle-level search threshold */

FMUNIT topunits[MAXTOPNET][MAXTOPNET],	/* top-level maps */
  midunits[MAXTOPNET][MAXTOPNET]
          [MAXMIDNET][MAXMIDNET],	/* middle */
  botunits[MAXTOPNET][MAXTOPNET]
          [MAXMIDNET][MAXMIDNET]
          [MAXBOTNET][MAXBOTNET];	/* bottom-level maps */

int
  /* indices of the input components passed down to the middle level */
  midindices[MAXTOPNET][MAXTOPNET][MAXSLOT * MAXREP],
  nmidindices[MAXTOPNET][MAXTOPNET],	/* their number */
  /* indices of the input components passed down to the bottom level */
  botindices[MAXTOPNET][MAXTOPNET][MAXMIDNET][MAXMIDNET][MAXSLOT * MAXREP],
  nbotindices[MAXTOPNET][MAXTOPNET][MAXMIDNET][MAXMIDNET]; /* their number */


/************ trace stuff *************/

int tracenc,			/* radius of the trace */
  tsettle;			/* number of settling iterations */
double minact,			/* lower threshold for trace map sigmoid */
  maxact,			/* upper threshold for trace map sigmoid */
  gammaexc,			/* exc lat weight on trace feature maps */
  gammainh,			/* inh lat weight on trace feature maps */
  aliveact,			/* threshold for successful retrieval */
  epsilon;			/* min activ. for trace to be stored on unit */

double latweights[MAXTOPNET][MAXTOPNET]
                [MAXMIDNET][MAXMIDNET]
                [MAXBOTNET][MAXBOTNET]
                [MAXBOTNET][MAXBOTNET]; /* lateral weights for trace units */


/************ end definitions *************/

#else
/* these definitions are in effect in all other files except main.c */

extern WORDSTRUCT *lwords, *swords;
extern int nlwords, nswords,
  nlrep, nsrep;
extern int
  sentdelimindex,
  questdelimindex;
extern int instances[MAXWORDS];
extern int ninstances;
extern STORYSTRUCT story;
extern QASTRUCT qa;
extern int nslot,
  ncase;
extern int 
  inputs[NMODULES][MAXCASE + MAXSLOT],
  targets[NMODULES][MAXCASE + MAXSLOT],
  ninputs[NMODULES],
  noutputs[NMODULES];
extern double
  inprep[NMODULES][MAXIOREP],
  outrep[NMODULES][MAXIOREP],
  tgtrep[NMODULES][MAXIOREP];
extern int ninprep[NMODULES],
  noutrep[NMODULES];
extern double swordrep[MAXREP],
  caserep[MAXCASE * MAXREP],
  slotrep[MAXSLOT * MAXREP];
extern int ncaserep, nslotrep;
extern char initfile[MAXFILENAMEL + 1],
  lrepfile[MAXFILENAMEL + 1],
  srepfile[MAXFILENAMEL + 1],
  procfile[MAXFILENAMEL + 1],
  hfmfile[MAXFILENAMEL + 1],
  hfminpfile[MAXFILENAMEL + 1],
  lexfile[MAXFILENAMEL + 1],
  current_inpfile[MAXFILENAMEL + 1];
extern int chain,
  withlex,  withhfm,
  displaying,
  babbling,
  print_mistakes,
  log_lexicon,
  ignore_stops,
  text_question;
extern int simulator_running,
  stepping,
  nohfmmouseevents,
  nolexmouseevents;
extern double withinerr;
extern char *promptstr;
extern XtAppContext app_con;
extern Display *theDisplay;
extern Visual *visual;
extern Colormap colormap;
extern Widget main_widget, form;
extern NETSTRUCT net[NMODULES];
extern int asmboxhght, titleboxhght;
extern RESOURCE_DATA data;
extern int nhidrep[NPROCMODULES];
extern double
  prevhidrep[NPROCMODULES][MAXHIDREP],
  hidrep[NPROCMODULES][MAXHIDREP],
  hidbias[NPROCMODULES][MAXHIDREP],
  outbias[NPROCMODULES][MAXIOREP],
  wih[NPROCMODULES][MAXIOREP][MAXHIDREP],
  who[NPROCMODULES][MAXIOREP][MAXHIDREP],
  wph[NPROCMODULES][MAXHIDREP][MAXHIDREP];
extern int nlnet, nsnet;
extern FMUNIT lunits[MAXLSNET][MAXLSNET],
  sunits[MAXLSNET][MAXLSNET];
extern double
  lsassoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET],
  slassoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET];
extern int ntopnet,
  nmidnet,
  nbotnet;
extern double topsearch,
  midsearch;
extern FMUNIT topunits[MAXTOPNET][MAXTOPNET],
  midunits[MAXTOPNET][MAXTOPNET]
          [MAXMIDNET][MAXMIDNET],
  botunits[MAXTOPNET][MAXTOPNET]
          [MAXMIDNET][MAXMIDNET]
          [MAXBOTNET][MAXBOTNET];
extern int
  midindices[MAXTOPNET][MAXTOPNET][MAXSLOT * MAXREP],
  nmidindices[MAXTOPNET][MAXTOPNET],
  botindices[MAXTOPNET][MAXTOPNET][MAXMIDNET][MAXMIDNET][MAXSLOT * MAXREP],
  nbotindices[MAXTOPNET][MAXTOPNET][MAXMIDNET][MAXMIDNET];
extern int tracenc,
  tsettle;
extern double minact,
  maxact,
  gammaexc,
  gammainh,
  aliveact,
  epsilon;
extern double latweights[MAXTOPNET][MAXTOPNET]
                       [MAXMIDNET][MAXMIDNET]
                       [MAXBOTNET][MAXBOTNET]
                       [MAXBOTNET][MAXBOTNET];

#endif /*  #ifdef DEFINE_GLOBALS */
