/* File: stats.c
 *
 * Statistics initialization, collection, and output routines for DISCERN.
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: stats.c,v 1.6 1994/09/05 21:46:38 risto Exp $
 */

#include <stdio.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "defs.h"
#include "globals.c"


/************ statistics constants *************/

/* text constants for log output */
#define BEGIN_ASSEMBLY "| "	/* assemblies are enclosed in these */
#define END_ASSEMBLY "|\n"
#define LEXICON_TRANS ">"	/* indicates lexicon translation */


/******************** function prototypes ************************/

/* global functions */
#include "prototypes.h"

/* functions local to this file */
static void print_task_stats __P((char taskname[], int taski));
static double reszero __P ((double num, int den));


/******************** static variables ************************/

/* statistics */
static int 
  within[MAXTASK][NMODULES],	/* units close enough */
  allarray[MAXTASK][NMODULES][MAXWORDS + 1],
 *all[MAXTASK][NMODULES],	/* # of all words output; [-1]=blank */
  corrarray[MAXTASK][NMODULES][MAXWORDS + 1],
 *corr[MAXTASK][NMODULES];	/* # of words output correctly; [-1]=blank */
static double deltasum[MAXTASK][NMODULES]; /* cumulative error sum */


/*********************  initializations ******************************/

void
init_stats_blanks ()
/* initialize the pointers to statistics tables so that the first space 
   can be used for the blank with index [-1] */
{
  int i, j;
  for (i = 0; i < MAXTASK; i++)
    for (j = 0; j < NMODULES; j++)
      {
	all[i][j] = allarray[i][j] + 1;
	corr[i][j] = corrarray[i][j] + 1;
      }
}


void
init_stats ()
/* initialize the statistics tables */
{
  int modi,			/* module number */
    taski,			/* task number (para or qa) */
    i;

  /* separate stats for parsing+paraphrasing stories and question answering */
  for (taski = 0; taski < MAXTASK; taski++)
    for (modi = 0; modi < NMODULES; modi++)
      {
	deltasum[taski][modi] = 0.0;
	within[taski][modi] = 0;
	/* there is a different number of lexical vs. semantic words */
	for (i = BLANKINDEX;
	     i < ((modi == LINPMOD || modi == LOUTMOD) ? nlwords : nswords);
	     i++)
	  corr[taski][modi][i] = all[taski][modi][i] = 0;
      }
}


/*********************  cumulation ******************************/

void
collect_stats (taski, modi)
/* the main routine for accumulating performance statistics */
/* all modules call up this routine after propagation */
     int taski;			/* para or qa */
     int modi;			/* module number */
{
  int i, nearest;
  double uniterror;		/* error of this unit */
  char s[MAXSTRL + 1], ss[MAXSTRL + 1];
  WORDSTRUCT *words;		/* lexicon (lexical or semantic) */
  int nrep;			/* rep dimension (lexical or semantic) */
  int nwords;			/* number of words (lexical or semantic) */

  /* first select the right lexicon */
  select_lexicon (modi, &words, &nrep, &nwords);
  
  /* cumulate error for each output unit */
  for (i = 0; i < noutrep[modi]; i++)
    {
      /* calculate the absolute error of this unit */
      uniterror = fabs (tgtrep[modi][i] - outrep[modi][i]);
      /* cumulate error (for average) */
      deltasum[taski][modi] += uniterror;
      /* cumulate number of "correct" units (close enough to correct) */
      if (uniterror < withinerr)
	within[taski][modi]++;
    }

  /* cumulate word-based error counts */
  /* start an assembly in the log output */
  if (babbling && modi != SENTGENMOD && modi < LINPMOD)
    printf (BEGIN_ASSEMBLY);
  for (i = 0; i < noutputs[modi]; i++)
    {
      all[taski][modi][targets[modi][i]]++;/* cumulate total number of words */
      /* find the representation in the lexicon closest to the output */
      nearest = find_nearest (&outrep[modi][i * nrep], words, nrep, nwords);
      if (nearest == targets[modi][i])
	/* it was the right one; update the correct count for this word */
	corr[taski][modi][targets[modi][i]]++;

      /* print the word at the log output */
      /* print if babbling, or if there was a mistake */
      /* print lexicon output only if also log_lexicon is on */
      if ((babbling && !(!log_lexicon && /* nearest==targets[modi][i] && */
		(modi == SOUTMOD || modi == SENTGENMOD || modi == SINPMOD)))
	  || (print_mistakes && nearest != targets[modi][i]))
	{
	  if (nearest == targets[modi][i] || text_question)
	    /* only print the actual word generated */
	    sprintf (s, "%s", words[nearest].chars);
	  else
	    {
	      /* print an error */
	      /* print module number if only errors logged */
	      if (print_mistakes && !babbling)
		sprintf (ss, "%d:", modi);
	      else
		sprintf (ss, "%s", "");
	      /* otherwise print both the output and the correct word */
	      sprintf (s, "*%s%s(%s)*",
		       ss, words[nearest].chars,
		       words[targets[modi][i]].chars);
	    }
	  /* the errors are printed on separate lines */
	  /* lexicon translation is indicated by LEXICON_TRANS string */
	  printf ("%s%s%s", s, (print_mistakes && !babbling) ? "\n" : "",
		  ((log_lexicon /*|| nearest!=targets[modi][i] */ ) &&
		   (modi == LINPMOD || modi == SENTGENMOD ||
		    modi == SINPMOD)) ? LEXICON_TRANS : " ");
	}
    }
  /* close with the string indicating assembly-based output */
  if (babbling && modi != SENTGENMOD && modi < LINPMOD)
    printf (END_ASSEMBLY);
}


/*********************  results  ******************************/

void
print_stats ()
/* print both paraphrasing and question answering statistics */
{
  print_task_stats ("Paraphrasing", PARATASK);
  print_task_stats ("Question answering", QATASK);
}


static void
print_task_stats (taskname, taski)
/* print out the performance statistics in the log output */
     char taskname[];		/* paraphrasing or qa */
     int taski;			/* same as a flag */
{
  int modi,			/* module number */
    nwords, nrep,		/* number of words and rep components */
    i,
    sum_corr, sum_all,		/* number of correct and all words */
    sum_corrinst, sum_allinst;	/* number of correct and allinstances */

  printf ("\n%s performance:\n", taskname);
  printf ("Module       All    Inst  <%.2f    AvgErr\n", withinerr);
  /* print all modules separately */
  for (modi = 0; modi < NMODULES; modi++)
    {
      /* first set the dimensions properly (lexical or semantic) */
      nwords = (modi == LINPMOD || modi == LOUTMOD) ? nlwords : nswords;
      nrep = (modi == LINPMOD || modi == LOUTMOD) ? nlrep : nsrep;
      sum_corr = sum_all = sum_corrinst = sum_allinst = 0;
      /* sum up all word counts and all correct word counts */
      for (i = BLANKINDEX; i < nwords; i++)
	{
	  sum_corr += corr[taski][modi][i];
	  sum_all += all[taski][modi][i];
	}
      /* sum up all instance word counts */
      for (i = 0; i < ninstances; i++)
	{
	  sum_corrinst += corr[taski][modi][instances[i]];
	  sum_allinst += all[taski][modi][instances[i]];
	}
      printf ("Module %2d: ", modi);
      printf ("%6.1f %6.1f %6.1f %9.4f\n",
	      /* percentage of correct words */
	      reszero (100.0 * sum_corr, sum_all),
	      /* percentage of correct instances */
	      reszero (100.0 * sum_corrinst, sum_allinst),
	      /* percentage of output units close enough */
	      reszero (100.0 * within[taski][modi], sum_all * nrep),
	      /* average error per output unit */
	      reszero (deltasum[taski][modi], sum_all * nrep));
    }
}


static double
reszero (num, den)
/* if no data was collected, should print 0 instead of crashing */
     double num;			/* numerator */
     int den;				/* denominator */
{
  if (den == 0)
    return (0.0);
  else
    return (num / (double) den);
}
