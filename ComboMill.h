/**************************************************************************
ComboMill - generate combinations over a supplied sequence

Credits:
- By Paul-Michael Agapow, 2003.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- A reification of a combination generating algorithm that preserves state
  between calls. It is necessary to do this because otherwise it is hard
  to keep track of where the algorithm has gotten up to.

Changes:
- 00.4.26: Started the conversion for reasons stated above. 

To Do:
- It may still be possible to do a classless / stateless algorithm for
  combinations. Think about it.
- First_K_SubSet, Last_K_Subset, Next_K_SubSet?
- Make sure this cannot be called / constructed without an argument seq

**************************************************************************/


// *** INCLUDES

#include "Sbl.h"

#include <algorithm>
#include <iterator>

using std::iterator;
using std::vector;
using std::count;
using std::cout;
using std::endl;


// *** MAIN BODY *********************************************************/
#pragma mark --

template <typename container_t>
class ComboMill
{
private:

	typedef typename container_t::iterator		iterator;
	typedef typename container_t::size_type	size_type;
	
public:

	// *** LIFECYCLE
	
	ComboMill (iterator iStartIter, iterator iStopIter)
	{
		Init (iStartIter, iStopIter);
	}

	ComboMill (UInt iRangeStart, UInt iRangeStop)
	{
		InitRange (iRangeStart, iRangeStop);
	}
	
	ComboMill (UInt iRangeSize)
	{
		InitRange (0, iRangeSize - 1);
	}

	// LIFECYCLE HELPERS
	void Init (iterator iStartIter, iterator iStopIter)
	{
		mSeqStart = iStartIter;
		mSeqStop = iStopIter;
		
		// init bit vector
		iterator theCurrIter = mSeqStart;
		while (theCurrIter != mSeqStop)
		{
			mMembership.push_back(false);
			theCurrIter++;
		}
	}
	
	void InitRange (UInt iRangeStart, UInt iRangeStop)
	{
		for (UInt i = iRangeStart; i <= iRangeStop; i++)
			mRange.push_back(i);
		Init (mRange.begin(), mRange.end());
	}
	
	// *** ACCESS
	
	void GetCurrent (iterator iStartOut, iterator& oStopOut)
	{	
		Get (iStartOut, oStopOut, true);
	}

	void GetComplement (iterator iStartOut, iterator& oStopOut)
	{	
		Get (iStartOut, oStopOut, false);
	}

	void Get (iterator iStartOut, iterator& oStopOut, bool iState)
	{	
		iterator theCurrIter = mSeqStart;
		oStopOut = iStartOut;

		for (int i = 0; i < mMembership.size(); i++)
		{
			assert (theCurrIter != mSeqStop);
			
			if (mMembership[i] == iState)
			{
				*oStopOut = *theCurrIter;
				// cout << *oStopOut << " " << *theCurrIter << endl;
				oStopOut++;
			}
			
			theCurrIter++;
		}
	}
	
	UInt Size ()
	{
		int	theResult;
		theResult = count (mMembership.begin(), mMembership.end(), 1);
		return theResult;
	}
	
	void SetMembership (bool iIsMember)
	{
		for (int i = 0; i < mMembership.size(); i++)
		{
			mMembership[i] = iIsMember;
		}
	}
	
	// *** MUTATION

	// INCREMENT
	// Note how this wraps from the last combo to the first combo
	// To Do: make operator++
	void Next ()
	{
		assert (0 < mMembership.size());
		
		for (int i = 0; i < mMembership.size(); i++)
		{
			if (mMembership[i] == false)
			{
				mMembership[i] = true;
				break;
			}
			else
			{
				mMembership[i] = false;
			}
		}
	}
	
	void Previous ()
	{
		assert (0 < mMembership.size());
		
		for (int i = mMembership.size() - 1; 0 <= i; i--)
		{
			if (mMembership[i] == true)
			{
				mMembership[i] = false;
				break;
			}
			else
			{
				mMembership[i] = true;
			}
		}
	}
	
	void First ()
	{
		this->SetMembership (false);
	}
	
	void Last ()
	{
		this->SetMemberShip (true);
	}

	// is it all zeros?
	bool IsFirst ()
	{
		for (int i = 0; i < mMembership.size(); i++)
		{
			if (mMembership[i] == true)
				return false;
		}
		return true;
	}

	// is it all ones?
	bool IsLast ()
	{
		for (int i = 0; i < mMembership.size(); i++)
		{
			if (mMembership[i] == false)
				return false;
		}
		return true;
	}
	
	
	// *** KJ SUBSETS
	// That is, subsets or combinations of a size from K to J inclusive.

	void FirstKJ (UInt iLowerBound, UInt iUpperBound)
	{
		assert (iLowerBound <= iUpperBound);
		assert (0 <= iLowerBound);
		assert (iUpperBound <= mMembership.size());
		
		First();
		if ((Size() < iLowerBound) or (iUpperBound < Size())) 
			NextKJ (iLowerBound, iUpperBound);
	}
	
	void NextKJ (UInt iLowerBound, UInt iUpperBound)
	{
		do
		{
			Next ();
		}
		while ((Size() < iLowerBound) or (iUpperBound < Size()));
	}
	
	void PreviousKJ (UInt iLowerBound, UInt iUpperBound)
	{
		do
		{
			Previous ();
		}
		while ((Size() < iLowerBound) or (iUpperBound < Size()));
	}
	
	void LastKJ (UInt iLowerBound, size_type iUpperBound)
	{
		assert (iLowerBound <= iUpperBound);
		assert (0 <= iLowerBound);
		assert (iUpperBound <= mMemberShip.size());
		
		Last();
		if ((Size() < iLowerBound) or (iUpperBound < Size())) 
			PreviousKJ (iLowerBound, iUpperBound);
	}
	
	// K SUBSETS

	void FirstK (UInt iSubsetSize)
	{
		assert (0 <= iSubsetSize);
		assert (iSubsetSize <= mMembership.size());
		
		FirstKJ (iSubsetSize, iSubsetSize);
	}
	
	void NextK (UInt iSubsetSize)
	{
		NextKJ (iSubsetSize, iSubsetSize);
	}
	
	void PreviousK (UInt iSubsetSize)
	{
		PreviousKJ (iSubsetSize, iSubsetSize);
	}
	
	void LastK (UInt iSubsetSize)
	{
		LastKJ (iSubsetSize, iSubsetSize);
	}
	
	// *** DEPRECIATED, DEBUG, DEV
	
	void Dump ()
	{
		cout << "*** Dumping contents of Combomill at " << &this << ":" << endl;
		cout << "* Subject container:" << endl;
		this->PrintContainer (mSeqStart,mSeqStop);
		cout << "* Membership vector:" << endl;
		this->PrintContainer (mMembership);
	}

	// *** INTERNALS
		
	vector<bool>	mMembership;
	iterator			mSeqStart;
	iterator			mSeqStop;
	vector<UInt>	mRange;
	

	
};


// *** DEPRECIATED FUNCTIONS *********************************************/

using std::cout;
using std::endl;


void TestComboMill ();

template <class ForwardIter>
void PrintContainer (ForwardIter start, ForwardIter stop)
{
	cout << "Container contents: ";

	if (start != stop)
	{
		cout << *start;
		for (start++; start != stop; start++)
			cout << ", " << *start;
	}
	else
	{
		cout << "-";
	}
	cout << endl;
}

template <class Container>
void PrintContainer (Container& theGroup)
{
	PrintContainer (theGroup.begin(), theGroup.end());
}

#define TESTARRSIZE 5

void TestComboMill ()
{
	cout << "Testing ComboMill" << endl;

	// init test array
	vector<int>	theTestArr (TESTARRSIZE);
	for (int i = 0; i < TESTARRSIZE; i++)
		theTestArr[i] = 10 - i;
	
	// init result array
	vector<int> theResArr(TESTARRSIZE);
	
	// build combo mill
	ComboMill< vector<int> > theTestMill (theTestArr.begin(), theTestArr.end());
	cout << "The whole set is:" << endl;
	PrintContainer(theTestArr);
	
	vector<int>::iterator theSetIter = theResArr.begin();

	cout << "*** Generating combinations ..." << endl;
	theTestMill.First();	
	for (int i = 0; i < 40; i++)
	{
		//theTestMill.Dump();
		theTestMill.GetCurrent (theResArr.begin(), theSetIter);
		PrintContainer(theResArr.begin(), theSetIter);
		theTestMill.Next();
	}

	cout << "*** Generating k subsets ..." << endl;
	theTestMill.FirstK(2);	
	for (int i = 0; i < 40; i++)
	{
		//theTestMill.Dump();
		theTestMill.GetCurrent (theResArr.begin(), theSetIter);
		PrintContainer(theResArr.begin(), theSetIter);
		theTestMill.NextK(2);
	}

	cout << "*** Generating kj subsets ..." << endl;
	theTestMill.FirstKJ(2,3);	
	for (int i = 0; i < 40; i++)
	{
		//theTestMill.Dump();
		theTestMill.GetCurrent (theResArr.begin(), theSetIter);
		PrintContainer(theResArr.begin(), theSetIter);
		theTestMill.NextKJ(2,3);
	}
	
	
	cout << "Finished testing ComboMill" << endl;
}


// *** END ***************************************************************/



