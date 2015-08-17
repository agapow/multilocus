/**************************************************************************
SblNumerics - utility math functions

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.agapow.net>

About:
- A group of math functions, largely generic and using valarrays

Changes:
- 99.8.7: Created.

To Do:
- other sort functions?
- can heapsort be set to work in descending order?

**************************************************************************/


// *** INCLUDES

#include "SblNumerics.h"


// *** INTERFACE *********************************************************/

// COUNT SAMPLES FROM POPULATION
// How many different samples or combinations of (iSampleSize) can you make
// from a population of (iPopulationSize)? 
UInt CountSamplesFromPopulation (UInt iPopulationSize, UInt iSampleSize)
{
	return (Factorial(iPopulationSize) /
		(Factorial(iSampleSize) * Factorial(iPopulationSize - iSampleSize)));
}

UInt CountCombinations (UInt iPopulationSize, UInt iSampleSize)
{
	return CountSamplesFromPopulation (iPopulationSize, iSampleSize);
}


// FACTORIAL
// Calculates the factorial of the operand by recursion. Note it only
// accepts legitimate (positive) arguments.
UInt Factorial (UInt iOperand)
{
	if (iOperand <= 1)
		return 1;
	else
		return (iOperand * Factorial(iOperand - 1));
}


// *** END