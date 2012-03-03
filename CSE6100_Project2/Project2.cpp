#include <stdio.h>
#include <stdlib.h>
#include <iostream>


FILE *f1, *f2, *fopen();
unsigned int historyBits;
unsigned int PHTSize;
unsigned int PHTMask;

#pragma region Inline Declarations
inline void error(char *s);
inline void GAg();
inline void SetupPHT(int exp);
#pragma endregion

int main(int argc, char *argv[])
{
	if ((f1 = fopen("history.txt", "r")) == NULL) {
		error("no history.txt file found.");
    }

	SetupPHT(8);
}

void SetupPHT(int exp)
{
	PHTSize = pow(2, exp);

	//Only want the number of BHR bits to be enough to fully reference the PHT. Not any larger.
	// 2^N bit PHT
	// N bit BHR
	PHTMask = 0;
	for(int i = 0; i < exp; i++)
		PHTMask |= 1 << i;
}

void error(char *s)
{
    printf("%s \nExiting this sorry mess.\n\n", s);

    exit(1);
}