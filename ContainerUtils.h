/**************************************************************************
StringUtils.h - assorted container utility functions 

Credits:
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- Some templated functions for use with most containers.
- StringsUtils has some specialisations of these.

Changes:
- 00.12.8: created from portions of StringsUtils..

To Do:
- 

**************************************************************************/

#ifndef CONTAINERUTILS_H
#define CONTAINERUTILS_H


#include "Sbl.h"
#include <algorithm>

SBL_NAMESPACE_START


// *** CONSTANTS, DEFINES & PROTOTYPES
#pragma mark Constants


// *** MEMBERSHIP FUNCTIONS **********************************************/
#pragma mark -

template <typename TERMITER, typename TARGETITER>
bool
isMemberOf
(TERMITER iStartTerms,  TERMITER iStopTerms, TARGETITER iStartTarget,
	TARGETITER iStopTarget)
//: can any of terms in the first container be found in the second?
// This only tells you whether any of the search terms are in the target,
// not where they are or how many times they occur. Use the raw STL
// algorithms for that.
{
// Generalized to non-string containers, the design of this fxn deserves
// some explanation. If either the iterator or the search term are passed
// in via references (which you would do if you wanted to pass out the
// position of the found term or just save time copying) then you can't
// make  calls like this: "isMemberOf ('c', myStr.begin(), myStr.end());"
// as neither the first iterator nor the searchterm can be modified.
// TO DO: should this be const?

	// for each value in the search terms container
	for (; iStartTerms != iStopTerms; iStartTerms++)
	{
		// see if you can find it in the target container
		if (std::find (iStartTarget, iStopTarget, *iStartTerms) != iStopTarget)
			return true;
	}
	return false;
}


template <typename X, typename TARGETITER>
bool
isMemberOf
(X iSearchTerm, TARGETITER iStartTarget, TARGETITER iStopTarget)
//: can the search term be found in the target container?
// See notes on isMemberOf (iter1, iter1, iter2, iter2).
{
	if (std::find (iStartTarget, iStopTarget, iSearchTerm) == iStopTarget)
		return false;
	else
		return true;
}


template <typename TERMITER, typename TARGETITER>
bool
isSubsetOf
(TERMITER iStartTerms,  TERMITER iStopTerms, TARGETITER iStartTarget,
	TARGETITER iStopTarget)
//: can all of terms in the first container be found in the second?
// This only tells you whether all of the search terms are in the target,
// not where they are or how many times they occur.
{
	// for each value in the search terms container
	for (; iStartTerms != iStopTerms; iStartTerms++)
	{
		// see if you can find it in the target container
		if (std::find (iStartTarget, iStopTarget, *iStartTerms) == iStopTarget)
			return false;
	}
	return true;
}


// *** DEPRECIATED, DEBUG, DEVELOPMENT ***********************************/
#pragma mark -

#if 0
// To run test function, set to 1.
// Note this may generate a "function has no prototype" warning.

	void testContainerUtils ()
	//: Simply a test suite for the functions within this module.
	{
		DBG_MSG ("*** Testing container utils ...");

		DBG_MSG ("");
		DBG_MSG ("* Testing membership functions:");
		
		int	theArr1[] = {1, 2, 3};
		int	theArr2[] = {3};
		int	theArr3[] = {4, 6, 2};
		DBG_MSG ("Array 1 is {1, 2, 3}");
		DBG_MSG ("Array 2 is {3}");
		DBG_MSG ("Array 3 is {4, 6, 2}");
		
		bool theResult[6];
		theResult[0] = isMemberOf (theArr1, theArr1+3, theArr2, theArr2+1);
		theResult[1] = isMemberOf (theArr1, theArr1+3, theArr3, theArr3+3);
		theResult[2] = isMemberOf (theArr2, theArr2+1, theArr1, theArr1+3);
		theResult[3] = isMemberOf (theArr2, theArr2+1, theArr3, theArr3+3);
		theResult[4] = isMemberOf (theArr3, theArr3+3, theArr1, theArr1+3);
		theResult[5] = isMemberOf (theArr3, theArr3+3, theArr2, theArr2+1);
		
		DBG_MSG ("The seq membership results are:");
		DBG_MSG ("  is array 1 in array 2 (expect t): " << theResult[0]);
		DBG_MSG ("  is array 1 in array 3 (expect t): " << theResult[1]);
		DBG_MSG ("  is array 2 in array 1 (expect t): " << theResult[2]);
		DBG_MSG ("  is array 2 in array 3 (expect f): " << theResult[3]);
		DBG_MSG ("  is array 3 in array 1 (expect t): " << theResult[4]);
		DBG_MSG ("  is array 3 in array 2 (expect f): " << theResult[5]);

		DBG_MSG ("The single membership results are:");
		DBG_MSG ("  is 2 in array 1 (expect t): " << isMemberOf (2,
			theArr1, theArr1+3));
		DBG_MSG ("  is 2 in array 2 (expect f): " << isMemberOf (2,
			theArr2, theArr2+1));
		DBG_MSG ("  is 2 in array 3 (expect t): " << isMemberOf (2,
			theArr3, theArr3+3));
			
		DBG_MSG ("Finished testing container utils.");
	}

#endif


SBL_NAMESPACE_STOP

#endif

// *** END ***************************************************************/