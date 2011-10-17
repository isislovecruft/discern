/* File: prototypes.h
 *
 * Prototypes for global functions.
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: prototypes.h,v 1.19 1994/09/20 10:46:59 risto Exp $
 */

/********* defined in main.c *************/
void process_command __P ((FILE *fp, char *commandstr, char *rest));
void readfun __P ((FILE *fp, double *place, double par1, double par2));
void printcomment __P ((char *beginning, char *s, char *ending));
void fgetline __P ((FILE *fp, char *s, int lim));
void fgl __P ((FILE *fp));
int find_nearest __P ((double rep[], WORDSTRUCT words[],
		       int nrep, int nwords));
int list2indices __P ((int itemarray[], char rest[],
		       int maxitems, char listname[]));
int text2ints __P ((int itemarray[], int nitems, char nums[]));
int read_till_keyword __P ((FILE * fp, char keyword[], int required));
int select_lexicon __P ((int modi, WORDSTRUCT **words,
			 int *nrep, int *nwords));
int open_file __P ((char *filename, char *mode, FILE **fp, int required));
double distance __P ((int *foo, double v1[], double v2[], int nrep));
double seldistance __P ((int indices[], double v1[], double v2[], int ncomp));
int clip __P ((int index, int limit, int (*comparison) (int, int)));
int ile __P ((int i, int j));
int ige __P ((int i, int j));
int fsmaller __P ((double x, double y));
int fgreater __P ((double x, double y));
void distanceresponse __P ((FMUNIT *unit, double inpvector[], int ninpvector,
			    double (*responsefun)
			    (int *, double *, double *, int), int indices[]));
void updatebestworst __P ((double *best, double *worst, int *besti, int *bestj,
			   FMUNIT *unit, int i, int j,
			   int (*better) (double, double),
			   int (*worse) (double, double)));


/********* defined in proc.c *************/
void parse_story __P ((void));
void gener_story __P ((void));
void formcue __P ((void));
void produceanswer __P ((void));
void proc_init __P ((void));
void proc_clear_network __P ((int modi));


/********* defined in hfm.c ************/
void presentmem __P ((int taski, int mod, int slots[], double *bestvalue));
void classify __P ((int modi, IMAGEDATA *image));
void hfm_init __P ((void));
void hfm_clear_values __P ((void));
void hfm_clear_prevvalues __P ((void));
void hfm_clear_labels __P ((void));


/********* defined in trace.c *********/
void store_trace __P ((IMAGEDATA image, FMUNIT table[MAXBOTNET][MAXBOTNET],
		       double lweights[MAXBOTNET][MAXBOTNET]
		                      [MAXBOTNET][MAXBOTNET],
		       int dim, int indices[]));
void retrieve_trace __P ((int topi, int topj, int midi, int midj,
			  int *boti, int *botj, double *value,
			  FMUNIT table[MAXBOTNET][MAXBOTNET],
			  double lweights[MAXBOTNET][MAXBOTNET]
			                 [MAXBOTNET][MAXBOTNET], 
			  int dim, int indices[], double inpvector[]));
void clear_traces __P ((void));


/********** defined in lex.c *********/
void input_lexicon __P ((int taski, int index));
void output_lexicon __P ((int taski, int index));
void lex_init __P ((void));
void lex_clear_values __P ((FMUNIT units[MAXLSNET][MAXLSNET], int nnet));
void lex_clear_prevvalues __P ((FMUNIT units[MAXLSNET][MAXLSNET], int nnet));
void lex_clear_labels __P ((FMUNIT units[MAXLSNET][MAXLSNET], int nnet));


/********** defined in stats.c *********/
void init_stats_blanks __P ((void));
void init_stats __P ((void));
void collect_stats __P ((int taski, int modi));
void print_stats __P ((void));


/********* defined in graph.c ***********/
void display_init __P ((void));
void wait_and_handle_events __P ((void));
void close_display __P ((void));
void clear_networks_display __P ((void));
void wait_for_run __P ((void));
void start_running __P ((void));
void display_error __P ((int modi, double outrep[], int nas, int target[],
			 WORDSTRUCT words[], int nrep, int step));
void display_current_proc_net __P ((int modi));
void display_sequence __P ((int modi, int x, int y));
void display_labeled_layer __P ((int modi, int nas, double rep[],
				 int nums[], int x, int y, int labeloffset));
void display_layer __P ((int modi, int nas, double layer[], int x,
			 int y, int nrep));
void display_assembly __P ((int modi, int x, int y, double assembly[],
			    int nrep));
void display_new_hfm_labels __P ((void));
void display_hfm_values __P ((void));
void display_hfm_error __P ((int modi, IMAGEDATA image));
void display_hfm_top __P ((void));
void display_hfm_mid __P ((int topi, int topj));
void display_hfm_bot __P ((int topi, int topj, int midi, int midj));
void display_new_lex_labels __P ((int modi, int nnet, int nwords,
				  WORDSTRUCT words[], int nrep,
				  FMUNIT units[MAXLSNET][MAXLSNET]));
void display_lex __P ((int modi, FMUNIT units[MAXLSNET][MAXLSNET], int nnet));
void display_lex_error __P ((int modi));

