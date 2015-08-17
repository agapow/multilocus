/**************************************************************************
Combination.cpp - a simple collection of objects

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.agapow.net>

About:
- A class that provides the various combinations (unique unordered subsets)
  possible from a set of a given size.

**************************************************************************/

#ifndef COMBINATION_H
#define COMBINATION_H


// *** INCLUDES

#include "Sbl.h"
#include <vector>
#include <algorithm>

using namespace sbl;

using std::vector;


// *** CLASS DECLARATION *************************************************/

class Combination
{
public:
	// Lifecycle
	Combination	() {};
	Combination (UInt iFirstElement)
		{ Add (iFirstElement); }
	Combination (vector<UInt>& iFirstElements)
		{
			for (UInt i = 0; i < iFirstElements.size(); i++)
				Add (iFirstElements[i]);
		}
		
	// Access
	UInt	Size	()	
		{ return mContents.size(); }
	bool	Member	(UInt iElement)	
		{
			for (UInt i = 0; i < mContents.size(); i++)
				if (mContents[i] == iElement) return true;
			return false;
		}	
	UInt&	operator[]	(UInt iIndex)
		{ 
			return at (iIndex);
		}
	UInt&	at (UInt iIndex)
		{ 
			assert ((0 <= iIndex) and (iIndex < Size()));
			return mContents[iIndex];
		}
		
	// Manipulation
	void 	Add	(UInt	iNewElement)
		{
			if (Member (iNewElement))
				return;
			mContents.push_back (iNewElement);
		}				

	void 	Sort	()
		{
			std::sort (mContents.begin(), mContents.end());
		}				
	
	// Depreciated & Debug
	void Dump ()
	{
		DBG_MSG("Combination of size " << Size());
		DBG_VECTOR(&mContents);
	}
private:
	// Internals
	vector<UInt>	mContents;
};


#endif

// *** END ***************************************************************/







