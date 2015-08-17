/**************************************************************************
CombinationMill.cpp - provides combinations of sets

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.agapow.net>

About:
- A class that provides the various combinations (unique unordered subsets)
  possible from a set of a given size.
- While there is a temptation for this class to store the combinations
  after it has generated them, there seems little reason. There are few
  circumstances when an instantiation will generate a set of combinations
  and later have to generate exactly the same set. The user can always
  store the result. Furthermore, and despite initial impressions,
  combinations cannot be recursively generated from combinatsion of other
  size. (i.e. knowing the list of all combinations of size 4 doesn't get
  you part of the way to knowing the lists of size 3 or 5 combinations.)

Changes:
- 99.6.6: Created.
- 99.11.16: Got it wrong. Do it again. Tested and verified. Test function
  embedded. Defined Combination as a type in its own right so it can be
  given isMemberOf() as a function.

To Do:
- extend to covering divisions and partitions.
- currently we assume combinations run from 0 to mSetSize. Perhaps there
  should be a constructor that allows the sequence to run from any int to
  any int. (Or perhaps any arbitary sequence!)
- Is there a better name than "Mill" for this sort of service object?
  "Service" like "RandomService"? The whole nomenclature is less than
  obvious but this is largely aesthetic. "ComboService"?
- The Stony Brook algorithm repository has some possible alternative (and
  maybe better) implementations of this problem. 
- Could perhaps use a GetRandomCombo(), GetNextCombo(), GetAllCombos(),
  GetPartition (); It also occurs to me that isMemberOf(), AddUnique(),
  MergeUnique() and so on could be implemented as abstract Stl algorithms.

**************************************************************************/


// *** INCLUDES

#include "CombinationMill.h"
#include "RandomService.h"


// *** CONSTANTS & DEFINES

void Test ();


// *** MAIN BODY *********************************************************/

// *** LIFECYCLE METHODS *************************************************/

// Combinations _must_ have a set size defined at creation. This cannot
// be changed afterwards. (Why would you want to? Why not create another
// Mill?)
CombinationMill::CombinationMill (int	iSetSize)
{
	mSetSize = iSetSize;
}


// *** PUBLIC METHODS ****************************************************/

// *** PRODUCTION METHODS

void CombinationMill::GetCombinations
(comboVector_t& oCombinations, UInt iComboSize)
{
	assert (0 < iComboSize);
	assert (iComboSize <= mSetSize);
	
	AddCombination (oCombinations, 0, iComboSize);
}


void CombinationMill::GetRandomCombination (Combination& oCombo, UInt iComboSize)
{
	assert (iComboSize <= mSetSize);
	
	// pick iComboSize members from set
	for (UInt i = 0; i < iComboSize; i++)
	{
		// pick a random element until you get one you haven't picked before
		int theChoice;
		do
		{
			theChoice = mRng.UniformWhole(mSetSize);
		}
		while (oCombo.Member(theChoice) == true);
		oCombo.Add(theChoice);
	}
}


void	CombinationMill::MakeComplement
(Combination& iOrigSet, Combination& oComplement)
{
	for (UInt i = 0; i < mSetSize; i++)
	{
		if (not iOrigSet.Member(i))
			oComplement.Add (i);
	}

}


// *** PRIVATE METHODS ***************************************************/

// ADD COMBINATION
// !! Produces the vector of combinations of the parameter size and stores
// it in the classes vector of combos. 
void CombinationMill::AddCombination
(comboVector_t& oNewCombos, UInt iStart, UInt iSize)
{
	assert (0 < iSize);
	assert (iSize <= (mSetSize - iStart));

	oNewCombos.clear ();
	
	if (iSize == 1)
	{
		for (UInt i = iStart; i < mSetSize; i++)
		{
			Combination	theNewCombo (i);
			oNewCombos.push_back (theNewCombo);
		}
	}
	else
	{
		for (UInt i = iStart; i <= (mSetSize - iSize); i++)
		{
			// get the combos that are one size smaller
			comboVector_t	theSubCombos;
			AddCombination (theSubCombos, i + 1, iSize - 1);
			// append current index to it
			for (UInt j = 0; j < theSubCombos.size(); j++)
				theSubCombos[j].Add (i);
			oNewCombos.insert (oNewCombos.end(), theSubCombos.begin(),
				theSubCombos.end());
		}	
	}
}


// *** DEPRECIATED & TEST FUNCTIONS **************************************/

// TEST
// Purely for testing the class and illustrative purposes
void Test ()
{
	DBG_MSG ("Testing ComboService ...");
	UInt 					theSetSize = 8;
	UInt					theComboSize = 6;
	CombinationMill	theMill (theSetSize);
	comboVector_t		theCombos;
	theMill.GetCombinations (theCombos, theComboSize);

	DBG_MSG ("The combovector has a setsize of " << theSetSize << " and a combo size of " << theComboSize);
	DBG_MSG ("The combovector has " << theCombos.size() << " combinations.");
	for (UInt i = 0; i < theCombos.size(); i++)
	{
		for (UInt j = 0; j < theCombos[i].Size(); j++)
			DBG_MSG ("combo " << i << ", element " << j << ": " << theCombos[i][j] );
	
	}
	DBG_MSG ("Finished testing ComboService.");
}


// *** END ***************************************************************/
