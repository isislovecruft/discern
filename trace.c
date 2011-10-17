/* File: trace.c
 *
 * Trace feature maps in DISCERN
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: trace.c,v 1.21 1994/09/05 21:46:38 risto Exp $
 */

#include <stdio.h>
#include <math.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "defs.h"
#include "globals.c"


/******************** Function prototypes ************************/

/* global functions */
#include "prototypes.h"

/* functions local to this file */
static void compute_lat_responses __P((FMUNIT table[MAXBOTNET][MAXBOTNET],
				       double lweights[MAXBOTNET][MAXBOTNET]
				                      [MAXBOTNET][MAXBOTNET],
				       int dim, int indices[],
				       int topi, int topj, int midi, int midj,
				       int *boti, int *botj,
				       double *value, double inpvector[]));
static double mem_unit_resp __P((int indices[],
				FMUNIT table[MAXBOTNET][MAXBOTNET],
				double lweights[MAXBOTNET][MAXBOTNET]
				               [MAXBOTNET][MAXBOTNET],
				int boti, int botj, int dim,
				double inpvector[]));
static double ext_resp __P((int indices[], double comp[], int dim,
			   double inpvector[]));
static double sigmoid __P((double activity));


/*******************  storing and retrieving traces  **********************/

void
store_trace (image, table, lweights, dim, indices)
/* create a trace around the given unit */
     IMAGEDATA image;                      /* image unit indices and value */
     FMUNIT table[MAXBOTNET][MAXBOTNET];   /* trace map and its lat weights */
     double lweights[MAXBOTNET][MAXBOTNET][MAXBOTNET][MAXBOTNET];
     int dim,		                   /* story rep dimension, */
     indices[];				   /* indices of active components */
{
  int i, j, ii, jj,			   /* indices of map units */
  centeri, centerj,			   /* neighborhood center */
  lowi, lowj, highi, highj, foo;	   /* neighborhood boundaries */
  double lowest, highest;		   /* of activations in the neighbhd */

  /* determine the weight change neighborhood: move it away from the boundary
     so that each trace is encoded with a full neighborhood */
  /* center of the neighborhood = image unit */
  centeri=image.boti;
  centerj=image.botj;
  /* move the center away from the boundary if necessary */
  centeri = clip(clip(centeri, tracenc, ige), nbotnet - 1 - tracenc, ile);
  centerj = clip(clip(centerj, tracenc, ige), nbotnet - 1 - tracenc, ile);
  /* neighborhood boundaries */
  lowi = centeri - tracenc;
  highi = centeri + tracenc;
  lowj = centerj - tracenc;
  highj = centerj + tracenc;

  /* find the highest and lowest activation in the neighborhood */
  lowest = LARGEFLOAT;
  highest = (-1.0);
  for (i = lowi; i <= highi; i++)
    for (j = lowj; j <= highj; j++)
      updatebestworst(&lowest, &highest, &foo, &foo, &table[i][j],
		      i, j, fsmaller, fgreater);

  /* scale the responses linearly between 1.0 (highest) and 0.0 (lowest) */
  for (i = 0; i < nbotnet; i++)
    for (j = 0; j < nbotnet; j++)
      if (i >= lowi && i <= highi && j >= lowj && j <= highj)
	table[i][j].value = 1.0 - (table[i][j].value - lowest)
	                          / (highest - lowest);
      else
	/* make all other activations 0 */
	table[i][j].value = 0.0;

  /* modify weights toward the input in a neighborhood of the maximally 
     responding unit */
  for (i = lowi; i <= highi; i++)
    for (j = lowj; j <= highj; j++)
      for (ii = 0; ii < nbotnet; ii++)
	for (jj = 0; jj < nbotnet; jj++)
	  /* make all weights from lower to at least epsilon higher activity,
	     and all weights to the unit itself, excitatory proportional
	     to the activity of the source unit */
	  if ((ii >= lowi && ii <= highi && jj >= lowj && jj <= highj) &&
	      ((table[ii][jj].value > table[i][j].value + epsilon) ||
	       (ii == i && jj == j)))
	    lweights[i][j][ii][jj] = gammaexc * table[i][j].value;
	  else
	  /* make all weights from higher to lower activity inhibitory,
	     proportional to the activity of the source unit */
	    lweights[i][j][ii][jj] = gammainh * table[i][j].value;

  /* display the activation on this trace map and the lateral connections */
  if (displaying)
    {
      display_hfm_bot (image.topi, image.topj, image.midi, image.midj);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }
}


void
retrieve_trace (topi, topj, midi, midj, boti, botj, value,
		table, lweights, dim, indices, inpvector)
/* find the center of the trace in the neighborhood of the cue input */
     int topi, topj, midi, midj;	/* indices of the map */
     int *boti, *botj;			/* return indices of the trace */
     double *value;			/* return value of the trace activ. */
     FMUNIT table[MAXBOTNET][MAXBOTNET]; /* trace map and its lat weights */
     double lweights[MAXBOTNET][MAXBOTNET][MAXBOTNET][MAXBOTNET];
     int dim,		                /* story rep dimension, */
     indices[];				/* indices of active components */
     double inpvector[];		/* the input story representation */
{
  int i, j,
  ts;					/* settling iteration counter */

  /* for each input, start with a blank network because the similarity
     measure is different from the one used in finding the image units */
  for (i = 0; i < nbotnet; i++)
    for (j = 0; j < nbotnet; j++)
      {
	table[i][j].prevvalue = NOPREV;
	table[i][j].value = 0.0;
      }
  /* display the blank trace map */
  if (displaying)
    {
      display_hfm_bot (topi, topj, midi, midj);
      wait_and_handle_events ();        /* stop if stepping,check for events */
    }

  /* as long as the highest activation is higher than aliveact threshold,
     settle the activation until the settling limit. If the activation drops 
     below aliveact (after having settled at least ts/2 iterations), abort
     the settling: no trace is found on the map */
  *value = 1.0;				/* to get the for loop started */
  for (ts = 1;
       (ts <= tsettle && *value >= aliveact) || ts <= tsettle / 2;
       ts++)
    compute_lat_responses (table, lweights, dim, indices, topi, topj,
			   midi, midj, boti, botj, value, inpvector);
}


/*******************  settling the response  **********************/

static void
compute_lat_responses (table, lweights, dim, indices, topi, topj,
			    midi, midj, boti, botj, value, inpvector)
/* compute the response as the sum of external and lateral activation */
     FMUNIT table[MAXBOTNET][MAXBOTNET]; /* trace map and its lat weights */
     double lweights[MAXBOTNET][MAXBOTNET][MAXBOTNET][MAXBOTNET];
     int dim,		                /* story rep dimension, */
     indices[],				/* indices of active components */
     topi, topj, midi, midj,	        /* indices of the map */
     *boti, *botj;			/* return indices of the trace */
     double *value;			/* return value of the trace */
     double inpvector[];		/* the input story representation */
{
  int i, j;
  double best = (-1.0);			/* activation of the max resp unit */

  /* copy the current response to be the previous response for propagation */
  for (i = 0; i < nbotnet; i++)
    for (j = 0; j < nbotnet; j++)
      table[i][j].prevvalue = table[i][j].value;

  /* compute the new responses for each unit */
  for (i = 0; i < nbotnet; i++)
    for (j = 0; j < nbotnet; j++)
      {
	/* response is the sigmoid of the sum of external and lateral actv. */
	table[i][j].value = sigmoid (mem_unit_resp (indices, table, lweights,
						    i, j, dim, inpvector));
	/* keep track of the max responding unit and its activation */
	if (table[i][j].value >= best)
	  {
	    *boti = i;
	    *botj = j;
	    best = table[i][j].value;
	  }
      }
  /* return the highest activation on the map (to decide whether there is
     a trace on the map at all */
  *value = best;
  if (babbling)
    printf (" ->(%d,%d):%.3f", *boti, *botj, best);
  /* display the current activation of the map */
  if (displaying)
    {
      display_hfm_bot (topi, topj, midi, midj);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }
}


static double
mem_unit_resp (indices, table, lweights, boti, botj, dim, inpvector)
/* compute the weighted sum of external and lateral activation for a unit */
     FMUNIT table[MAXBOTNET][MAXBOTNET]; /* trace map and its lat weights */
     double lweights[MAXBOTNET][MAXBOTNET][MAXBOTNET][MAXBOTNET];
     int dim,		                /* story rep dimension, */
     indices[],				/* indices of active components */
     boti, botj;			/* indices of this unit */
     double inpvector[];		/* the input story representation */
{
  int i, j;
  double resp = 0.0;			/* initially the sum = 0 */

  /* external input activation */
  resp = ext_resp (indices, table[boti][botj].comp, dim, inpvector);

  /* lateral activation */
  for (i = 0; i < nbotnet; i++)
    for (j = 0; j < nbotnet; j++)
      resp = resp + lweights[i][j][boti][botj] * table[i][j].prevvalue;
  return (resp);
}


static double
ext_resp (indices, comp, dim, inpvector)
/* compute the response of the unit to external input (before sigmoid) */
     int dim,		                /* story rep dimension, */
     indices[];				/* indices of active components */
     double comp[];			/* afferent weights */
     double inpvector[];		/* the input story representation */
{
  if (dim == 0)				/* in case this is an unused map */
    return (0.0);
  else
    /* the response is Euclidean distance scaled between 0 and 1 and
       inverted so that min distance returns 1 and max 0 */
    return (1.0 - seldistance (indices, inpvector, comp, dim) /
	    sqrt ((double) dim));
}


static double
sigmoid (activity)
  /* transform the activity to a sigmoid response between 0 and maximum */
     double activity;
{
  /* maxact determines the maximum, minact the shift along the threshold */
  return (1.0 / (1.0 + exp (maxact * (minact - activity))));
}


/*********************  initializations ******************************/

void
clear_traces ()
/* when no traces exist, all lateral weights are inhibitory */
{
  int topi, topj, midi, midj, boti, botj, lati, latj;

  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      for (midi = 0; midi < nmidnet; midi++)
	for (midj = 0; midj < nmidnet; midj++)
	  for (boti = 0; boti < nbotnet; boti++)
	    for (botj = 0; botj < nbotnet; botj++)
	      for (lati = 0; lati < nbotnet; lati++)
		for (latj = 0; latj < nbotnet; latj++)
		  latweights[topi][topj][midi][midj][boti][botj][lati][latj]
		    = gammainh;
}
