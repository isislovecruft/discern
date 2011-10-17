/* File: hfm.c
 *
 * Hierarchical feature maps in DISCERN
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: hfm.c,v 1.37 1994/09/20 10:46:59 risto Exp $
 */

#include <stdio.h>
#include <math.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "defs.h"
#include "globals.c"


/************ hfm constants *************/

/* keywords expected in hfmsimu */
#define HFMSIMU_WEIGHTS "network-weights"/* weights in hfmsimu */
#define HFMSIMU_INDICES "input-indices"	/* compression indices in hfmsimu */
#define HFMSIMU_NTOPNET "topmapsize"	/* top-level map size in hfmsimu */
#define HFMSIMU_NMIDNET "midmapsize"	/* middle-level map size */
#define HFMSIMU_NBOTNET "botmapsize"	/* bottom-level map size */


/******************** Function prototypes ************************/

/* global functions */
#include "prototypes.h"

/* functions local to this file */
static void classify_and_store __P ((int modi, IMAGEDATA *image));
static void classify_and_retrieve __P ((int modi, IMAGEDATA *image));
static void top_responses __P ((int modi, double *best, double *worst,
				IMAGEDATA *image));
static void middle_responses __P ((int modi, double *best, double *worst,
				   IMAGEDATA *image));
static void bottom_responses __P ((int modi, double *best, double *worst,
				  IMAGEDATA *image));
static void form_inprep_targets __P ((int modi, int slots[]));
static void form_outrep __P ((int modi, IMAGEDATA image));
static void bottom_retrieve __P ((int modi, int topi, int topj,
				 int midi, int midj, IMAGEDATA *searchimage));
static void hfm_iterate_weights __P((void (*dofun) (FILE *, double *,
						    double, double),
				     FILE *fp, double par1, double par2));


/*********************  memory classification ***************************/

void
presentmem (taski, modi, slots, bestvalue)
/* present a story rep vector to the memory, find the image units,
   and either store the vector or retrieve the stored pattern */
     int taski;				/* para or qa */
     int modi;				/* store or retrieve */
     int slots[];			/* target indices */
     double *bestvalue;			/* activation of max resp unit */
{
  IMAGEDATA image;			/* image unit indices & value */
  int i;

  /* we don't allow mouse events during propagation */
  nohfmmouseevents = TRUE;
  
  /* either get the input vector from sentpars or cueformer output,
     or if the modules are not chained, form it from the targets */
  form_inprep_targets (modi, slots);

  /* store the story representation in the memory */
  if (modi == STOREMOD)
    /* find the image units and store the trace at the bottom map */
    classify_and_store (STOREMOD, &image);
  /* retrieve the trace using input vector as a cue */
  else if (modi == RETMOD)
    classify_and_retrieve (RETMOD, &image);

  /* form the outputvector from the weights of the image units */
  form_outrep (modi, image);
  /* collect accuracy statistics unless the retrieval was unsuccessful */
  if (!(modi == RETMOD && image.value < aliveact))
    collect_stats (taski, modi);

  /* allow mouse events again */
  nohfmmouseevents = FALSE;

  /* display the log line */
  if (displaying)
    {
      display_hfm_error (modi, image);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }
  
  /* if modules are in a chain, establish the result to be used
     for the answerprod or storygen */
  if (chain)
    for (i = 0; i < nslotrep; i++)
      slotrep[i] = outrep[modi][i];
  *bestvalue=image.value;
}


static void
classify_and_store (modi, image)
/* find the image units at the three levels and store the trace 
   at the bottom level map */
  int modi;			/* module number: store or retrieve */
  IMAGEDATA *image;		/* image unit indices & value */
{
  /* find the image units at the three levels */
  classify (modi, image);

  /* create a trace on the appropriate trace map */
  store_trace (*image,
	       botunits[image->topi][image->topj][image->midi][image->midj],
	       latweights[image->topi][image->topj][image->midi][image->midj],
	       nbotindices[image->topi][image->topj][image->midi][image->midj],
	       botindices[image->topi][image->topj][image->midi][image->midj]);
}


void
classify (modi, image)
/* find the image units at the three levels
   (needs to be a separate routine for determining the labels) */
  int modi;			/* module number: store or retrieve */
  IMAGEDATA *image;		/* image unit indices & value */
{
  double foo1, foo2;		/* best and worst response (not used) */

  printcomment ("\n", "storing into episodic memory:", "");

  /* clear and display the hfm system */
  if (displaying)
    {
      hfm_clear_values ();	/* update prevvalues, clear values */
      display_hfm_values ();	/* display the cleared values */
                                /* (those that weren't clear already, i.e.
				   differ from prevvalues) */
    }

  /* compute responses of the top-level units, display them,
     and find indices of the image unit and return them in image */
  top_responses (modi, &foo1, &foo2, image);
  
  /* compute responses of the middle-level units in the winning map
     and find indices of the image unit and return them in image */
  middle_responses (modi, &foo1, &foo2, image);

  /* compute responses of the bottom-level units in the winning map 
     and find indices of the image unit and return them in image */
  bottom_responses (modi, &foo1, &foo2, image);
}


static void
classify_and_retrieve (modi, image)
/* look for the appropriate trace under all maps that are close enough
   to the cue vector, return the indices to the best one and the 
   trace activation in image */
  int modi;			/* module number: store or retrieve */
  IMAGEDATA *image;		/* image unit indices & value */
{
  int topi, topj, midi, midj;	/* indices for the top and middle maps */
  double topbest, topworst;	/* max and min responding top-level units */
  double midbest, midworst;	/* max and min responding mid-level units */
  IMAGEDATA searchimage;	/* image during search in the different maps */
  
  /* start with an impossibly weak best activation */
  searchimage.value = (-2.0);

  printcomment ("\n", "retrieving from episodic memory:", "");

  /* first clear the map activations */
  hfm_clear_values ();
  if (displaying)
    display_hfm_values ();

  /* compute responses of the top-level units, display them,
     and find best and worst response values (indices not used) */
  top_responses (modi, &topbest, &topworst, image);

  /* look for traces under all top-level units that have activation
     larger than topsearch threshold and have lines to their middle map */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      if (topworst - (topworst - topbest) * topunits[topi][topj].value
	  < topsearch && nmidindices[topi][topj] > 0)
	{

	  /* compute responses of the middle-level map i, j
	     and find best and worst response values (indices not used) */
	  middle_responses (modi, &midbest, &midworst, image);

	  /* look for traces under all mid-level units midi, midj of map i, j,
	     that have activation larger than midsearch threshold
	     and have lines to their bottom-level map */
	  for (midi = 0; midi < nmidnet; midi++)
	    for (midj = 0; midj < nmidnet; midj++)
	      if (midworst - (midworst - midbest) *
		             midunits[topi][topj][midi][midj].value
		  < midsearch && nbotindices[topi][topj][midi][midj] > 0)
		/* try to retrieve a trace from map [topi][topj][midi][midj] */
		/* if the trace value is better, return indices and
		   activation in searchimage */
		bottom_retrieve (modi, topi, topj, midi, midj, &searchimage);
	}
  /* make the best value found in the above search global */
  *image = searchimage;
}


/**********************  map responses  **************************/

static void
top_responses (modi, best, worst, image)
  /* compute responses of the top-level units
     and find max and min responding units, returning their values
     in best and worst, and best indices in image */
  int modi;			/* module number: store or retrieve */
  double *best, *worst;		/* best and worst response found so far */
  IMAGEDATA *image;		/* image unit indices & value */
{
  int topi, topj;		/* map indices */

  /* first initialize the current best and worst values */
  *best = LARGEFLOAT;
  *worst = (-1.0);
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      {
	/* response proportional to the distance of weight and input vectors */
	distanceresponse (&topunits[topi][topj], inprep[modi],
			  ninprep[modi], distance, NULL);
	/* find best and worst activations and best indices */
	updatebestworst (best, worst, &image->topi, &image->topj,
			 &topunits[topi][topj],
			 topi, topj, fsmaller, fgreater);
      }
  
  /* scale the responses linearly between 1.0 (best) and 0.0 (worst) */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      topunits[topi][topj].value =
	(*worst - topunits[topi][topj].value) / (*worst - *best);

  /* display the responses of the top-level map */
  if (displaying)
    {
      display_hfm_top ();
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }
}


static void
middle_responses (modi, best, worst, image)
  /* compute responses of the units in the winning middle-level map
     and find max and min responding units, returning their values
     in best and worst, and best indices in image */
  int modi;			/* module number: store or retrieve */
  double *best, *worst;		/* best and worst response found so far */
  IMAGEDATA *image;		/* image unit indices & value */
{
  int midi, midj;		/* map indices */

  /* first initialize the current best and worst values */
  *best = LARGEFLOAT;
  *worst = (-1.0);
  for (midi = 0; midi < nmidnet; midi++)
    for (midj = 0; midj < nmidnet; midj++)
      {
	/* response proportional to the distance of weight and input vectors */
	distanceresponse (&midunits[image->topi][image->topj][midi][midj],
			  inprep[modi],
			  nmidindices[image->topi][image->topj], seldistance, 
			  midindices[image->topi][image->topj]);
	/* find best and worst activations and best indices */
	updatebestworst (best, worst, &image->midi, &image->midj,
			 &midunits[image->topi][image->topj][midi][midj],
			 midi, midj,
			 fsmaller, fgreater);
      }
  
  /* scale the responses linearly between 1.0 (best) and 0.0 (worst) */
  for (midi = 0; midi < nmidnet; midi++)
    for (midj = 0; midj < nmidnet; midj++)
      midunits[image->topi][image->topj][midi][midj].value =
	(*worst - midunits[image->topi][image->topj][midi][midj].value) /
	  (*worst - *best);

  /* display the responses for the middle-level map */
  if (displaying)
    {
      display_hfm_mid (image->topi, image->topj);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

}


static void
bottom_responses (modi, best, foo, image)
  /* compute responses of the units in the winning bottom-level map
     and find max and min responding units, returning the value of the 
     max in best, and best indices in image */
  int modi;			/* module number: store or retrieve */
  double *best, *foo;		/* best and worst response (not used) */
  IMAGEDATA *image;		/* image unit indices & value */
{
  int boti, botj;		/* map indices */

  /* first initialize the current best and worst values */
  *best = LARGEFLOAT;
  *foo = LARGEFLOAT;
  for (boti = 0; boti < nbotnet; boti++)
    for (botj = 0; botj < nbotnet; botj++)
      {
	/* response proportional to the distance of weight and input vectors */
	distanceresponse (&botunits[image->topi][image->topj]
			           [image->midi][image->midj]
			           [boti][botj],
			  inprep[modi],
			  nbotindices[image->topi][image->topj]
			           [image->midi][image->midj],
			  seldistance,
			  botindices[image->topi][image->topj]
			            [image->midi][image->midj]);
	/* find best and worst activations and best indices */
	updatebestworst (best, foo, &image->boti, &image->botj,
			 &botunits[image->topi][image->topj]
			          [image->midi][image->midj]
			          [boti][botj],
			 boti, botj,
			 fsmaller, fgreater);
      }
  /* the bottom-level values will be scaled and displayed with the trace */
}


static void
bottom_retrieve (modi, topi, topj, midi, midj, searchimage)
/* try to retrieve a trace from the map [topi][topj][midi][midj]; if its value
   is larger than best value found so far, return its value and indices */
     int modi;				/* module number: store or retrieve */
     int topi, topj, midi, midj;	/* map indices */
     IMAGEDATA *searchimage;            /* trace return indices and value */
{
  int boti, botj;			/* return indices of the trace */
  double value;				/* return trace value */
  char s[MAXSTRL + 1];

  /* print out which map is currently under consideration */
  sprintf (s, "\n%smap (%d,%d), (%d,%d):",
	   BEGIN_COMMENT, topi, topj, midi, midj);
  if (babbling)
    printf ("%s", s);

  /* find a trace in map [topi][topj][midi][midj], returning its value
     in value and indices in boti and botj */
  retrieve_trace (topi, topj, midi, midj, &boti, &botj, &value,
		  botunits[topi][topj][midi][midj],
		  latweights[topi][topj][midi][midj],
		  nbotindices[topi][topj][midi][midj],
		  botindices[topi][topj][midi][midj],
		  inprep[modi]);
  /* close the log output after retrieve result (printed under trace) */
  if (babbling)
    printf ("%s", END_COMMENT);

  /* if this trace has a better value than the best trace so far
     (searchimage->value), return its indices and value */
  if (value > searchimage->value)
    {
      searchimage->topi = topi;
      searchimage->topj = topj;
      searchimage->midi = midi;
      searchimage->midj = midj;
      searchimage->boti = boti;
      searchimage->botj = botj;
      searchimage->value = value;
    }
}


/******************  forming input and output vectors  ********************/

static void
form_inprep_targets (modi, slots)
/* form an input vector and targets for the hfm system */
  int modi;				/* module number: store or retrieve */
  int slots[];				/* target indices */
{
  int i, j;				/* component counters */

  /* if input comes from the storypars or cueformer, use current storyrep */
  if (chain)
    for (i = 0; i < nslotrep; i++)
      inprep[modi][i] = slotrep[i];
  /* form the input vector from correct targets (from the lexicon) */
  else
    for (i = 0; i < ninputs[modi]; i++)
      for (j = 0; j < nsrep; j++)
	inprep[modi][i * nsrep + j] = swords[slots[i]].rep[j];

  /* form the targets */
  for (i = 0; i < ninputs[modi]; i++)
    {
      targets[modi][i] = slots[i];	
      for (j = 0; j < nsrep; j++)
	tgtrep[modi][i * nsrep + j] = swords[slots[i]].rep[j];
    }
}


static void
form_outrep (modi, image)
/* form the final output vector of the map from the input weights
   of the image units */
  int modi;				/* module number: store or retrieve */
  IMAGEDATA image;			/* image unit indices & value */
{
  int i, babbling_save;
  char s[MAXSTRL + 1];

  /* no trace was found in retrieval */
  if (modi == RETMOD && image.value < aliveact)
    {
      if (babbling || print_mistakes)
	{
	  /* this is actually a mistake, but it is reported through a comment;
	     comments are printed only when babbling,
	     so force babbling for this while */
	  babbling_save = babbling;
	  babbling = TRUE;
	  printcomment ("\n", "oops: no image found", "\n");
	  babbling = babbling_save;
	}
      for (i = 0; i < noutrep[modi]; i++)
	outrep[modi][i] = 0.0;		/* no output vector generated */
    }
  /* a good trace was found, or vector was stored */
  else
    {
      /* report the image unit indices */
      sprintf (s, "image units (%d,%d), (%d,%d), (%d,%d):",
	       image.topi, image.topj, image.midi, image.midj,
	       image.boti, image.botj);
      printcomment ("\n", s, "\n");

      /* collect the output vector from the input weights of the image units */
      for (i = 0; i < noutrep[modi]; i++)
	outrep[modi][i] = topunits[image.topi][image.topj].comp[i];
      for (i = 0; i < nmidindices[image.topi][image.topj]; i++)
	outrep[modi][midindices[image.topi][image.topj][i]] =
	  midunits[image.topi][image.topj][image.midi][image.midj].comp[i];
      for (i = 0;
	   i < nbotindices[image.topi][image.topj][image.midi][image.midj];
	   i++)
	outrep[modi][botindices[image.topi][image.topj]
		               [image.midi][image.midj][i]] =
			      botunits[image.topi][image.topj]
				      [image.midi][image.midj]
				      [image.boti][image.botj].comp[i];
    }
}


/*********************  initializations ******************************/

void
hfm_init ()
/* initialize the episodic memory */
{
  int topi, topj, midi, midj, k;
  FILE *fp;

  /* set up numbers of input/output words and reps for convenience */
  ninputs[STOREMOD] = noutputs[STOREMOD] = nslot;
  ninputs[RETMOD] = noutputs[RETMOD] = nslot;
  ninprep[STOREMOD] = noutrep[STOREMOD] = nslot * nsrep;
  ninprep[RETMOD] = noutrep[RETMOD] = nslot *nsrep;

  /* open the weight file */
  printf ("Reading hierarchical feature maps from %s...", hfmfile);
  open_file (hfmfile, "r", &fp, REQUIRED);

  /* ignore everything until the net sizes */
  read_till_keyword (fp, HFMSIMU_NTOPNET, REQUIRED);
  fscanf (fp, "%d", &ntopnet);
  if (ntopnet > MAXTOPNET || ntopnet <= 0)
    {
      fprintf (stderr, "%s %s exceed array size\n", hfmfile, HFMSIMU_NTOPNET);
      exit (EXIT_SIZE_ERROR);
    }
  read_till_keyword (fp, HFMSIMU_NMIDNET, REQUIRED);
  fscanf (fp, "%d", &nmidnet);
  if (nmidnet > MAXMIDNET || nmidnet <= 0)
    {
      fprintf (stderr, "%s %s exceed array size\n", hfmfile, HFMSIMU_NMIDNET);
      exit (EXIT_SIZE_ERROR);
    }
  read_till_keyword (fp, HFMSIMU_NBOTNET, REQUIRED);
  fscanf (fp, "%d", &nbotnet);
  if (nbotnet > MAXBOTNET || nbotnet <= 0)
    {
      fprintf (stderr, "%s %s exceed array size\n", hfmfile, HFMSIMU_NBOTNET);
      exit (EXIT_SIZE_ERROR);
    }
  /* then ignore everything until the weights */
  read_till_keyword (fp, HFMSIMU_WEIGHTS, REQUIRED);
  /* read the weights */
  hfm_iterate_weights (readfun, fp, NOT_USED, NOT_USED);
  /* then ignore everything until the indices */
  read_till_keyword (fp, HFMSIMU_INDICES, REQUIRED);
  /* read the mid-level indices */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      {
	fscanf (fp, "%d", &nmidindices[topi][topj]);
	for (k = 0; k < nmidindices[topi][topj]; k++)
	  if (fscanf (fp, "%d", &midindices[topi][topj][k]) != 1)
	    {
	      fprintf (stderr, "Error reading mid-level indices\n");
	      exit (EXIT_DATA_ERROR);
	    }
      }
  /* read the bottom-level indices */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      for (midi = 0; midi < nmidnet; midi++)
	for (midj = 0; midj < nmidnet; midj++)
	  {
	    fscanf (fp, "%d\n", &nbotindices[topi][topj][midi][midj]);
	    for (k = 0; k < nbotindices[topi][topj][midi][midj]; k++)
	      if (fscanf (fp, "%d\n",
			  &botindices[topi][topj][midi][midj][k]) != 1)
		{
		  fprintf (stderr,
			   "Error reading bottom-level indices\n");
		  exit (EXIT_DATA_ERROR);
		}
	  }
  printf ("Done.\n");
}


static void
hfm_iterate_weights (dofun, fp, par1, par2)
/* read the weights from the file */
     void (*dofun) __P((FILE *, double *, double, double)); /* read function */
     FILE *fp;
     double par1, par2;			/* not used (in reading weights) */
{
  int topi, topj, midi, midj, boti, botj, k;

  /* top-level input weights */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      for (k = 0; k < nslotrep; k++)
	(*dofun) (fp, &topunits[topi][topj].comp[k], par1, par2);

  /* middle-level input weights */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      for (midi = 0; midi < nmidnet; midi++)
	for (midj = 0; midj < nmidnet; midj++)
	  for (k = 0; k < nslotrep; k++)
	    (*dofun) (fp, &midunits[topi][topj][midi][midj].comp[k],
		      par1, par2);

  /* bottom-level input weights */
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      for (midi = 0; midi < nmidnet; midi++)
	for (midj = 0; midj < nmidnet; midj++)
	  for (boti = 0; boti < nbotnet; boti++)
	    for (botj = 0; botj < nbotnet; botj++)
	      for (k = 0; k < nslotrep; k++)
		(*dofun) (fp, &botunits[topi][topj]
			               [midi][midj]
			               [boti][botj].comp[k],
			  par1, par2);
}


void
hfm_clear_values ()
/* clear the lexicon activations */
{
  int topi, topj, midi, midj, boti, botj;

  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      {
	topunits[topi][topj].prevvalue = topunits[topi][topj].value;
	topunits[topi][topj].value = 0.0;
	for (midi = 0; midi < nmidnet; midi++)
	  for (midj = 0; midj < nmidnet; midj++)
	    {
	      midunits[topi][topj][midi][midj].prevvalue
		= midunits[topi][topj][midi][midj].value;
	      midunits[topi][topj][midi][midj].value = 0.0;
	      for (boti = 0; boti < nbotnet; boti++)
		for (botj = 0; botj < nbotnet; botj++)
		  {
		    botunits[topi][topj][midi][midj][boti][botj].prevvalue =
		      botunits[topi][topj][midi][midj][boti][botj].value;
		    botunits[topi][topj][midi][midj][boti][botj].value = 0.0;
		  }
	    }
      }
}


void
hfm_clear_prevvalues ()
/* reset the previous activations to NOPREV, so that the whole hfm system
   will be displayed (because prevvalues are different to current values) */
{
  int topi, topj, midi, midj, boti, botj;

  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      {
	topunits[topi][topj].prevvalue = NOPREV;
	for (midi = 0; midi < nmidnet; midi++)
	  for (midj = 0; midj < nmidnet; midj++)
	    {
	      midunits[topi][topj][midi][midj].prevvalue = NOPREV;
	      for (boti = 0; boti < nbotnet; boti++)
		for (botj = 0; botj < nbotnet; botj++)
		  botunits[topi][topj][midi][midj][boti][botj].prevvalue
		    = NOPREV;
	    }
      }
}


void
hfm_clear_labels ()
/* clear map labels */
{
  int topi, topj, midi, midj, boti, botj;
  for (topi = 0; topi < ntopnet; topi++)
    for (topj = 0; topj < ntopnet; topj++)
      {
	topunits[topi][topj].labelcount = 0;
	for (midi = 0; midi < nmidnet; midi++)
	  for (midj = 0; midj < nmidnet; midj++)
	    {
	      midunits[topi][topj][midi][midj].labelcount = 0;
	      for (boti = 0; boti < nbotnet; boti++)
		for (botj = 0; botj < nbotnet; botj++)
		  botunits[topi][topj][midi][midj][boti][botj].labelcount = 0;
	    }
      }
}
