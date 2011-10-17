/* File: defs.h
 *
 * Defines parameters and data structures for the DISCERN system.
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: defs.h,v 1.38 1994/09/20 10:46:59 risto Exp $
 */

/*********** size constants *************/

/* These constants define the maximum sizes of tables that hold data
   and the network weights and representations */
#define MAXREP 12		/* max size of lexical & semantic reps */
#define MAXSENT 7		/* max number of sentences in a story */
#define MAXSEQ 8		/* max number of words in a sentence */
#define MAXCASE 6		/* max number of cases in sentence rep */
#define MAXSLOT 7		/* max number of slots in story rep */

#define MAXIOREP ((MAXCASE + MAXSLOT) * MAXREP) /* max size of proc IO layers*/
#define MAXHIDREP 100		/* max size of proc hidden layers */
#define MAXLSNET 20		/* max size of lexicon feature maps (side) */
#define MAXTOPNET 2		/* max top-level feature map size (side) */
#define MAXMIDNET 2		/* max mid-level feature map size (side) */
#define MAXBOTNET 8		/* max bottom-level map size (side) */
#define MAXFMLABELS 2		/* max # of displayed labels per map unit */

#define MAXWORDS 300		/* max number of lexical & semantic words */
#define MAXWORDL 30		/* max length of input words (chars) */
#define MAXFILENAMEL 100	/* max length of file names (chars) */
#define MAXSTRL 1000		/* max length of input lines (chars) */

/* system constants (not maxsize but actual size) */
#define NMODULES 12		/* 6 procnets + store + ret + l + s + ls + sl*/
#define NPROCMODULES 6		/* 6 procnets */


/*********** module, parameter, return, and range constants *************/

/* Module numbers are used to index data structures where module-specific
   data such as input/output activation, weights, graphics values are kept.
   The processing modules need to be consequtive before memory modules,
   which can appear in any order */
#define SENTPARSMOD 0		/* module # for sentence parser */
#define STORYPARSMOD 1		/* module # for story parser */
#define STORYGENMOD 2		/* module # for story generator */
#define SENTGENMOD 3		/* module # for sentence generator */
#define CUEFORMMOD 4		/* module # for cue former */
#define ANSWERPRODMOD 5		/* module # for answer producer */
#define STOREMOD 6		/* module # for storing into episodic mem */
#define RETMOD 7		/* module # for retrieving from episodic mem */
#define LINPMOD 8		/* module # for lexical input map  */
#define SOUTMOD 9		/* module # for semantic output map  */
#define SINPMOD 10		/* module # for semantic input map  */
#define LOUTMOD 11		/* module # for lexical output map  */
#define HFMWINMOD STOREMOD	/* module # for hfm window */
#define LEXWINMOD LINPMOD	/* module # for lex window */
#define SEMWINMOD SINPMOD	/* module # for sem window */

/* statistics tables are indexed by the task */
#define MAXTASK 2		/* max # of tasks (para, qa) */
#define PARATASK 0		/* index to task table */
#define QATASK 1		/* index to task table */

/* initialization, index, return code constants */
#define LARGEINT 999999999	/* used to init bestcounts */
#define LARGEFLOAT 999999999.9	/* used to initialize bestvalues */
#define NOPREV (-1)		/* no previous value on the map */
#define BLANKINDEX (-1)		/* index for the blank word */
#define TRUE 1
#define FALSE 0
#define NONE (-2)		/* indicates no value; note -1 = BLANKINDEX */
#define NOT_USED 0		/* this parameter is not in use */
#define REQUIRED 1		/* whether to require a keyword */
#define NOT_REQUIRED 0		/* or not */

/* text constants for log output */
#define BEGIN_COMMENT "[ "	/* comments are enclosed in these */
#define END_COMMENT " ]"
#define INP_SLOTS "slots"	/* message when # of slots is wrong */
#define DISCERNPROMPT "DISCERN> "/* default prompt */

/* exit codes; 1 is used by the system */
#define EXIT_NORMAL 0
#define EXIT_SIZE_ERROR 2
#define EXIT_DATA_ERROR 3
#define EXIT_COMMAND_ERROR 4
#define EXIT_FILE_ERROR 5
#define EXIT_X_ERROR 6


/*********** data types *************/

/* the lexicon data structure: an array of word strings and reps */
typedef struct WORDSTRUCT
  {
    char chars[MAXWORDL + 1];
    double rep[MAXREP];
  }
WORDSTRUCT;

/* one sentence */
typedef struct SENTSTRUCT
  {
    int words[MAXSEQ],
      caseroles[MAXCASE],
      included,			/* whether to include this in input */
      nseq;			/* number of words in the sentence */
  }
SENTSTRUCT;

/* story data */
typedef struct STORYSTRUCT
  {
    int slots[MAXSLOT];		/* slot-filler representation */
    struct SENTSTRUCT
      sents[MAXSENT];		/* sentences */
    int nsent;			/* number of sentences */
  }
STORYSTRUCT;

/* question and answer data */
typedef struct QASTRUCT
  {
    int slots[MAXSLOT];		/* target story representation */
    struct SENTSTRUCT
      question, answer;		/* question and answer sentences */
  }
QASTRUCT;

/* definition for feature map units in lexical maps and hfm */
/* the comp is a little wasteful for lexicon maps */
typedef struct FMUNIT
  {
    /* previous value is needed to see if display needs updating */
    float value,
      prevvalue;		/* activation value, previous activ. */
    double comp[MAXSLOT * MAXREP];/* input weights */
    int labelcount;		/* how many inputs mapped on the unit */
    char labels[MAXFMLABELS][MAXWORDL + 1];/* the labels of those inputs */
  }
FMUNIT;

/* holds the indices of the image units, and trace activation */
typedef struct IMAGEDATA
  {
    int topi, topj,		/* top level */
    midi, midj,			/* middle */
    boti, botj;			/* bottom */
    float value;		/* activation of the trace */
  }
IMAGEDATA;


/************ graphics definitions *************/

/* separation of labels and unit values */
#define ABOVE  (-asmboxhght)	/* distance of labels from units */
#define ABOVE2 (-net[modi].uhght-asmboxhght)	/* same when beyond a layer */
#define BELOW  net[modi].uhght	/* distance of labels from units */
#define BELOW2 (2*net[modi].uhght)/* same when beyond a layer */

/* geometry of a network display */
typedef struct NETSTRUCT
  {
    int
      width,			/* window width and height */
      height,
      uwidth,			/* unit width and height */
      uhght,
      midwidth,			/* hfm middle-level unit width and height */
      midhght,
      botwidth,			/* hfm bottom-level unit width and height */
      bothght,
      marg,			/* left margin */
      inpx,			/* left side of input layer */
      inp1x,			/* left side of second input layer (answprod)*/
      prevx,			/* left side of prev hid layer */
      hidx,			/* left side of hidden layer */
      outx,			/* left size of output layer */
      tgtx,			/* left side of target layer */
      inpy,			/* top of input layer */
      inp1y,			/* top of second input layer (answerprod) */
      prevy,			/* top of previous hidden layer */
      hidy,			/* top of hidden layer */
      outy,			/* top of output layer */
      tgty,			/* top of target layer */
      hsp,			/* separation of assemblies */
      columns;			/* max number of assemblies */
    char sequence[MAXSEQ * (2 * MAXWORDL + 5) + 1], /* I/O word sequence */
      newitem[2 * MAXWORDL + 5 + 1];	/* new item in that sequence */
    char log[MAXSTRL + 1];	/* current I/O item and error */
  }
NETSTRUCT;

/* the resource data for the X display */
typedef struct RESOURCE_DATA
  {
    Boolean bringupdisplay;	/* whether to start display */
    int delay;			/* seconds to sleep in major simulation steps*/
    float tracelinescale,	/* scaling of lateral weight lengths */
      tracewidthscale,		/* scaling of lateral weight widths */
      reversevalue;		/* threshold for reversing color */
    Dimension netwidth,		/* width of network widgets */
      procnetheight,
      hfmnetheight,
      lexnetheight;		/* height of the widgets */
    Boolean owncmap;		/* use private colormap */
    Pixel textColor,		/* color of the text on display */
      latweightColor,		/* color of trace map lateral wghts */
      netColor;			/* color of network outlines */
    String commandfont,		/* command line */
      titlefont,		/* network title */
      logfont,			/* I/O item and error */
      asmfont,			/* words in the assemblies */
      asmerrorfont,		/* erroneous words in the assemblies */
      hfmfont,			/* top and middle level labels */
      tracefont,		/* bottom level labels */
      lexfont,
      semfont;			/* lexical and semantic map labels */
    Boolean help;		/* user help command line option */
  }
RESOURCE_DATA, *RESOURCE_DATA_PTR;


/************ funtion prototype macros *************/

#if __STDC__
#define __P(a) a
#else
#ifndef __P			/* Sun ANSI compiler defines this */
#define __P(a) ()
#endif
#endif
