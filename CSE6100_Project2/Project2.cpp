#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>

using std::map;

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

int kBits = 5;
int totalBranches;

int GApEntries = 32;
int bitsFromAddress;
map<int, char*> GApHash;

map<int, int> PAgBHR;

map<int,int> AddrMap;
int uniqueAddr = 0;

int correct_predictions, mispredictions, tot_brs, btb_len, msbraddr, msbraddr_hits, msbraddr_misses;

struct { 
  unsigned int taken;        /* taken (1) or not taken (0) */
  int addr_of_br;        /* addr of branch instruction itself */
  int br_target;         /* target of branch if taken */
  int hits;              /* number of times this BTB entry has hit */
  int misses;            /* number of times this BTB entry has missed */

} branchRecord[MAX_BTB_LEN] ;

enum PREDICTOR { BR_GAg, BR_GAp, BR_PAg, BR_gshare};

#pragma region Inline Declarations
inline void error(char *s);
inline void GAg();
inline void GAp();
inline void PAg(int, int);
inline void gshare(int, int);
inline void SetupPHT(int, PREDICTOR);
inline int get_btb_index(int addr_br_instr);
inline int check_prediction(int branch, int index);
inline void update_table(int branch, int index, int addr, int target);
inline void run_btb();
inline void populate_branchRecord();
inline void clear_btb();
inline double log2(double n);
#pragma endregion

int main(int argc, char *argv[])
{
	if ((f1 = fopen("history.txt", "r")) == NULL) {
	//if ((f1 = fopen("TestBranches.txt", "r")) == NULL) {
		error("no history.txt file found.");
    }

    btb_len = MAX_BTB_LEN;
	PREDICTOR choice = BR_GAg;

	//Parse the command line flags
	for(int i = 1; i < argc; i++)
	{
		//get the predictor to use
		if(!strcmp(argv[i], "-t"))
		{
			if(!strcmp(argv[i + 1], "gag")) choice = BR_GAg;
			else if(!strcmp(argv[i + 1], "gap")) choice = BR_GAp;
			else if(!strcmp(argv[i + 1], "pag")) choice = BR_PAg;
			else if(!strcmp(argv[i + 1], "gshare")) choice = BR_gshare;
		}
		else if(!strcmp(argv[i], "-k")) kBits = atoi(argv[i + 1]);
		else if(!strcmp(argv[i], "-s")) PHTSize = atoi(argv[i + 1]);
	}

    correct_predictions = mispredictions = 0;
    msbraddr = msbraddr_hits = msbraddr_misses = 0;
	GApEntries = PHTSize;

	SetupPHT(kBits, choice);
    populate_branchRecord();
	fclose(f1);

	if(choice == BR_GAg)
		GAg();
	else if(choice == BR_GAp)
		GAp();
	else if(choice == BR_PAg)
		PAg(kBits, (int)log2(kBits));
	else if(choice == BR_gshare)
		gshare(kBits, kBits);
	
	//getchar();
}

void GAg()
{
	
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
			if(PHT[PHTidx] < 3) PHT[PHTidx]++;
		}
		else
		{
			if(PHT[PHTidx] == 2) PHT[PHTidx]--;//Move from weak taken to strong not taken
			if(PHT[PHTidx] > 0) PHT[PHTidx]--;
		}
#pragma endregion
	}

	printf("GAg\nCorrect: %d\nIncorrect:%d\n\n", correct_predictions, mispredictions);
}

void GAp()
{
	fseek(f1, 0, SEEK_SET);
	bitsFromAddress = (int)log2(GApEntries) - kBits;
	int addrMask = 0;
	for(int i = 0; i < bitsFromAddress; i++)//Creates a mask to get the lower bits of the address
		addrMask |= 1 << i;
	
    for (tot_brs=0; tot_brs < totalBranches; tot_brs++) {
		int address = branchRecord[tot_brs].addr_of_br;
		int PHTidx = ((BHR & BHRMask) << bitsFromAddress) | (address & addrMask);
		
		char* localPHT;

		//Find the address' PHT
		//Initialize a PHT if necessary
		map<int, char*>::iterator it = GApHash.find(address);
		if(it == GApHash.end())
		{
			localPHT = new char[GApEntries];
			memset(localPHT, 0, sizeof(char) * GApEntries);//Set all entries to 00. (SNP)
			GApHash[address] = localPHT;
		}
		else
			localPHT = it->second;

		int prediction = (localPHT[PHTidx] & 2) >> 1;//Checks to see if the record at the BHR index is 10 or 11. (Taken)
		if(prediction == branchRecord[tot_brs].taken)
			correct_predictions++;
		else
			mispredictions++;

		//Update PHT and BHR
#pragma region Updates
		BHR = (BHR << 1) + branchRecord[tot_brs].taken;
		if(branchRecord[tot_brs].taken)
		{
			if(localPHT[PHTidx] == 1) localPHT[PHTidx]++;//Move from weak not taken to strong taken
			if(localPHT[PHTidx] < 3) localPHT[PHTidx]++;
		}
		else
		{
			if(localPHT[PHTidx] == 2) localPHT[PHTidx]--;//Move from weak taken to strong not taken
			if(localPHT[PHTidx] > 0) localPHT[PHTidx]--;
		}
#pragma endregion
	}

	printf("GAp\nCorrect: %d\nIncorrect:%d\n\n", correct_predictions, mispredictions);
}

void PAg(int exp, int BHTbits)
{
	memset(PHT, 0, sizeof(char) * pow(2, 4));//Reset the PHT

	//Set the BHR for a 4 bit history
	BHRMask = 0;
	for(int i = 0; i < 4; i++)
		BHRMask |= 1 << i;

	//Set BHT mask
	int BHTMask = 0;
	for(int i = 0; i < BHTbits; i++)
		BHTMask |= 1 << i;

    for (tot_brs=0; tot_brs < totalBranches; tot_brs++) {

		int addr = (branchRecord[tot_brs].addr_of_br) & BHTMask;
		map<int, int>::iterator it = PAgBHR.find(addr);

		//load the BHR of this register
		int localBHR = 0;
		if(it != PAgBHR.end())
			localBHR = it->second;
	

		int PHTidx = localBHR & BHRMask;
		int prediction = (PHT[PHTidx] & 2) >> 1;//Checks to see if the record at the BHR index is 10 or 11. (Taken)
		if(prediction == branchRecord[tot_brs].taken)
			correct_predictions++;
		else
			mispredictions++;

		//Update PHT and BHR
#pragma region Updates
		localBHR = (localBHR << 1) + branchRecord[tot_brs].taken;
		if(branchRecord[tot_brs].taken)
		{
			if(PHT[PHTidx] == 1) PHT[PHTidx]++;//Move from weak not taken to strong taken
			if(PHT[PHTidx] < 3) PHT[PHTidx]++;
		}
		else
		{
			if(PHT[PHTidx] == 2) PHT[PHTidx]--;//Move from weak taken to strong not taken
			if(PHT[PHTidx] > 0) PHT[PHTidx]--;
		}

		//Store the new BHR
		PAgBHR[addr] = localBHR;
#pragma endregion
	}

	printf("PAg\nCorrect: %d\nIncorrect:%d\n\n", correct_predictions, mispredictions);
}

void gshare(int exp, int bits)
{
	//Reset the BHR
	BHR = 0;

	//Reset the PHT
	memset(PHT, 0, sizeof(char) * pow(2, exp));

	//Setup the mask for the BHR and address
	int MASK = 0;
	for(int i = 0; i < bits; i++)
		MASK |= 1 << i;
	
    for (tot_brs=0; tot_brs < totalBranches; tot_brs++) {
		
		int PHTidx = (BHR & MASK) ^ (branchRecord[tot_brs].addr_of_br & MASK);

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
			if(PHT[PHTidx] < 3) PHT[PHTidx]++;
		}
		else
		{
			if(PHT[PHTidx] == 2) PHT[PHTidx]--;//Move from weak taken to strong not taken
			if(PHT[PHTidx] > 0) PHT[PHTidx]--;
		}
#pragma endregion
	}

	printf("gshare\nCorrect: %d\nIncorrect:%d\n\n", correct_predictions, mispredictions);
}

void SetupPHT(int exp, PREDICTOR choice)
{
	if(choice == BR_GAg)
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

		map<int, int>::iterator it = AddrMap.find(addr_br_instr);
		int count = 0;
		if(it == AddrMap.end())
			uniqueAddr++;
		else
			count = it->second;
		AddrMap[addr_br_instr] = count + 1;
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

//Source: Lothar on http://stackoverflow.com/questions/758001/log2-not-found-in-my-math-h
// Calculates log2 of number.  
double log2( double n )  
{  
    // log(n)/log(2) is log2.  
    return log( n ) / log( 2 );  
}