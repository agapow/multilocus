/**************************************************************************
RandomService.cpp - encapsulated pseudo-random number generator

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://mesa@agapow.net> <http://www.agapow.net/software/mesa/>
- From public domain code by Ray Gardner, based on "RandomService Number
  Generators: Good Ones Are Hard to Find", Park & Miller (1988) CACM 31:10
  and "Two Fast Implementations of the 'Minimal Standard' RandomService
  Number Generator", Carta (1990) CACM 33:1. Chi squared test from
  Sedgewick (1990) "Algorithms in C" p517.

About:
- A psuedo-random number generator intended to provide various random
  types, in various distributions over various ranges. As a service, it
  provides a fat interface with many functions. Where a generator function
  (STL-wise) is needed, i.e. a class that makes only randoms of a given
  distribution, instead use RandomGenerator and all its progeny.
- This can serve as the base class for a family of RNGs, with the others
  needing only to define their basic "randomness" generator, Generator()
  and perhaps InitSeed() and SetSeed().
- Implemented as a linear congruential generator, where f(z) = 16807 z mod
  (2 ** 31 - 1). Uses L. Schrage's method to avoid overflow problems.
- The names "Float" and "Whole"  are used to avoid implying incorrect
  limitations to the random functions, and also to divide them into two
  broad classes: those that take and return decimal/real numbers and those
  that use whole/integer numbers. 

Changes:
- 99.8.1: Modified from previous Random class. Almost immediately ran into
  this problem - a call like Uniform (5,10) has to be typecast as long to
  allow the compiler to resolve the ambiguity of the Uniform (long,long)
  vs Uniform (double,double). This is not neat. Therefore introduced fxns
  names UniformWhole and UniformFloat and depreciated the overloaded
  Uniform() fxns.
- 00.1.15: in earlier versions, the seed was set from a time_t, which has
  precision only down to a second. Thus a series of RNGs instantiated one
  after the other would produce the same stream of numbers. So the call
  to automatically init the seed was shifted to a seperate virtual fxn
  (so it can be overidden in derived classes) and now runs off clock_t.
- 00.2.15: SetSeed() was made virtual, so derived classes can do their
  own checks for bad seed values.
- 00.2.16: Also the Test() function has been placed within an #ifdef block
  (this should happen with all Test() functions) and a chi squared test
  introduced. Briefly, chi^2 is the sum of the squared frequencies,
  multiplied by the expected mean frequency, less the number of samples.
  This figure should be close to possible range / number of different
  results, and definitely within 2*sqrt(range). Or where N is number of
  samples, R is the range, Chi^2 = SUM[0<=i<R] (freq_i - N/R)^2 / (N/R).
  N should be 10 times R.
  

To Do:
- incorporate exceptions
- allow the arguments in range to be given in either order. Is this a good
  idea?
- write a template metaprogram for speed? In fact, should this be made
  into a template to avoid the type problem.
- should the seed be an unsigned long?

**************************************************************************/


// *** INCLUDES

#include "RandomService.h"

#include <ctime>
#include <limits>
#include <cmath>

using std::time;
using std::clock;
using std::numeric_limits;
using std::time_t;
using std::fmod;


SBL_NAMESPACE_START

// *** CONSTANTS & DEFINES

const long	kRandMax = 2147483647L;
const long	kA = 16807;         		


// *** MAIN BODY *********************************************************/

// *** LIFECYCLE *********************************************************/

RandomService::RandomService ()
{
	InitSeed();
}


RandomService::RandomService ( long iSeed )
{
	SetSeed(iSeed);
}


// INIT SEED
// This automagically sets the seed on construction. Can be overridden
// if desired. 
// !! Note: we don't know what the type returned by clock() will be, or
// the local size of a long. So we coerce everything to the same type
// with double() and use the fmod to ensure we don't get overflow on
// theDefaultSeed.
void RandomService::InitSeed ()
{
	long theDefaultSeed = long (fmod (double(clock()),
		double(numeric_limits<long>::max())));
	
	SetSeed(theDefaultSeed);
}


// *** ACCESS ************************************************************/
#pragma mark -
// SET SEED
// Note how seed is bulletproofed against being set to 0. If linear
// congruential generators are seeded with 0, the result is Bad.
void RandomService::SetSeed ( long iSeed )
{
	mSeed = iSeed ? (iSeed & kRandMax) : 1;
}


// *** INTERNALS *********************************************************/

double RandomService::Generate ()
//: Returns the next raw random number in the range 0 to 1.
// This is the basal function for generating "random-ness" and called
// (eventually) by all others to generate numbers in the appropriate
// intervals. It's one of two functions that can be over-ridden in a
// derived class.
{
/*
GOTCHA: note the typecasting to double in the last line. Without
this, the return from this function will always be 0, as the result of
a long divsion will be a long, and as it is less than 1, becomes 0.
*/
	unsigned long	theLo, theHi;

	theLo = (unsigned long) (kA * (long) (mSeed & 0xFFFF));
	theHi = (unsigned long) (kA * (long) ((unsigned long) mSeed >> 16));
	theLo += (theHi & 0x7FFF) << 16;
	if (theLo > kRandMax)
	{
	   theLo &= kRandMax;
	   ++theLo;
	}
	theLo += theHi >> 15;
	if (theLo > kRandMax)
	{
	   theLo &= kRandMax;
	   ++theLo;
	}

	mSeed = (long) theLo;
	return (double (mSeed) / double (kRandMax));
}


// *** RANDOM NUMBERS ****************************************************/

// *** UNIFORM DISTRIBUTION
// To avoid calling ambiguity and the need for typecasting, as detailed
// above.
#pragma mark -
double RandomService::UniformFloat ()
{
	return (Uniform ());
}

double RandomService::UniformFloat ( double iCeiling )
{
	return (Uniform (iCeiling));
}

double RandomService::UniformFloat ( double iFloor, double iCeiling )
{
	return (Uniform (iFloor, iCeiling));
}

long RandomService::UniformWhole ( long iNumChoices )
{
	return (Uniform (iNumChoices));
}

long RandomService::UniformWhole ( long iFloor, long iCeiling )
{
	return (Uniform (iFloor, iCeiling));
}


// *** OTHER DISTRIBUTIONS

// *** NORMAL DISTRIBUTION
#pragma mark -
double RandomService::NormalFloat ()
{
	double theTempRand = Generate () - 0.5;
	return (theTempRand * Generate ());
}

double RandomService::NormalFloat ( double iCeiling )
{
	return (NormalFloat() * iCeiling);
}

double RandomService::NormalFloat ( double iFloor, double iCeiling )
{
	assert (iFloor <= iCeiling);
	
	return (NormalFloat (iCeiling - iFloor) + iFloor);
}

long RandomService::NormalWhole ( long iNumChoices )
{
	return long (NormalFloat() * iNumChoices);
}

long RandomService::NormalWhole ( long iFloor, long iCeiling )
{
	assert (iFloor <= iCeiling);
	
	return (NormalWhole (iCeiling - iFloor + 1) + iFloor);
}


// *** DEPRECATED FUNCTIONS *********************************************/
#pragma mark -

// UNIFORM ()
// Returns a double from the interval 0 -> 1. While this function is the
// same as Generate() we have this as a seperate function in case the
// substructure changes later.
double RandomService::Uniform ()
{
	return (Generate ());
}

// UNIFORM (double)
// Returns a double from the interval 0 -> iCeiling.
double RandomService::Uniform ( double iCeiling )
{
	return (Generate() * iCeiling);
}

// UNIFORM (double,double)
// Returns a double from the interval iFloor -> iCeiling. Assumes
// (although it is probably a breakable assumption) that iFloor is
// less than iCeiling.
double RandomService::Uniform ( double iFloor, double iCeiling )
{
	assert (iFloor <= iCeiling);
	
	return (Uniform (iCeiling - iFloor) + iFloor);
}


// UNIFORM (long)
// Returns a long from the interval 0 -> (iNumChoices-1) inclusive.
// To put it another way, the parameter says how many possible choices
// there are and one is returned, numbering from zero up.
long RandomService::Uniform ( long iNumChoices )
{
	double	theFloatRand = Uniform();
	long		theLongRand = long (theFloatRand * double(iNumChoices));
	return (theLongRand);
}

// UNIFORM  (long,long)
// Returns a long from the interval iFloor -> iCeiling inclusive.
long RandomService::Uniform ( long iFloor, long iCeiling )
{
	assert (iFloor <= iCeiling);
	
	return (Uniform (iCeiling - iFloor + 1) + iFloor);
}


// *** TEST FUNCTIONS ****************************************************/

#ifdef SBL_DBG

#include <iostream>
//#include <iomanip>
#include <vector.h>


void ChiSqTest (RandomService* iThisObj, UInt iReps, UInt iRange);

template <typename element_t>
void
TestPrintVector (vector<element_t>& iVector)
{
	for (UInt i = 0; i < iVector.size(); i++)
		cout << iVector[i] << " ";
	cout << endl;
}


void RandomService::Test()
{
	DBG_MSG("*** Testing RandomService class");
	
	DBG_MSG("After seeding with 1 & 10K reps, seed == 1043618065");		
	SetSeed(1);
   for (int i = 0; i < 10000; ++i)
		(void) Generate();		
	DBG_MSG("After 10,000 reps, the seed is " << mSeed);
	
	InitSeed();
	
	DBG_MSG("Testing Uniform(5,10): longs 5-10 inclusive");	
   for (int i = 0; i < 20; ++i)
   {
   	vector<long> theResults;
   	for (int j = 0; j < 5; j++)
   		theResults.push_back(Uniform((long) 5,10));
   	DBG_ANON_VECTOR(theResults, 5);
	}	

	DBG_MSG("");
	DBG_MSG("Testing Uniform(5.0,10.0): floats 5.0-10.0 exclusive");		
   for (int i = 0; i < 10; ++i)
   {
   	vector<double> theResults;
   	for (int j = 0; j < 5; j++)
			theResults.push_back(Uniform((double) 5.0,10.0));
		DBG_ANON_VECTOR(theResults, 8);
	}
	
	DBG_MSG("");
	ChiSqTest (this, 1000, 100);
	
	
	DBG_MSG("*** Finished testing RandomService class");
}

// see comments in header
void ChiSqTest (RandomService* iThisObj, UInt iReps, UInt iRange)
{
	DBG_MSG("Chi squared test:")

	// generate a vector of random number frequencies
	vector<int>	theFreqs(iRange,0);
	for (UInt i = 0; i < iReps; i++)
		theFreqs[iThisObj->UniformWhole(iRange)]++;
	
	// calculate distribution
	int theSumSqs = 0;
	for (UInt i = 0; i < theFreqs.size(); i++)
		theSumSqs += theFreqs[i] * theFreqs[i];
	float theChiSq = double(iRange * theSumSqs) / double(iReps)  - double(iReps);

	DBG_MSG("reps=" << iReps << "; r=" << iRange << "±"
		<< (2.0*std::sqrt(double(iRange))) << "; Chi^2=" << theChiSq);
	
}


#else

void RandomService::Test() {}

#endif


SBL_NAMESPACE_STOP

// *** END ***************************************************************/

