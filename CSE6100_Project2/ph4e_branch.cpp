/* this program models a (M,N) Branch History Table with BTB_LEN entries
 * Chapter 2 concept example 2, Patterson/Hennessy 4e  January 2006  Bob Colwell
 *
 * Reads the file history.txt, format as follows:
 * 0x40074cdb	 0x40074cdf	1
 * 0x40074ce2	 0x40078d12	0
 * 0x4009a247	 0x4009a2bb	0
 * 0x4009a259	 0x4009a2c8	0
 * 0x4009a267	 0x4009a2ac	1
 * 0x4009a2b4	 0x4009a323	1
 * 0x4009955c	 0x40099676	0
 * 0x40099565	 0x400995d0	1
 *                              ^
 *                              +--   1: taken, 0: not-taken
 *                   ^
 *      ^            +-- branch target       
 *      |
 *      +--address of branch instruction
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using std::cin;

#define M 1
#define N 2
#define MAX_BTB_LEN 1024
#define MAXITERS 100000
#define TRUE 1
#define FALSE 0

struct { 
  int prediction;        /* taken (1) or not taken (0) */
  int addr_of_br;        /* addr of branch instruction itself */
  int br_target;         /* target of branch if taken */
  int hits;              /* number of times this BTB entry has hit */
  int misses;            /* number of times this BTB entry has missed */

} btb[MAX_BTB_LEN] ;

void dump_table();
void run_btb();
void update_table(int branch, int index, int addr, int target);
int check_prediction(int branch, int index);
void check_btb_consistency();
void calc_overall_hitrate(char *s);
void calc_mispred_rate(char *s);
void msbranch();
void capacity_misses();
int get_btb_index(int addr_br_instr);
void clear_btb();
void warm_start();
void clear_btb_stats();
void vary_btb_size();
float calc_rates();
void error(char *s);

int correct_predictions, mispredictions, tot_brs, btb_len, msbraddr, msbraddr_hits, msbraddr_misses;
FILE *f1, *f2, *fopen();

void main_old(unsigned long argc, char **argv)
{
    if ((f1 = fopen("history.txt", "r")) == NULL) {
	error("no history.txt file found.");
    }

    if (argc > 1)                       /* 1st arg is BTB length */
	 btb_len = atoi(argv[1]);
    else btb_len = MAX_BTB_LEN;

    correct_predictions = mispredictions = 0;
    msbraddr = msbraddr_hits = msbraddr_misses = 0;

    clear_btb();
    run_btb();

    calc_overall_hitrate("\nProblem 2.1.1");
    calc_mispred_rate   ("\nProblem 2.1.2");
    warm_start();       /* Problem 2.1.5 */
    msbranch();         /* Problem 2.1.3 */
    capacity_misses();  /* Problem 2.1.4 */
    vary_btb_size();    /* Problem 2.1.6 */
	int test;
	cin >> test;
}

void run_btb()
{
    int t_or_nt, addr_br_instr, br_target, btb_index;
	
    for (tot_brs=0; tot_brs<MAXITERS; tot_brs++) {
      if (fscanf(f1, "%x %x %d", &addr_br_instr, &br_target, &t_or_nt) != 3) break;

	btb_index = get_btb_index(addr_br_instr);

	if ((check_prediction(t_or_nt, btb_index) && (addr_br_instr == msbraddr))) {
	  msbraddr_hits++;
	}  else {
	  msbraddr_misses++;
	}

	update_table(t_or_nt, btb_index, addr_br_instr, br_target);
    }
}


int get_btb_index(int addr_br_instr)
{
    int index;

    index = addr_br_instr & (btb_len - 1);       /* zero out high bits to wrap around BTB */
    return(index);
}

void update_table(int branch, int index, int addr, int target)
{
	/* does the BTB entry at this index correspond to this particular branch? */

	if (btb[index].addr_of_br == addr) {
	    btb[index].hits++;
	} else {
	    btb[index].misses++;
	    btb[index].addr_of_br = addr;
	}
	
	switch (branch) {
	    case 1: 	btb[index].prediction++;    /* strengthen "taken" prediction */
		if (btb[index].prediction > 3)      /* max it out at 11 */
		    btb[index].prediction = 3;
			break;
	    case 0: 	btb[index].prediction--;    /* strengthen "not-taken" prediction */
		if (btb[index].prediction < 0)      /* don't let it go below 00 */
		    btb[index].prediction = 0;
			break;
	   default: error("branch value error in update_table");
	}
}

int check_prediction(int branch, int index)
{
    switch (branch) {
	case 1: 
	  if (btb[index].prediction >= 2) {
	    correct_predictions++; return(TRUE);
	  } else {
	    mispredictions++;      return(FALSE);
	  } break;
	case 0:
	  if (btb[index].prediction <= 1) {
	    correct_predictions++; return(TRUE);
	  } else {
	    mispredictions++;      return(FALSE);
	  } break;
	default: error("branch value error in check_prediction");
    }
}

void calc_overall_hitrate(char *s)
{
  int i, hits;

  hits = 0;
  for (i=0; i<btb_len; i++)
      hits += btb[i].hits;
  printf("%s", s);
  printf("\n\tNumber of hits in BTB: %d. Total brs: %d. Hit rate: %4.1f%%\n",
	   hits, tot_brs, 100.0*(float)hits/(float)tot_brs);

}

void calc_mispred_rate(char *s)
{
  printf("%s\n\tIncorrect predictions: %d of %d, or %4.1f%%\n", s,
	   mispredictions, tot_brs, 100.0*(float)mispredictions/(float)tot_brs);
}

void msbranch()
{
  int n, braddr, brtarg, tnt, braddr_hits, braddr_misses;
  braddr_hits = braddr_misses = 0;
    /* most common branch, percentage of overall correct predicts */

    printf("\nProblem 2.1.3\n\t$ sort -n history.txt | uniq -c -w 20 | sort -n | tail -1\n");
    printf("\tyields branch that occurs most often in this trace file, along with #occurrences.\n");

    msbraddr = msbraddr_hits = msbraddr_misses = 0;
    system( "./find_most_signif_branch" );   /* dumps result into msbranch.txt file */

    system( "sort -n history.txt | uniq -c -w 20 | sort -n | tail -1 > msbranch.txt" ); 

    if ((f2 = fopen("msbranch.txt", "r")) == NULL) {
	error("no msbranch.txt file found (with most significant branch inside); prob 1a.3.");
    }
    if (fscanf(f2, "%d %x %x %d", &n, &msbraddr, &brtarg, &tnt) != 4) 
      error("trouble reading msbranch.txt");

    printf("\tMost significant branch is seen %d times, out of %d total brs",
	   n, tot_brs);
    printf(" ; %4.1f%%\n", 100.0*(float)n/(float)tot_brs);

    /* Rewind the history.txt file, clear the BTB, rerun, but this time keep track of 
     * how many hits and misses are contributed by the most_significant_branch.
     */

    clear_btb();
    rewind(f1);    /* history.txt */
    correct_predictions = mispredictions = 0;
    run_btb();
    printf("\tMS branch=0x%x, correct predictions=%d (of %d total correct preds)",
	   msbraddr, msbraddr_hits, correct_predictions);
    printf("  or %4.1f%%\n", 100.0 * (float) msbraddr_hits / (float) correct_predictions);
}

void capacity_misses()
{
  int i, total_misses, total_compulsory;
  total_misses = 0;

  /* already kept track of #misses per BTB entry. Just add 'em all up, subtract out the
   * number of misses from each branch being seen for first time (get that from some cmd
   * line shell scripting)
   */
  system( "sort -n history.txt | uniq -w 10 | wc -l > num_first_time_misses.txt" );
  if ((f2 = fopen("num_first_time_misses.txt", "r")) == NULL) {
	error("no num_first_time_misses.txt file found (num unique branchs); prob 2.1.5.");
  }
  if (fscanf(f2, "%d", &total_compulsory) != 1) 
    error("trouble reading num_first_time_misses.txt");
  for (i=0; i<btb_len; i++)
    total_misses += btb[i].misses;

  printf("\nProblem 2.1.4\n\tTotal unique branches (1 miss per br compulsory): %d.\n",
	 total_compulsory);
  printf("\tTotal misses seen: %d.", total_misses);
  if (total_misses < total_compulsory) {
      dump_table();
      error("something screwy in capacity_misses, tot misses < compulsories");
  } else {
      printf(" So total capacity misses = total misses - compulsory misses = %d.\n",
	 total_misses - total_compulsory);
  }

}

void vary_btb_size()
{
  float mispredict_rate[10], hit_rates[10];
  int i, j, hits, misses;

  j=0;
  for (i=1; i<=64; i = i * 2) {
    correct_predictions = mispredictions = 0;
    clear_btb();
    rewind(f1);
    btb_len = i;
    run_btb();
    mispredict_rate[j] = calc_rates();
    j++;
  }

  printf("\nProblem 2.1.6\n\tBTB Length\tmispredict rate");
  j=0;
  for (i=1; i<=64; i = i * 2) {
    printf("\n\t%4d\t\t%4.2f%%", i, mispredict_rate[j]);
    j++;
  }
  printf("\n");
}

float calc_rates()
{
  int i, hits, misses;

  hits = misses = 0;
  for (i=0; i<btb_len; i++) {
      hits   += btb[i].hits;
      misses += btb[i].misses;
  }
  tot_brs = hits + misses;
  return (100.0*(float) misses / (float) tot_brs);
}

void warm_start()
{
    clear_btb();
    rewind(f1);
    correct_predictions = mispredictions = 0;
    run_btb();

    rewind(f1);
    correct_predictions = mispredictions = 0;
    clear_btb_stats();
    run_btb();
    calc_overall_hitrate("\nProblem 2.1.5");
    calc_mispred_rate("");
}

void clear_btb_stats()
{
  int i;
  for (i=0; i<btb_len; i++)
      btb[i].hits = btb[i].misses = 0;
}

void dump_table()
{
  int i, j, hits;

  for (i=0; i<btb_len; i++) {
	printf("\n%03d: ", i);
	switch (btb[i].prediction) {
	    case 0: printf(" N "); break;
	    case 1: printf(" n "); break;
	    case 2: printf(" t "); break;
	    case 3: printf(" T "); break;
	    default: error("btb prediction error in dump_table");
	}
	printf("   %8x   %8x", btb[i].addr_of_br, btb[i].br_target);
	printf("  %8dh    %8dm", btb[i].hits, btb[i].misses);
    }
    printf("\n");
}

void error(char *s)
{
    printf("%s \nExiting this sorry mess.\n\n", s);

    exit(1);
}

void clear_btb()
{
  int i;

    for (i=0; i<MAX_BTB_LEN; i++) {
      btb[i].prediction = 0;             /* init to strongly not-taken */
      btb[i].addr_of_br = 0;
      btb[i].br_target = 0;
      btb[i].hits = btb[i].misses = 0;
    }
}
