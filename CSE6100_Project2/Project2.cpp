#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#define MAX_BTB_LEN 100000
#define MAXITERS 100000
#define TRUE 1
#define FALSE 0

FILE *f1, *f2, *fopen();
unsigned int historyBits;
unsigned int PHTSize;
unsigned int BHRMask;
unsigned int BHR;
unsigned char *PHT;

int totalBranches;

int correct_predictions, mispredictions, tot_brs, btb_len, msbraddr, msbraddr_hits, msbraddr_misses;

struct { 
  unsigned int taken;        /* taken (1) or not taken (0) */
  int addr_of_br;        /* addr of branch instruction itself */
  int br_target;         /* target of branch if taken */
  int hits;              /* number of times this BTB entry has hit */
  int misses;            /* number of times this BTB entry has missed */

} branchRecord[MAX_BTB_LEN] ;

#pragma region Inline Declarations
inline void error(char *s);
inline void GAg();
inline void SetupPHT(int exp);
inline int get_btb_index(int addr_br_instr);
inline int check_prediction(int branch, int index);
inline void update_table(int branch, int index, int addr, int target);
inline void run_btb();
inline void populate_branchRecord();
inline void clear_btb();
#pragma endregion

int main(int argc, char *argv[])
{
	if ((f1 = fopen("history.txt", "r")) == NULL) {
		error("no history.txt file found.");
    }

	if (argc > 1)                       /* 1st arg is BTB length */
	 btb_len = atoi(argv[1]);
    else btb_len = MAX_BTB_LEN;

    correct_predictions = mispredictions = 0;
    msbraddr = msbraddr_hits = msbraddr_misses = 0;

	SetupPHT(8);
    populate_branchRecord();
	GAg();
}

void GAg()
{
	fseek(f1, 0, SEEK_SET);
	
    for (tot_brs=0; tot_brs < totalBranches; tot_brs++) {
		int PHTidx = BHR & BHRMask;
		int prediction = (PHT[PHTidx] & 2) >> 1;//Checks to see if the record at the BHR index is 10 or 11. (Taken)
		if(prediction == branchRecord[tot_brs].taken)
			correct_predictions++;
		else
			mispredictions++;

		//Update PHT and BHR
#pragma region Updates
		BHR = (BHR << 1) + branchRecord[tot_brs].taken;
		if(branchRecord[tot_brs].taken)
		{
			if(PHT[PHTidx] == 1) PHT[PHTidx]++;//Move from weak not taken to strong taken
			PHT[PHTidx]++;
		}
		else
		{
			if(PHT[PHTidx] == 2) PHT[PHTidx]--;//Move from weak taken to strong not taken
			PHT[PHTidx]--;
		}
#pragma endregion
	}
}

void SetupPHT(int exp)
{
	PHTSize = pow(2, exp);

	//Only want the number of BHR bits to be enough to fully reference the PHT. Not any larger.
	// 2^N bit PHT
	// N bit BHR
	BHRMask = 0;
	for(int i = 0; i < exp; i++)
		BHRMask |= 1 << i;

	PHT = new unsigned char[PHTSize];
	for(int i = 0; i < PHTSize; i++)
		PHT[i] = 0;
}

/*void clear_btb()
{
  int i;

    for (i=0; i<MAX_BTB_LEN; i++) {
      btb[i].prediction = 0;             // init to strongly not-taken
      btb[i].addr_of_br = 0;
      btb[i].br_target = 0;
      btb[i].hits = btb[i].misses = 0;
    }
}*/

void populate_branchRecord()
{
	int t_or_nt, addr_br_instr, br_target, btb_index;
	
    for (tot_brs=0; tot_brs<MAXITERS; tot_brs++) {
		if (fscanf(f1, "%x %x %d", &addr_br_instr, &br_target, &t_or_nt) != 3) 
		{
			totalBranches = tot_brs;
			break;
		}
		//int idx = get_btb_index(addr_br_instr);
		branchRecord[tot_brs].taken = t_or_nt;
		branchRecord[tot_brs].addr_of_br = addr_br_instr;
		branchRecord[tot_brs].br_target = br_target;
	}

}

/*
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

void update_table(int branch, int index, int addr, int target)
{
	// does the BTB entry at this index correspond to this particular branch?

	if (btb[index].addr_of_br == addr) {
	    btb[index].hits++;
	} else {
	    btb[index].misses++;
	    btb[index].addr_of_br = addr;
	}
	
	switch (branch) {
	    case 1: 	btb[index].prediction++;    // strengthen "taken" prediction
		if (btb[index].prediction > 3)      // max it out at 11
		    btb[index].prediction = 3;
			break;
	    case 0: 	btb[index].prediction--;    // strengthen "not-taken" prediction
		if (btb[index].prediction < 0)      // don't let it go below 00
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
}*/

int get_btb_index(int addr_br_instr)
{
    int index;

    index = addr_br_instr & (btb_len - 1);       //zero out high bits to wrap around BTB
    return(index);
}

void error(char *s)
{
    printf("%s \nExiting this sorry mess.\n\n", s);

    exit(1);
}