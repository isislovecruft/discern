/* File: proc.c
 *
 * Processing modules of DISCERN.
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: proc.c,v 1.38 1994/09/20 10:46:59 risto Exp $
 */

#include <stdio.h>
#include <math.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "defs.h"
#include "globals.c"


/************ processing module constants *************/

/* keywords expected in procsimu */
#define PROCSIMU_NHIDREP "network-nhidrep" /* hidden layer size in procsimu */
#define PROCSIMU_WEIGHTS "network-weights" /* procmodule weights in procsimu */


/******************** Function prototypes ************************/

/* global functions */
#include "prototypes.h"

/* functions local to this file */
static void parse_sentence __P((SENTSTRUCT sent, int taski));
static void gener_sentence __P((SENTSTRUCT sent, int taski));
static void forward_propagate __P((int modi));
static void proc_iterate_weights __P((void (*dofun) (FILE *, double *,
						     double, double),
				      FILE *fp, double par1, double par2));


/*********************  parsing routines ***************************/

void
parse_story ()
/* read a sequence of sentence case-role representations into a 
   slot-filler representation of the entire story. 
   Calls parse_sentence as a subroutine to get each case-role rep. */
{
  int i, j, k,
  senti,			/* sentence number in the input file */
  step = 0,			/* actual sentence number in the sequence */
  modi = STORYPARSMOD;		/* module number */

  printcomment ("\n", "parsing input story:", "\n");

  /* first clean up the network display */
  if (displaying)
    {
      proc_clear_network (modi);
      display_current_proc_net (modi);
    }
  
  /* get the target slot-filler indices */
  for (i = 0; i < noutputs[modi]; i++)
    targets[modi][i] = story.slots[i];
  /* form the target activation vector */
  for (i = 0; i < noutputs[modi]; i++)
    for (j = 0; j < nsrep; j++)
      tgtrep[modi][i * nsrep + j] = swords[targets[modi][i]].rep[j];
  /* display the target activation */
  if (displaying)
    {
      display_labeled_layer (modi, noutputs[modi], tgtrep[modi], targets[modi],
			     net[modi].tgtx, net[modi].tgty, BELOW);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

  /* previous hidden layer is blank in the beginning of the sequence */
  for (i = 0; i < nhidrep[modi]; i++)
    prevhidrep[modi][i] = 0.0;
  /* process each sentence included in the input story */
  for (senti = 0; senti < story.nsent; senti++)
    if (story.sents[senti].included)
      {
	/* first form the case-role representation for the sentence */
	parse_sentence (story.sents[senti], PARATASK);

	/* get the indices of the correct case-role representation */
	for (i = 0; i < ninputs[modi]; i++)
	  inputs[modi][i] = story.sents[senti].caseroles[i];
	/* if the modules are in a chain,
	   use sentence parser output as input */
	if (chain)
	  for (i = 0; i < ncaserep; i++)
	    inprep[modi][i] = caserep[i];
	else
	  /* use the correct case-role rep as input */
	  for (i = 0; i < ninputs[modi]; i++)
	    for (j = 0; j < nsrep; j++)
	      inprep[modi][i * nsrep + j] = swords[inputs[modi][i]].rep[j];

	/* display the input representation and the previous hidden layer */
	if (displaying)
	  {
	    display_labeled_layer (modi, ninputs[modi], inprep[modi],
				   inputs[modi],
				   net[modi].inpx, net[modi].inpy, ABOVE);
	    display_assembly (modi, net[modi].prevx, net[modi].prevy,
			      prevhidrep[modi], nhidrep[modi]);
	    wait_and_handle_events ();  /* stop if stepping, check events */
	  }

	/* propagate from input and prevhid layer to the output */
	forward_propagate (modi);
	/* display hidden layer, output, and the log line */
	if (displaying)
	  {
	    display_assembly (modi, net[modi].hidx, net[modi].hidy,
			      hidrep[modi], nhidrep[modi]);
	    display_labeled_layer (modi, noutputs[modi], outrep[modi],
				   targets[modi],
				   net[modi].outx, net[modi].outy, BELOW2);
	    display_error (modi, outrep[modi], noutputs[modi],
			   targets[modi], swords, nsrep, ++step);
	    wait_and_handle_events ();  /* stop if stepping, check events */
	  }
	/* update the previous hidden layer */
	for (k = 0; k < nhidrep[modi]; k++)
	  prevhidrep[modi][k] = hidrep[modi][k];
      }

  printcomment ("\n", "into internal rep:", "\n");
  /* collect statistics about the output accuracy of this module */
  collect_stats (PARATASK, modi);

  /* if the modules are in a chain, establish the result to be used
     for the episodic memory or storygen */
  if (chain)
    for (i = 0; i < nslotrep; i++)
      slotrep[i] = outrep[modi][i];
}


static void
parse_sentence (sent, taski)
/* read a sequence of input word representations into a case-role
   representation of the sentence. Calls the input_lexicon to transform
   lexical to the semantic representation. */
     SENTSTRUCT sent;		/* indices of the words in the sentence */
     int taski;			/* paraphrasing or question answering */
{
  int i, j, k,
  wordi,			/* word number in the input file */
  step = 0,			/* actual word number in the sequence */
  modi = SENTPARSMOD;		/* module number */

  /* first clean up the network display */
  if (displaying)
    {
      proc_clear_network (modi);
      display_current_proc_net (modi);
    }

  /* get the target caserole indices */
  for (i = 0; i < noutputs[modi]; i++)
    targets[modi][i] = sent.caseroles[i];
  /* form the target activation vector */
  for (i = 0; i < noutputs[modi]; i++)
    for (j = 0; j < nsrep; j++)
      tgtrep[modi][i * nsrep + j] = swords[targets[modi][i]].rep[j];

  /* display the target activation */
  if (displaying)
    {
      display_labeled_layer (modi, noutputs[modi], tgtrep[modi], targets[modi],
			     net[modi].tgtx, net[modi].tgty, BELOW);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

  /* previous hidden layer is blank in the beginning of the sequence */
  for (i = 0; i < nhidrep[modi]; i++)
    prevhidrep[modi][i] = 0.0;

  /* process each word */
  for (wordi = 0; wordi < sent.nseq; wordi++)
    {
      /* present the word to the lexicon to obtain
	 the semantic representation */
      if (withlex)
	input_lexicon (taski, sent.words[wordi]);
      else if (babbling)
	printf ("%s ", swords[sent.words[wordi]].chars);
      /* index of the current input word */
      inputs[modi][0] = sent.words[wordi];
      /* if the modules are in a chain and lexicon translation is modeled,
	 use the lexicon output as input */
      if (chain && withlex)
	for (i = 0; i < nsrep; i++)
	  inprep[modi][i] = swordrep[i];
      else
	/* use the correct semantic word representation */
	for (i = 0; i < nsrep; i++)
	  inprep[modi][i] = swords[inputs[modi][0]].rep[i];

      /* display the sentence read so far, the input representation, 
	 and the previous hidden layer activation */
      if (displaying)
	{
	  /* include the current word in the sentence read so far */
	  sprintf (net[modi].sequence, "%s %s",
		   net[modi].sequence, net[modi].newitem);
	  display_sequence (modi, net[modi].inpx, net[modi].inpy);
	  display_labeled_layer (modi, ninputs[modi], inprep[modi],
				 inputs[modi],
				 net[modi].inpx, net[modi].inpy, ABOVE);
	  display_assembly (modi, net[modi].prevx, net[modi].prevy,
			    prevhidrep[modi], nhidrep[modi]);
	  wait_and_handle_events ();  /* stop if stepping, check events */
	}

      /* propagate from input and prevhid layer to the output */
      forward_propagate (modi);
      /* display hidden layer, output, and the log line */
      if (displaying)
	{
	  display_assembly (modi, net[modi].hidx, net[modi].hidy,
			    hidrep[modi], nhidrep[modi]);
	  display_labeled_layer (modi, noutputs[modi], outrep[modi],
				 targets[modi],
				 net[modi].outx, net[modi].outy, BELOW2);
	  display_error (modi, outrep[modi], noutputs[modi], targets[modi],
			 swords, nsrep, ++step);
	  wait_and_handle_events ();  /* stop if stepping, check events */
	}

      /* update the previous hidden layer */
      for (k = 0; k < nhidrep[modi]; k++)
	prevhidrep[modi][k] = hidrep[modi][k];
    }
  if (babbling)
    putchar ('\n');
  /* collect statistics about the output accuracy of this module */
  collect_stats (taski, modi);

  /* if the modules are in a chain, establish the result to be used
     for the story parser or cue former */
  if (chain)
    for (i = 0; i < ncaserep; i++)
      caserep[i] = outrep[modi][i];
}


/*********************  generation routines ***************************/

void
gener_story ()
/* generate a paraphrase of the story from its slot-filler representation.
   Generates a sequence of sentence case-role representations and calls
   gener_sentence as a subroutine to output the words of each sentence. */
{
  int i, j, k,
  senti,			/* sentence number in the input file */
  step = 0,			/* actual sentence number in the sequence */
  modi = STORYGENMOD;		/* module number */

  printcomment ("\n", "generating paraphrase:", "\n");

  /* first clean up the network display */
  if (displaying)
    {
      proc_clear_network (modi);
      display_current_proc_net (modi);
    }
  
  /* get the input slot-filler indices */
  for (i = 0; i < ninputs[modi]; i++)
    inputs[modi][i] = story.slots[i];
  /* form the input activation vector */
  if (chain)
    /* if the modules are in a chain, use the current slotrep (either from
       the story parser or from the episodic memory) as input */
    for (i = 0; i < nslotrep; i++)
      inprep[modi][i] = slotrep[i];
  else
    /* otherwise use the correct representation */
    for (i = 0; i < ninputs[modi]; i++)
      for (j = 0; j < nsrep; j++)
	inprep[modi][i * nsrep + j] = swords[inputs[modi][i]].rep[j];

  /* display the input representation */
  if (displaying)
    {
      display_labeled_layer (modi, ninputs[modi], inprep[modi], inputs[modi],
			     net[modi].inpx, net[modi].inpy, BELOW);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

  /* previous hidden layer is blank in the beginning of the sequence */
  for (i = 0; i < nhidrep[modi]; i++)
    prevhidrep[modi][i] = 0.0;

  /* generate the output sentences */
  for (senti = 0; senti < story.nsent; senti++)
    {
      /* display the previous hidden layer */
      if (displaying && senti != 0)
	{
	  display_assembly (modi, net[modi].prevx, net[modi].prevy,
			    prevhidrep[modi], nhidrep[modi]);
	  wait_and_handle_events ();	/* stop if stepping, check events */
	}

      /* get the indices for the target case-role representation */
      for (i = 0; i < noutputs[modi]; i++)
	targets[modi][i] = story.sents[senti].caseroles[i];
      /* form the target case-role representation */
      for (i = 0; i < noutputs[modi]; i++)
	for (j = 0; j < nsrep; j++)
	  tgtrep[modi][i * nsrep + j] = swords[targets[modi][i]].rep[j];

      /* propagate from input to the output */
      forward_propagate (modi);
      /* display hidden layer, output, target, and the log line */
      if (displaying)
	{
	  display_assembly (modi, net[modi].hidx, net[modi].hidy,
			    hidrep[modi], nhidrep[modi]);
	  display_labeled_layer (modi, noutputs[modi], outrep[modi],
				 targets[modi],
				 net[modi].outx, net[modi].outy, ABOVE2);
	  display_layer (modi, noutputs[modi], tgtrep[modi],
			 net[modi].tgtx, net[modi].tgty, nsrep);
	  display_error (modi, outrep[modi], noutputs[modi],
			 targets[modi], swords, nsrep, ++step);
	  wait_and_handle_events ();	/* stop if stepping, check events */
	}

      /* update the previous hidden layer */
      for (k = 0; k < nhidrep[modi]; k++)
	prevhidrep[modi][k] = hidrep[modi][k];

      /* collect statistics about the output accuracy of this module */
      collect_stats (PARATASK, modi);

      /* if the modules are in a chain, establish the result to be used
	 by the sentence generator */
      if (chain)
	for (i = 0; i < ncaserep; i++)
	  caserep[i] = outrep[modi][i];

      /* generate the words for the sentence */
      gener_sentence (story.sents[senti], PARATASK);
    }
}


static void
gener_sentence (sent, taski)
/* generate the semantic words of the sentence one at a time from its internal
   case-role representation, call the lexicon to translate them into
   lexical words */
     SENTSTRUCT sent;		/* indices of the words in the sentence */
     int taski;			/* paraphrasing or question answering */
{
  int i, j, k,
  wordi,			/* word number in the input file */
  step = 0,			/* actual word number in the sequence */
  modi = SENTGENMOD;		/* module number */

  /* first clean up the network display */
  if (displaying)
    {
      proc_clear_network (modi);
      display_current_proc_net (modi);
    }

  /* get the input case-role indices */
  for (i = 0; i < ninputs[modi]; i++)
    inputs[modi][i] = sent.caseroles[i];
  /* form the input activation vector */
  for (i = 0; i < ninputs[modi]; i++)
    for (j = 0; j < nsrep; j++)
      /* if the modules are in a chain, use the current caserep (either from
	 the story generator or from the answer produces) as input */
      if (chain)
	inprep[modi][i * nsrep + j] = caserep[i * nsrep + j];
      else
	/* otherwise use the correct representation */
	inprep[modi][i * nsrep + j] = swords[inputs[modi][i]].rep[j];

  /* display the input representation */
  if (displaying)
    {
      display_labeled_layer (modi, ninputs[modi], inprep[modi], inputs[modi],
			     net[modi].inpx, net[modi].inpy, BELOW);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

  /* previous hidden layer is blank in the beginning of the sequence */
  for (i = 0; i < nhidrep[modi]; i++)
    prevhidrep[modi][i] = 0.0;

  /* generate the output words */
  for (wordi = 0; wordi < sent.nseq; wordi++)
    {
      /* display the previous hidden layer */
      if (displaying && wordi != 0)
	{
	  display_assembly (modi, net[modi].prevx, net[modi].prevy,
			    prevhidrep[modi], nhidrep[modi]);
	  wait_and_handle_events ();	/* stop if stepping, check events */
	}

      /* get the index of the target word */
      targets[modi][0] = sent.words[wordi];
      /* form the target semantic word representation */
      for (j = 0; j < nsrep; j++)
	tgtrep[modi][j] = swords[targets[modi][0]].rep[j];

      /* propagate from input to the output */
      forward_propagate (modi);
      /* display hidden layer, sentence so far, output, target,
	 and the log line */
      if (displaying)
	{
	  display_assembly (modi, net[modi].hidx, net[modi].hidy,
			    hidrep[modi], nhidrep[modi]);
	  /* include the current word in the sentence produced so far */
	  sprintf (net[modi].sequence, "%s %s",
		   net[modi].sequence, net[modi].newitem);
	  display_sequence (modi, net[modi].tgtx, net[modi].tgty);
	  display_labeled_layer (modi, noutputs[modi], outrep[modi],
				 targets[modi],
				 net[modi].outx, net[modi].outy, ABOVE2);
	  display_layer (modi, noutputs[modi], tgtrep[modi],
			 net[modi].tgtx, net[modi].tgty, nsrep);
	  display_error (modi, outrep[modi], noutputs[modi], targets[modi],
			 swords, nsrep, ++step);
	  wait_and_handle_events ();	/* stop if stepping, check events */
	}

      /* update the previous hidden layer */
      for (k = 0; k < nhidrep[modi]; k++)
	prevhidrep[modi][k] = hidrep[modi][k];

      /* collect statistics about the output accuracy of this module */
      collect_stats (taski, modi);
      /* if the modules are in a chain and lexicon is included in the
	 simulation, establish the result to be used as input to the
	 lexicon */
      if (chain && withlex)
	for (j = 0; j < nsrep; j++)
	  swordrep[j] = outrep[modi][j];
      /* if lexicon is included, translate the semantic word to its
	 lexical counterpart */
      if (withlex)
	output_lexicon (taski, sent.words[wordi]);
      /* if generating an answer to a text_question and the current word
	 is the period, generate no more words */
      if (text_question &&
	  find_nearest (outrep[modi], swords, nsrep, nswords)
	  == sentdelimindex)
	break;
    }
  if (babbling)
    putchar ('\n');
}


/*********************  question answering  ***************************/

void
formcue ()
/* given a question case-role representation as input, form a cue
   slot-filler representation for the episodic memory. Calls parse_sentence
   as a subroutine to obtain the case-role representation. */
{
  int i, j,
  step = 0,			/* there is no sequence, so this is always 0 */
  modi = CUEFORMMOD;		/* module number */

  printcomment ("\n", "parsing question:", "\n");

  /* first clean up the network display */
  if (displaying)
    {
      proc_clear_network (modi);
      display_current_proc_net (modi);
    }
  
  /* form the case-role representation for the question */
  parse_sentence (qa.question, QATASK);
  /* get the input case-role indices */
  for (i = 0; i < ninputs[modi]; i++)
    inputs[modi][i] = qa.question.caseroles[i];
  /* if the modules are in a chain, use the current caserep (from
     the sentence parser) as input */
  if (chain)
    for (i = 0; i < ncaserep; i++)
      inprep[modi][i] = caserep[i];
  else
    /* otherwise use the correct representation */
    for (i = 0; i < ninputs[modi]; i++)
      for (j = 0; j < nsrep; j++)
	inprep[modi][i * nsrep + j] = swords[inputs[modi][i]].rep[j];

  /* display the input representation */
  if (displaying)
    {
      display_labeled_layer (modi, ninputs[modi], inprep[modi], inputs[modi],
			     net[modi].inpx, net[modi].inpy, ABOVE);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

  /* get the target story slot-filler indices */
  for (i = 0; i < noutputs[modi]; i++)
    targets[modi][i] = qa.slots[i];
  /* form the target activation vector */
  for (i = 0; i < noutputs[modi]; i++)
    for (j = 0; j < nsrep; j++)
      tgtrep[modi][i * nsrep + j] = swords[targets[modi][i]].rep[j];

  /* propagate from input to the output */
  forward_propagate (modi);
  /* display hidden layer, output, target, and the log line */
  if (displaying)
    {
      display_assembly (modi, net[modi].hidx, net[modi].hidy,
			hidrep[modi], nhidrep[modi]);
      display_labeled_layer (modi, noutputs[modi], outrep[modi], targets[modi],
			     net[modi].outx, net[modi].outy, BELOW2);
      display_layer (modi, noutputs[modi], tgtrep[modi],
		     net[modi].tgtx, net[modi].tgty, nsrep);
      display_error (modi, outrep[modi], noutputs[modi], targets[modi],
		     swords, nsrep, ++step);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

  printcomment ("\n", "into cue:", "\n");
  /* collect statistics about the output accuracy of this module */
  collect_stats (QATASK, modi);

  /* if the modules are in a chain and the episodic memory is included in the
     simulation, establish the result to be used as input to the memory */
  if (chain && withhfm)
    for (i = 0; i < nslotrep; i++)
      slotrep[i] = outrep[modi][i];
}


void
produceanswer ()
/* given the question case-role representation and the story slot-filler
   representation as input, generates the case-role representation of
   the answer sentence, and calls sentence generator to output it word
   by word. */
{
  int i, j,
  step = 0,			/* there is no sequence, so this is always 0 */
  modi = ANSWERPRODMOD;		/* module number */

  /* first clean up the network display */
  if (displaying)
    {
      proc_clear_network (modi);
      display_current_proc_net (modi);
    }
  
  /* get the input case-role indices and the input slot-filler indices */
  for (i = 0; i < ncase; i++)
    inputs[modi][i] = qa.question.caseroles[i];
  for (i = 0; i < nslot; i++)
    inputs[modi][ncase + i] = qa.slots[i];
  /* if the modules are in a chain, use the current caserep (from
     the sentence parser) and the current slotrep (from the episodic
     memory) as input */
  if (chain)
    {
      for (i = 0; i < ncaserep; i++)
	inprep[modi][i] = caserep[i];
      for (i = 0; i < nslotrep; i++)
	inprep[modi][i + noutrep[0]] = slotrep[i];
    }
  else
    /* otherwise use the correct representations */
    for (i = 0; i < ninputs[modi]; i++)
      for (j = 0; j < nsrep; j++)
	inprep[modi][i * nsrep + j] = swords[inputs[modi][i]].rep[j];

  /* display the input representation */
  if (displaying)
    {
      /* question part of the input layer */
      display_labeled_layer (modi, ncase, inprep[modi], inputs[modi],
			     net[modi].inpx, net[modi].inpy, BELOW);
      /* story part */
      display_labeled_layer (modi, nslot, &inprep[modi][ncaserep],
			     &inputs[modi][ncase],
			     net[modi].inp1x, net[modi].inp1y, BELOW);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

  /* get the target case-role indices */
  for (i = 0; i < noutputs[modi]; i++)
    targets[modi][i] = qa.answer.caseroles[i];
  /* form the target activation vector */
  for (i = 0; i < noutputs[modi]; i++)
    for (j = 0; j < nsrep; j++)
      tgtrep[modi][i * nsrep + j] = swords[targets[modi][i]].rep[j];

  /* propagate from input to the output */
  forward_propagate (modi);
  /* display hidden layer, output, target, and the log line */
  if (displaying)
    {
      display_assembly (modi, net[modi].hidx, net[modi].hidy,
			hidrep[modi], nhidrep[modi]);
      display_labeled_layer (modi, noutputs[modi], outrep[modi], targets[modi],
			     net[modi].outx, net[modi].outy, ABOVE2);
      display_layer (modi, noutputs[modi], tgtrep[modi],
		     net[modi].tgtx, net[modi].tgty, nsrep);
      display_error (modi, outrep[modi], noutputs[modi], targets[modi],
		     swords, nsrep, ++step);
      wait_and_handle_events ();  /* stop if stepping, check for events */
    }

  printcomment ("\n", "generating answer:", "\n");
  /* collect statistics about the output accuracy of this module */
  collect_stats (QATASK, modi);
  /* if the modules are in a chain, establish the result to be used as 
     input to the sentence generator */
  if (chain)
    for (i = 0; i < ncaserep; i++)
      caserep[i] = outrep[modi][i];	/* activate result */
  /* generate the word of the answer sentence */
  gener_sentence (qa.answer, QATASK);
}


/*********************  propagation ******************************/

static void
forward_propagate (modi)
/* propagate activation from input and (possibly) previous hidden layer
   to the output layer */
     int modi;				/* module number */
{
  int i, k, p;

  /* hidden layer biases */
  for (k = 0; k < nhidrep[modi]; k++)
    hidrep[modi][k] = hidbias[modi][k];
  /* propagate from the previous hidden layer (if the network has one)
     to the hidden layer */
  if (modi != CUEFORMMOD && modi != ANSWERPRODMOD)
    for (p = 0; p < nhidrep[modi]; p++)
      for (k = 0; k < nhidrep[modi]; k++)
	hidrep[modi][k] += prevhidrep[modi][p] * wph[modi][p][k];
  /* propagate from the input layer to the hidden layer */
  for (i = 0; i < ninprep[modi]; i++)
    for (k = 0; k < nhidrep[modi]; k++)
      hidrep[modi][k] += inprep[modi][i] * wih[modi][i][k];
  /* pass the weighted sums through the sigmoid to get activations
     of the hidden layer units */
  for (k = 0; k < nhidrep[modi]; k++)
    hidrep[modi][k] = 1.0 / (1.0 + exp (-hidrep[modi][k]));

  /* output layer biases */
  for (i = 0; i < noutrep[modi]; i++)
    outrep[modi][i] = outbias[modi][i];
  /* propagate from the hidden layer to the output layer */
  for (k = 0; k < nhidrep[modi]; k++)
    for (i = 0; i < noutrep[modi]; i++)
      outrep[modi][i] += hidrep[modi][k] * who[modi][k][i];
  /* pass the weighted sums through the sigmoid to get activations
     of the output layer units */
  for (i = 0; i < noutrep[modi]; i++)
    outrep[modi][i] = 1.0 / (1.0 + exp (-outrep[modi][i]));
}


/*********************  initializations ******************************/

void
proc_init ()
/* initialize the processing modules */
{
  int modi;				/* module number */
  FILE *fp;				/* proc module definition file */
  char rest[MAXSTRL + 1];		/* temporarily holds the nhidreps */

  /* set up numbers of input/output words for convenience */
  /* one word representation wide */
  ninputs[SENTPARSMOD] = noutputs[SENTGENMOD] = 1;
  /* case-role representations */
  noutputs[SENTPARSMOD] = ninputs[STORYPARSMOD] = ninputs[CUEFORMMOD]
    = noutputs[ANSWERPRODMOD] = noutputs[STORYGENMOD] = ninputs[SENTGENMOD]
    = ncase;
  /* slot-filler representations */
  noutputs[STORYPARSMOD] = noutputs[CUEFORMMOD] = ninputs[STORYGENMOD] = nslot;
  /* answerprod has both case-role and slot-filler in its input */
  ninputs[ANSWERPRODMOD] = ncase + nslot;

  /* set up input/output dimensions for convenience */
  for (modi = 0; modi < NPROCMODULES; modi++)
    {
      ninprep[modi] = ninputs[modi] * nsrep;
      noutrep[modi] = noutputs[modi] * nsrep;
    }

  /* open the weight file */
  printf ("Reading processing modules from %s...", procfile);
  open_file (procfile, "r", &fp, REQUIRED);

  /* ignore everything until the hidden layer sizes */
  read_till_keyword (fp, PROCSIMU_NHIDREP, REQUIRED);

  /* read the size string */
  fgetline (fp, rest, MAXSTRL);
  /* convert the string to numbers and load */
  if (text2ints (nhidrep, NPROCMODULES, rest) != NPROCMODULES)
    {
      fprintf (stderr, "Wrong number of %s %s specifications\n",
	       procfile, PROCSIMU_NHIDREP);
      exit (EXIT_DATA_ERROR);
    }
  for (modi = 0; modi < NPROCMODULES; modi++)
    if (nhidrep[modi] > MAXHIDREP || nhidrep[modi] <= 0)
      {
	fprintf (stderr, "%s %s[%d] exceeds array size\n",
		 procfile, PROCSIMU_NHIDREP, modi);
	exit (EXIT_SIZE_ERROR);
      }
  /* then ignore everything until the weights */
  read_till_keyword (fp, PROCSIMU_WEIGHTS, REQUIRED);

  /* read the weights */
  proc_iterate_weights (readfun, fp, NOT_USED, NOT_USED);
  fclose (fp);
  printf ("Done.\n");
}


static void
proc_iterate_weights (dofun, fp, par1, par2)
/* read the weights from the file */
     /* reading function (as parameter so that randomization or writing
     could be done with proc_iterate_weights as well) */
     void (*dofun) __P((FILE *, double *, double, double));
     FILE *fp;				/* weight file */
     double par1, par2;			/* not used (in reading weights) */
{
  int modi, i, j, k;

  /* read weights for each of the processing modules */
  for (modi = 0; modi < NPROCMODULES; modi++)
    {
      /* input-to-hidden weights */
      for (i = 0; i < ninprep[modi]; i++)
	for (k = 0; k < nhidrep[modi]; k++)
	  (*dofun) (fp, &wih[modi][i][k], par1, par2);

      /* hidden layer biases */
      for (k = 0; k < nhidrep[modi]; k++)
	(*dofun) (fp, &hidbias[modi][k], par1, par2);

      /* previous hidden layer to hidden layer weights
         (for parsers and generators) */
      if (modi != CUEFORMMOD && modi != ANSWERPRODMOD)
	for (j = 0; j < nhidrep[modi]; j++)
	  for (k = 0; k < nhidrep[modi]; k++)
	    (*dofun) (fp, &wph[modi][j][k], par1, par2);

      /* hidden-to-output weights */
      for (k = 0; k < nhidrep[modi]; k++)
	for (i = 0; i < noutrep[modi]; i++)
	  (*dofun) (fp, &who[modi][k][i], par1, par2);

      /* output layer biases */
      for (i = 0; i < noutrep[modi]; i++)
	(*dofun) (fp, &outbias[modi][i], par1, par2);
    }
}


void
proc_clear_network (modi)
/* clear the activations and input/target indices and display strings */
     int modi;				/* module number */
{
  int i;

  for (i = 0; i < ninputs[modi]; i++)
    inputs[modi][i] = BLANKINDEX;
  for (i = 0; i < noutputs[modi]; i++)
    targets[modi][i] = BLANKINDEX;
  for (i = 0; i < ninprep[modi]; i++)
    inprep[modi][i] = 0.0;
  for (i = 0; i < noutrep[modi]; i++)
    outrep[modi][i] = 0.0;
  for (i = 0; i < noutrep[modi]; i++)
    tgtrep[modi][i] = 0.0;
  for (i = 0; i < nhidrep[modi]; i++)
    hidrep[modi][i] = 0.0;
  for (i = 0; i < nhidrep[modi]; i++)
    prevhidrep[modi][i] = 0.0;
  sprintf (net[modi].sequence, "%s", "");	/* clear word sequence */
  sprintf (net[modi].newitem, "%s", "");	/* clear new item in seq */
  sprintf (net[modi].log, "%s", "");		/* clear log line */
}
