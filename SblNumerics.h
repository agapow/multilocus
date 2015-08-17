/**************************************************************************
SblNumerics - utility math functions

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve>

About:
- A group of math functions, largely generic and using valarrays

Changes:
- 99.8.7: Created.

To Do:
- other sort functions?
- can heapsort be set to work in descending order?

**************************************************************************/


// *** INCLUDES

#include "Sbl.h"

#include <valarray>
#include <cassert>
#include <vector>
#include <numeric>
#include <iterator>
#include <cmath>
#include <iostream>
#include <iomanip>

using std::valarray;
using std::vector;
using std::size_t;
using std::accumulate;
using std::pow;
using std::iterator;
using std::iterator_traits;
using namespace sbl;
using std::setw;
using std::cout;
using std::endl;




// *** INTERFACE *********************************************************/

UInt CountSamplesFromPopulation (UInt iPopulationSize, UInt iSampleSize);
UInt CountCombinations (UInt iPopulationSize, UInt iSampleSize);
UInt Factorial (UInt iOperand);


// SWAP
// Exchange two variables of any type that can be assigned.
template <typename element_t>
void Swap (element_t &ioElem1, element_t &ioElem2)
{
	element_t theTmp = ioElem1;
	ioElem1 = ioElem2;
	ioElem2 = theTmp;
}


// Utility functions for heapsort
inline size_t parent (size_t k)
{
	return ((k - 1)/2);
}

inline size_t leftchild (size_t k)
{
	return (2*k + 1);
}

inline size_t rightchild (size_t k)
{
	return (2*k + 2);
}


// HEAPSORT (valarray)
// A generic function for sorting valarrays in place, in ascending order.
template <typename element_t>
void Heapsort(valarray<element_t> &data)
{
	size_t unsorted = data.size();

	for (size_t i = 1; i < unsorted; i++)
	{
		// where i is index of next element to be added to heap &
		// k is index of new element as it pushed upward thru heap
		size_t k = i;
		while ((k > 0) && (data[k] > data[parent(k)]))
		{
			Swap (data[k], data[parent(k)]);
			k = parent(k);
		}
	}

	while (unsorted > 1)
	{
		unsorted--;
		Swap (data[0], data[unsorted]);

		size_t	current = 0;		// index of node that's moving down
		size_t	big_child_index;	// index of larger child of current node
		bool		heap_ok = false;	// is heap correct?

		// keep going until heap is okay, and while current node has at
		// least a left child (leftchild (current) < iBound)
		while ((!heap_ok) && (leftchild (current) < unsorted))
		{
			// which is the larger child:
			if (rightchild (current) >= unsorted)
			{
				// no right child, so left child must be largest
				big_child_index = leftchild(current);
			}
			else if (data[leftchild (current)] > data[rightchild (current)])
			{
				// the left child is the bigger
				big_child_index = leftchild(current);
			}
			else
			{
				// the right child is the bigger
				big_child_index = rightchild(current);
			}

			// Is larger child > the current node? If so, swap
			// and continue; otherwise we are finished.
			if (data[current] < data[big_child_index])
			{
				Swap (data[current], data[big_child_index]);
				current = big_child_index;
			}
			else
				heap_ok = true;
		}
	}
}


// HEAPSORT (vector)
// A generic function for sorting vector in place, in ascending order.
template <typename element_t>
void Heapsort(vector<element_t> &data)
{
	size_t unsorted = data.size();

	for (size_t i = 1; i < unsorted; i++)
	{
		size_t k = i;
		while ((k > 0) && (data[k] > data[parent(k)]))
		{
			Swap (data[k], data[parent(k)]);
			k = parent(k);
		}
	}

	while (unsorted > 1)
	{
		unsorted--;
		Swap (data[0], data[unsorted]);

		size_t	current = 0;		// index of node that's moving down
		size_t	big_child_index;	// index of larger child of current node
		bool		heap_ok = false;	// is heap correct?

		while ((!heap_ok) && (leftchild (current) < unsorted))
		{
			// which is the larger child:
			if (rightchild (current) >= unsorted)
			{
				// no right child, so left child must be largest
				big_child_index = leftchild(current);
			}
			else if (data[leftchild (current)] > data[rightchild (current)])
			{
				// the left child is the bigger
				big_child_index = leftchild(current);
			}
			else
			{
				// the right child is the bigger
				big_child_index = rightchild(current);
			}

			// Is larger child > the current node? If so, swap
			// and continue; otherwise we are finished.
			if (data[current] < data[big_child_index])
			{
				Swap (data[current], data[big_child_index]);
				current = big_child_index;
			}
			else
				heap_ok = true;
		}
	}
}

// HEAPSORT (array, size)
// A generic function for sorting arrays in place, in ascending order.
template <typename element_t>
void Heapsort (element_t data[], size_t n)
{
	size_t unsorted = n;

	for (size_t i = 1; i < unsorted; i++)
	{
		size_t k = i;
		while ((k > 0) && (data[k] > data[parent (k)]))
		{
			Swap (data[k], data[parent(k)]);
			k = parent (k);
		}
	}

	while (unsorted > 1)
	{
		unsorted--;
		Swap (data[0], data[unsorted]);

		size_t	current = 0;	
		size_t	big_child_index;
		bool		heap_ok = false;

		while ((!heap_ok) && (leftchild (current) < unsorted))
		{
			if (rightchild (current) >= unsorted)
			{
				big_child_index = leftchild(current);
			}
			else if (data[leftchild (current)] > data[rightchild (current)])
			{
				big_child_index = leftchild(current);
			}
			else
			{
				big_child_index = rightchild(current);
			}

			if (data[current] < data[big_child_index])
			{
				Swap (data[current], data[big_child_index]);
				current = big_child_index;
			}
			else
				heap_ok = true;
		}
	}
}





// *** DEBUG

// DEBUG PRINT (valarray)
// For dumping to screen
template <typename element_t>
void DebugPrint ( valarray<element_t> &data )
{
	cout << "*** DEBUG: Contents of valarray at " << &data << endl;
	for (int i = 0; i < data.size(); i++)
		cout << setw(5) << i << " : " << data[i] << endl;
	cout << "*** END DEBUG" << endl;
}

// AVERAGE (valarray)
// Specialise this for ints?
// !! Note that valarray doesn't support iterators. 
template <typename element_t>
void Average (valarray<element_t>& data, double& oAvg, double& oVariance)
{
	size_t theNumItems = data.size ();
	assert (1 < theNumItems);
	
	oAvg = data.sum () / theNumItems;
	valarray<element_t> theDevVector = data - oAvg;

	element_t theVal, theSumSq = 0;
	for (size_t i = 0; i < theNumItems; i++)
	{
		theVal = theDevVector[i];
		theSumSq += theVal * theVal;
	}

	oVariance = theSumSq / theNumItems;
}


// SUM OF SQUARED DEVIATIONS (valarray)
// Calculates Sum(x_obs - x_mean)^2
/*
template <typename element_t>
double SumSqDev (valarray<element_t>& data)
{
	double theMean = sum;
}
 */


// STANDARD DEVIATION
// Given a valarray, calculates & returns the mean and std dev (by
// reference).
template <typename T>
T StdDeviation (T& oStdDev, valarray<T>& ikValues)
{
	T				theMean;
	valarray<T>	theDeviations;
	
	theMean = ikValues.sum() / T(ikValues.size());
	theDeviations = ikValues - theMean;
	theDeviations *= theDeviations;
	oStdDev = theDeviations.sum() / T(ikValues.size() - 1);
	
	oStdDev = pow(oStdDev, .5);
	return theMean;
}

// STANDARD ERROR
// Given a valarray, calculates & returns the mean and std error (by
// reference).
template <typename T>
T StdError (T& oStdError, valarray<T>& ikValues)
{
	T	theMean;
	T	theStdDev;
	
	theMean = StdDeviation (theStdDev, ikValues);
	oStdError = theStdDev / sqrt(T(ikValues.size()));
	return theMean;
}



/*
template <class InputIter>
inline
typename iterator_traits<InputIter>::value_type
sum (InputIter first, InputIter last, iterator_traits<InputIter>::value_type init = 0)
{
	for (; first != last; ++first)
		init = init + *first;
	return init;
}
 */


// *** END
