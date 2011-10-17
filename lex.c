/* File: lex.c
 *
 * The lexicon of DISCERN
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: lex.c,v 1.37 1994/09/20 10:46:59 risto Exp $
 */

#include <stdio.h>
#include <math.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "defs.h"
#include "globals.c"


/************ lexicon constants *************/

/* propagation */
#define ANC 1			/* response bubble radius */
#define PROPNC 4		/* number of units included in propagation */

/* keywords expected in lexsimu */
#define LEXSIMU_NLNET "lmapsize"		/* lexical map size */
#define LEXSIMU_NSNET "smapsize"		/* semantic map size */
#define LEXSIMU_WEIGHTS "network-weights"	/* weights */

/* list of units with highest activation;
   propagation takes place through these units */
typedef struct LEXPROPUNIT
  {
    int i, j;			/* index of the unit */
    float value;		/* activation value */
  }
LEXPROPUNIT;


/******************** Function prototypes ************************/

/* global functions */
#include "prototypes.h"

/* functions local to this file */
static void map_input __P((int taski, int modi,
			   FMUNIT inpunits[MAXLSNET][MAXLSNET], int ninpnet,
			   LEXPROPUNIT prop[], int *nprop));
static void associate __P((int taski, int modi,
			   FMUNIT inpunits[MAXLSNET][MAXLSNET],
			   LEXPROPUNIT prop[], int nprop,
			   FMUNIT outunits[MAXLSNET][MAXLSNET], int noutnet,
			   double assoc[MAXLSNET][MAXLSNET]
			               [MAXLSNET][MAXLSNET]));
static void assocresponse __P((FMUNIT *unit, int i, int j,
			       FMUNIT inpunits[MAXLSNET][MAXLSNET],
			       double assoc[MAXLSNET][MAXLSNET]
			                   [MAXLSNET][MAXLSNET],
			       LEXPROPUNIT prop[], int nprop));
static int propunit __P((int i, int j, LEXPROPUNIT prop[]));
static void findprop __P((int lowi, int lowj, int highi, int highj,
			  FMUNIT inpunits[MAXLSNET][MAXLSNET],
			  double *worst, LEXPROPUNIT prop[], int *nprop));
static void lex_iterate_weights __P((void (*dofun) (FILE *, double *,
						    double, double),
				     FILE *fp, double par1, double par2));


/*********************  lexicon translation  ******************************/

void
input_lexicon (taski, index)
/* translate a lexical input word into its semantic counterpart */
     int taski;				/* para or qa */
     int index;				/* index of the input word */
{
  int i;
  LEXPROPUNIT prop[PROPNC];       	/* propagation through these units */
  int nprop;				/* number of propunits */

  /* form the input and target */
  for (i = 0; i < ninprep[LINPMOD]; i++)
    inprep[LINPMOD][i] = tgtrep[LINPMOD][i] = lwords[index].rep[i];
  targets[LINPMOD][0] = index;

  /* find the image of the lexical input word on the lexical map */
  map_input(taski, LINPMOD, lunits, nlnet, prop, &nprop);

  /* form the input and target */
  for (i = 0; i < ninprep[SOUTMOD]; i++)
    tgtrep[SOUTMOD][i] = swords[index].rep[i];
  targets[SOUTMOD][0] = index;

  /* propagate lex map activation to the semantic map and find the image */
  associate (taski, SOUTMOD, lunits, prop, nprop, sunits, nsnet, lsassoc);

  /* form the input vector to sentpars */
  if (chain)
    for (i = 0; i < nsrep; i++)
      swordrep[i] = outrep[SOUTMOD][i];
}


void
output_lexicon (taski, index)
/* translate a semantic output word into its lexical counterpart */
     int taski;				/* para or qa */
     int index;				/* index of the output word */
{
  int i;
  LEXPROPUNIT prop[PROPNC];       	/* propagation through these units */
  int nprop;				/* number of propunits */

  /* form the input and target */
  if (chain)				/* take input from sentgen output */
    for (i = 0; i < ninprep[SINPMOD]; i++)
      {
	inprep[SINPMOD][i] = swordrep[i];
	tgtrep[SINPMOD][i] = swords[index].rep[i];
      }
  else					/* otherwise take it from lexicon */
    for (i = 0; i < ninprep[SINPMOD]; i++)
      inprep[SINPMOD][i] = tgtrep[SINPMOD][i] = swords[index].rep[i];
  targets[SINPMOD][0] = index;

  /* find the image of the semantic word on the semantic map */
  map_input(taski, SINPMOD, sunits, nsnet, prop, &nprop);

  /* form the input and target */
  for (i = 0; i < ninprep[LOUTMOD]; i++)
    tgtrep[LOUTMOD][i] = lwords[index].rep[i];
  targets[LOUTMOD][0] = index;

  /* propagate sem map activation to the lex map and find the image */
  associate (taski, LOUTMOD, sunits, prop, nprop, lunits, nlnet, slassoc);
}


static void
map_input (taski, modi, inpunits, ninpnet, prop, nprop)
/* present a word representation to the input map, find the image unit,
   define the propagation neighborhood, collect statistics, and display
   the map activation */
     int taski,				/* para or qa */
       modi;				/* module number */
     FMUNIT inpunits[MAXLSNET][MAXLSNET];/* map units */
     int ninpnet;			/* map size */
     LEXPROPUNIT prop[];	        /* propagation through these units */
     int *nprop;			/* number of propunits */
{
  int i, j;				/* indices to the map */
  double best,				/* activation of the image unit */
    worst;				/* act. of the min responding unit 
					   in neighborhood of the image unit */
  int besti, bestj;			/* indices of the image unit */

  /* we don't allow mouse events during propagation */
  nolexmouseevents = TRUE;

  /* compute responses of the units to the external input
     and find max and min responding units */
  best = LARGEFLOAT;
  worst = (-1.0);
  for (i = 0; i < ninpnet; i++)
    for (j = 0; j < ninpnet; j++)
      {
	/* response proportional to the distance of weight and input vectors */
	distanceresponse (&inpunits[i][j], inprep[modi], ninprep[modi],
			  distance, NULL);
	/* find best and worst and best indices */
	updatebestworst (&best, &worst, &besti, &bestj, &inpunits[i][j], i, j,
			 fsmaller, fgreater);
      }
  
  /* determine the propagation neighborhood: the PROPNC most highly active
     units within radius ANC from the image, clipped to the map boundaries;
     also record the weakest response in the neighborhood, count npropunits */
  findprop (clip(besti - ANC, 0, ige),
	    clip(bestj - ANC, 0, ige),
	    clip(besti + ANC, ninpnet - 1, ile),
	    clip(bestj + ANC, ninpnet - 1, ile),
	    inpunits, &worst, prop, nprop);

  /* calculate the actual activation values */
  for (i = 0; i < ninpnet; i++)
    for (j = 0; j < ninpnet; j++)
      /* only the propagation units should be active */
      if (propunit(i, j, prop))
	if (worst > best)
	  /* scale the values between max and min activation
	     in the entire neighborhood of radius ANC 
             (which may be larger than propagation neighborhood) */
	  inpunits[i][j].value = 1.0 - (inpunits[i][j].value - best)
	                         / (worst - best);
	else
	  inpunits[i][j].value = 1.0;
      else
	/* make everything else 0 */
	inpunits[i][j].value = 0.0;

  /* collect statistics about accuracy of the input map component,
     print log output */
  for (i = 0; i < noutrep[modi]; i++)
    outrep[modi][i] = inpunits[besti][bestj].comp[i];
  collect_stats (taski, modi);

  /* display map activation */
  if (displaying)
    {
      /* display map activity */
      display_lex (modi, inpunits, ninpnet);
      /* display the log line */
      display_lex_error (modi);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }
}


static void
associate (taski, modi, inpunits, prop, nprop, outunits, noutnet, assoc)
/* propagate through the neighborhood to the output map, find max output unit,
   collect stats, and display the map activation */
     int taski;				/* para or qa */
     int modi;				/* module number */
     FMUNIT inpunits[MAXLSNET][MAXLSNET]; /* input map */
     FMUNIT outunits[MAXLSNET][MAXLSNET]; /* output map */
     int noutnet;			/* output map size */
     double assoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET]; /* assoc weights */
     LEXPROPUNIT prop[];	        /* propagation through these units */
     int nprop;				/* number of propunits */
{
  int i, j;				/* output map indices */
  double abest,				/* best activation on output map */
    foo = (-1.0);			/* worst activation (not used) */
  int associ, assocj;			/* coordinates of associative image */

  /* propagate activation from the input map to the output map
     and find max and min responding units */
  abest = (-1.0);
  for (i = 0; i < noutnet; i++)
    for (j = 0; j < noutnet; j++)
      {
	/* propagate activation from input to output map */
	assocresponse (&outunits[i][j], i, j, inpunits, assoc, prop, nprop);
	/* find best and worst and best indices */
	updatebestworst (&abest, &foo, &associ, &assocj, &outunits[i][j],
			 i, j, fgreater, fsmaller);
      }
  
  /* scale the activation in the output map within 0 and 1 */
  for (i = 0; i < noutnet; i++)
    for (j = 0; j < noutnet; j++)
      {
	outunits[i][j].value = outunits[i][j].value /abest;
	/* avoid unnecessary display update */
	if (outunits[i][j].value < 0.1)
	  outunits[i][j].value = 0.0;
      }

  /* collect statistics about accuracy of the input map component,
     print log output */
  for (i = 0; i < noutrep[modi]; i++)
    outrep[modi][i] = outunits[associ][assocj].comp[i];
  collect_stats (taski, modi);

  /* allow mouse events again */
  nolexmouseevents = FALSE;

  /* display map activation */
  if (displaying)
    {
      /* display map activity */
      display_lex (modi, outunits, noutnet);
      /* display the log line */
      display_lex_error (modi);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }
}


static void
assocresponse (unit, i, j, inpunits, assoc, prop, nprop)
/* propagate activation from the input map to this output map unit */
     FMUNIT *unit;                          /* unit on the map */
     int i, j;
     FMUNIT inpunits[MAXLSNET][MAXLSNET];   /* input map */
     double assoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET];
                                            /* associative weights */
     LEXPROPUNIT prop[];	            /* propage through these units */
     int nprop;				    /* number of propunits */
{
  int p;

  /* save previous value to avoid redisplay if it does not change */
  unit->prevvalue = unit->value;

  /* propagate activity in the input map through the associative weights
     to the output unit */
  unit->value = 0.0;
  for (p = 0; p < nprop; p++)
    unit->value += inpunits[prop[p].i][prop[p].j].value
                   * assoc[prop[p].i][prop[p].j][i][j];
}


/****************** propagation neighborhood  *******************/

static void
findprop (lowi, lowj, highi, highj, inpunits, worst, prop, nprop)
/* find the PROPNC units in the neighborhood that should be included 
   in the propagation and place them in the global array prop; 
   as a side effect, determine the lowest activity in the neighborhood
   (used for scaling the activations for display) */
     int lowi, lowj, highi, highj;	/* boundaries of the neighborhood */
     FMUNIT inpunits[MAXLSNET][MAXLSNET]; /* the map */
     double *worst;			/* lowest activity in the neighborhd */
     LEXPROPUNIT prop[];	        /* propagation through these units */
     int *nprop;			/* number of propunits */
{
  int i, j,				/* map indices */
  p, k;					/* indices to activation list */

  prop[0].value = LARGEFLOAT;		/* initialize the list for sorting */
  *worst = (-1.0);			/* initialize the weakest value */

  for (i = lowi; i <= highi; i++)	/* look at all units in the  */
    for (j = lowj; j <= highj; j++)	/* neighborhood */
      {
	/* see if the activation of the current value is within
	   the PROPNC highest activations found so far
	   (note: low value means high activation) */
	for (p = 0; p < PROPNC; p++)
	  if (inpunits[i][j].value < prop[p].value)
	    {
	      /* found the right place; move the rest of the list down */
	      for (k = PROPNC - 1; k > p; k--)
		{
		  prop[k].value = prop[k - 1].value;
		  prop[k].i = prop[k - 1].i;
		  prop[k].j = prop[k - 1].j;
		}
	      /* insert the current value in the list */
	      prop[p].value = inpunits[i][j].value;
	      prop[p].i = i;
	      prop[p].j = j;
	      break;
	    }
	/* update the lowest activation (highest value) in the neighborhood */
	if (inpunits[i][j].value > *worst)
	  *worst = inpunits[i][j].value;
      }
  /* set up the number of units in the neighborhood for propagation */
  *nprop = (highi - lowi + 1) * (highj - lowj + 1);
  if (PROPNC < *nprop)
    *nprop = PROPNC;
}


static int
propunit (i, j, prop)
/* check if unit i, j is in the list of units included in the propagation */
     int  i, j;				/* unit indices */
     LEXPROPUNIT prop[];	        /* propagation through these units */
{
  int p;
  for (p = 0; p < PROPNC; p++)
    /* prop[p].value would be LARGEFLOAT if there are only p < PROPNC units
       in the neighborhood of radius ANC */
    if (prop[p].value < LARGEFLOAT)
      {
	if (prop[p].i == i && prop[p].j == j)
	  return(TRUE);
      }
    else break;
  return (FALSE);
}


/*********************  initializations ******************************/

void
lex_init ()
/* initialize the lexicon */
{
  FILE *fp;

  /* set up numbers of input/output words and reps for convenience */
  ninputs[LINPMOD] = noutputs[LINPMOD] = 1;
  ninputs[LOUTMOD] = noutputs[LOUTMOD] = 1;
  ninputs[SINPMOD] = noutputs[SINPMOD] = 1;
  ninputs[SOUTMOD] = noutputs[SOUTMOD] = 1;
  ninprep[LINPMOD] = noutrep[LINPMOD] = nlrep;
  ninprep[LOUTMOD] = noutrep[LOUTMOD] = nlrep;
  ninprep[SINPMOD] = noutrep[SINPMOD] = nsrep;
  ninprep[SOUTMOD] = noutrep[SOUTMOD] = nsrep;

  /* open the weight file */
  printf ("Reading lexicon from %s...", lexfile);
  open_file (lexfile, "r", &fp, REQUIRED);

  /* ignore everything until the lnet size */
  read_till_keyword (fp, LEXSIMU_NLNET, REQUIRED);
  fscanf (fp, "%d", &nlnet);
  if (nlnet > MAXLSNET || nlnet <= 0)
    {
      fprintf (stderr, "%s %s exceeds array size\n", lexfile, LEXSIMU_NLNET);
      exit (EXIT_SIZE_ERROR);
    }
  read_till_keyword (fp, LEXSIMU_NSNET, REQUIRED);
  fscanf (fp, "%d", &nsnet);
  if (nsnet > MAXLSNET || nsnet <= 0)
    {
      fprintf (stderr, "%s %s exceeds array size\n", lexfile, LEXSIMU_NSNET);
      exit (EXIT_SIZE_ERROR);
    }
  /* then ignore everything until the weights */
  read_till_keyword (fp, LEXSIMU_WEIGHTS, REQUIRED);
  /* read the weights */
  lex_iterate_weights (readfun, fp, NOT_USED, NOT_USED);
  fclose (fp);
  printf ("Done.\n");
}


static void
lex_iterate_weights (dofun, fp, par1, par2)
/* read the weights from the file */
     void (*dofun) __P((FILE *, double *, double, double)); /* read function */
     FILE *fp;
     double par1, par2;
{
  int i, j, k, ii, jj;

  /* lexical map input weights */
  for (i = 0; i < nlnet; i++)
    for (j = 0; j < nlnet; j++)
      for (k = 0; k < nlrep; k++)
	(*dofun) (fp, &lunits[i][j].comp[k], par1, par2);

  /* semantic map input weights */
  for (i = 0; i < nsnet; i++)
    for (j = 0; j < nsnet; j++)
      for (k = 0; k < nsrep; k++)
	(*dofun) (fp, &sunits[i][j].comp[k], par1, par2);

  /* associative connections */
  for (i = 0; i < nlnet; i++)
    for (j = 0; j < nlnet; j++)
      for (ii = 0; ii < nsnet; ii++)
	for (jj = 0; jj < nsnet; jj++)
	  {
	    (*dofun) (fp, &lsassoc[i][j][ii][jj], par1, par2);
	    (*dofun) (fp, &slassoc[ii][jj][i][j], par1, par2);
	  }
}


void
lex_clear_values (units, nnet)
/* clear map activations */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
     int nnet;				/* map size */
{
  int i, j;

  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      units[i][j].value = 0.0;
}


void
lex_clear_prevvalues (units, nnet)
/* reset the previous activations to NOPREV, so that the whole map
   will be displayed (because prevvalues are different to current values) */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
     int nnet;				/* map size */
{
  int i, j;

  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      units[i][j].prevvalue = NOPREV;
}

void
lex_clear_labels (units, nnet)
/* clear map activations */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
     int nnet;				/* map size */
{
  int i, j;
  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      units[i][j].labelcount = 0;
}
