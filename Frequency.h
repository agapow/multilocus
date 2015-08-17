/**************************************************************************
Frequency.h - for counting occurrences

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- There appears to be problems in the CodeWarrior implementation of map,
  which prevents it from being used to count how many times something
  occurs. This is a simple utility class to aid the collation and use of
  frequency data.
- To illustrate the map problem, this is some code where "function does
  not match" is reported on the insert function. make_pair and a static
  pair have been used to try and get around it to no avail. Possibly this
  is to do with my ignorance of C++
  
	void MultiLocusModel::MapCount (map<string, int>& myMap, string& iStr)
	{
		map<string, int>::iterator	p;
		p = ioCountMap.find (iStr);
		if (p != myMap.end())		// if already stored
			(p->second)++;				
		else								// if previously unstored
			myMap.insert(pair<string,int>(iStr,1));	
	}
	
- The original frequency representation is now superseded (but retained for
  compatibility purposes) in favour of a more flexible templated version
  that can accept keys of any type. Cool.
 
Changes:
- 00.3.1: Tidied and documented.

To Do:
- place in sibil namespace?

**************************************************************************/

#ifndef FREQUENCY_H
#define FREQUENCY_H


// *** INCLUDES

#include "Sbl.h"

#include <vector>
#include <string>

using std::vector;


// *** CONSTANTS & DEFINES

// *** CLASS DECLARATION *************************************************/


template <typename X>
class TFrequency
{
public:

// *** LIFECYCLE
	// all default constructors, copy & destructors


// *** ACCESS

	// Return value (frequency) of given key, 0 if it doesn't exist
	int	Value		(const X& iQueryKey)
	{
		for (UInt i = 0; i < mKeys.size(); i++)
		{
			if (mKeys[i] == iQueryKey)
			{
				return mFreqs[i];
			}
		}
		// if we get this far, the key isn't in the vector
		return 0;
	}

	// Return sum of values
	int	Value	()
	{
		int	theTotalValue = 0; 
		for (UInt i = 0; i < mKeys.size(); i++)
			theTotalValue += mFreqs[i];

		return theTotalValue;
	}

	// For iteration; return value (or key) by index of entry
	int&		ValueByIndex	(UInt iIndex) { return mFreqs[iIndex]; }	
	X			KeyByIndex		(UInt iIndex) { return mKeys[iIndex]; }	


// *** MANIPULATIONS

	// Increase frequency. Add to List if not present.
	void	Increment	(X& iIncrKey, int iNewVal = 1)
	{
		for (UInt i = 0; i < mKeys.size(); i++)
		{
			if (mKeys[i] == iIncrKey)
			{
				mFreqs[i] += iNewVal;
				return;
			}
		}
		// if we get this far, the key isn't in the vector
		mKeys.push_back (iIncrKey);
		mFreqs.push_back (iNewVal);
	}
	
	// Remove entry with this key
	void		Erase				(const X& iEraseKey)
	{
		for (UInt i = 0; i < mKeys.size(); i++)
		{
			if (mKeys[i] == iEraseKey)
			{
				mKeys.erase (mKeys.begin() + i);
				mFreqs.erase (mFreqs.begin() + i);
				return;
			}
		}
		
		// !! if we get to here, the key isn't in the vector which is
		// is not necessarily an error
	}
	
	// Add frequencies of other list
	void	Add	(TFrequency<X>& iNewEntries)
	{
		for (UInt i = 0; i < iNewEntries.Size(); i++)
			Increment (iNewEntries.KeyByIndex(i), iNewEntries.ValueByIndex(i));
	}
	
	// How many entries (keys) are there in the list
	int Size () { return mKeys.size(); }
	
	
// *** DEBUGGING & DEPRECIATED

	void Dump ()
	{
		DBG_MSG ("Frequency contains " <<  Size() << " keys");
		for (UInt i = 0; i < Size(); i++)
			DBG_MSG ("\tKey " << i << ": \"" << mKeys[i] << "\" -> " << mFreqs[i]);
	}


// *** INTERNALS
	
private:
	vector<X>			mKeys;
	vector<int>			mFreqs;	
};


// *** DEPRECIATED *******************************************************/
// This is the old non-template version of the class. Can only accept
// strings as keys.

class Frequency
{
public:
	// LIFECYCLE
		// all default constructors, copy & destructors

	// ACCESS
	// Return value (frequency) of given key, 0 if it doesn't exist
	int	Value		(const char* iKeyStr)
	{
		string	theKeyStr (iKeyStr);
		return Value (theKeyStr);
		// return Value (string (iKeyStr));
	}
	int	Value		(string& iKeyStr)
	{
		for (UInt i = 0; i < mKeys.size(); i++)
		{
			if (mKeys[i] == iKeyStr)
			{
				return mFreqs[i];
			}
		}
		// if we get this far, the key isn't in the vector
		return 0;
	}

	// Return sum of values
	int	Value	()
	{
		int	theTotalValue = 0; 
		for (UInt i = 0; i < mKeys.size(); i++)
			theTotalValue += mFreqs[i];

		return theTotalValue;
	}

	// For iteration; return value (or key) by index of entry
	int&		ValueByIndex	(UInt iIndex) { return mFreqs[iIndex]; }	
	string&	KeyByIndex		(UInt iIndex) { return mKeys[iIndex]; }	

	// MANIPULATIONS
	// Increase frequency. Add to List if not present.
	void	Increment	(string& iKeyStr, int iNewVal = 1)
	{
		for (UInt i = 0; i < mKeys.size(); i++)
		{
			if (mKeys[i] == iKeyStr)
			{
				mFreqs[i] += iNewVal;
				return;
			}
		}
		// if we get this far, the key isn't in the vector
		mKeys.push_back (iKeyStr);
		mFreqs.push_back (iNewVal);
	}
	
	// Remove entry with this key
	void		Erase				(const char* iKeyStr)
	{
		string	theKeyStr (iKeyStr);
		Erase (theKeyStr);
	}
	void		Erase				(string& iKeyStr)
	{
		for (UInt i = 0; i < mKeys.size(); i++)
		{
			if (mKeys[i] == iKeyStr)
			{
				mKeys.erase (mKeys.begin() + i);
				mFreqs.erase (mFreqs.begin() + i);
				return;
			}
		}
		// if we get to here, the key isn't in the vector
	}
	
	// Add frequencies of other list
	void	Add	(Frequency& iNewEntries)
	{
		// DBG_DUMP(iNewEntries);
		for (UInt i = 0; i < (UInt) iNewEntries.Size(); i++)
			Increment (iNewEntries.KeyByIndex(i), iNewEntries.ValueByIndex(i));
	}
	
	// How many entries (keys) are there in the list
	int Size () { return mKeys.size(); }
	
	// DEBUGGING & DEPRECIATED
	void Dump ()
	{
		DBG_MSG ("Frequency contains " <<  Size() << " keys");
		for (UInt i = 0; i < (UInt) Size(); i++)
		{
			DBG_MSG ("\tKey " << i << ": \"" << mKeys[i] << "\" -> " << mFreqs[i]);
		}
	}

	
private:
	vector<string>		mKeys;
	vector<int>			mFreqs;	
};

#endif

// *** END ***************************************************************/

