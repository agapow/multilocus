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

**************************************************************************/

#ifndef COMBINATIONMILL_H
#define COMBINATIONMILL_H


// *** INCLUDES

#include "Combination.h"
#include "RandomService.h"
#include <vector>

using std::vector;
using namespace sbl;


// *** CONSTANTS & DEFINES

typedef vector<Combination>	comboVector_t;	


// *** CLASS DECLARATION *************************************************/

class CombinationMill
{
public:
	// Lifecycle
	CombinationMill	(int	iSetSize);
				
	// Access
	void	GetCombinations (comboVector_t& oCombinations, UInt iComboSize);
	void	MakeComplement	(Combination& iOrigSet, Combination& oComplement);
	void 	GetRandomCombination (Combination& oCombo, UInt iComboSize);
							
	// Depreciated & Debug
	
private:
	// Internals
	void	AddCombination	(comboVector_t& oNewCombos, UInt iStart, UInt iSize);
	UInt	mSetSize;			// the set size
	
	CombinationMill	();	// so it cannot get called
	
	sbl::RandomService	mRng;
};


#endif

// *** END ***************************************************************/







