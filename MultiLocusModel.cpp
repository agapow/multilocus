/**************************************************************************
MultiLocusModel.cpp - the domain model and data structures

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.agapow.net>

About:
- Ideally this should be completely divorcable from any interface or
  application loop so as to be portable to other architectures.

Changes:
- 99.8.13: Created.

To Do:
- See comments in main body.

**************************************************************************/


// *** INCLUDES
#pragma mark Includes

#include "MultiLocusModel.h"

#include "StreamScanner.h"
#include "StringUtils.h"
#include "Frequency.h"
#include "CombinationMill.h"
#include "Combination.h"
#include "ComboMill.h"
#include "SblNumerics.h"
#include "Error.h"

#include <cstring>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <map>
#include <iostream>

using std::strlen;
using std::string;
using std::count;
using std::find_if;
using std::isalnum;
using std::isdigit;
using std::map;
using std::pair;
using std::make_pair;
using std::sqrt;
using std::ios;
using std::setw;
using std::swap;
using std::find;
using std::right;
using std::endl;
using std::cout;
using sbl::isMemberOf;
using sbl::StrMember;
using sbl::String2Int;
using sbl::StringConcat;


// *** CONSTANTS & DEFINES
#pragma mark Constants

bool IsNotDigit (char& iChar);

#define ASSERT_VALIDINDEX(x,y)	assert(0 <= x and x < (int) GetNumRows()); \
											assert(0 <= y and y < (int) GetNumCols())

enum pval_t
{
	kPval_NumDiff = 0,
	kPval_MaxFreq,
	kPval_Diversity,
	kPval_PorpCompat,
	kPval_IndexAssoc,
	kPval_RBarD,
	kPval_RBarS,
	kPval_Size
};

const int kRandomProgressStep = 10; // To Do: too big? too small?

const bool kRandomData	= false;
const bool kOriginalData = true;

const char* kSymbol_Unknown 	= "?";
const char* kSymbol_Gap 		= "-";



// *** MAIN BODY *********************************************************/
#pragma mark --

// *** LIFECYCLE *********************************************************/

MultiLocusModel::MultiLocusModel ()
{
	// never trust an uninitialised pointer
	mDiploData = mBackupDiploData = mOriginalDiploData = NULL;
	mHaploData = mBackupHaploData = mOriginalHaploData = NULL;
	// see default states
	mPloidy = kPloidy_None;
	mExcludeLoci = mExcludeIso = false;
	mIsDataRankable = true; 
	mDoMissingShuffle = kMissing_Free;
}

MultiLocusModel::~MultiLocusModel ()
{
//	if (mDiploData)
		delete mDiploData;
//	if (mHaploData)
		delete mHaploData;
//	if (mBackupDiploData)
		delete mBackupDiploData;
//	if (mBackupHaploData)
		delete mBackupHaploData;
		delete mOriginalDiploData;
		delete mOriginalHaploData;
	mDiploData = mBackupDiploData = mOriginalDiploData = NULL;
	mHaploData = mBackupHaploData = mOriginalHaploData = NULL;
}


// *** ACCESS ************************************************************/
#pragma mark --

UInt MultiLocusModel::GetNumRows ()
{
	if (GetPloidy() == kPloidy_Haploid)
		return mHaploData->size();
	else
		return mDiploData->size();
}


UInt MultiLocusModel::GetNumCols ()
{
	if (GetPloidy() == kPloidy_Haploid)
		return (*mHaploData)[0].size();
	else
		return (*mDiploData)[0].size();
}


ploidy_t MultiLocusModel::GetPloidy ()
{
	// Note: cannot assert (mOriginalData != NULL) as this func is called
	// in Backup ().
	
	switch (mPloidy)
	{
		case kPloidy_Haploid:
			assert ((mHaploData != NULL) and (mDiploData == NULL));
			// assert (mOriginalDiploData == NULL);
			// CHANGE: cannot make this check because this is called from
			// BackupOriginal() before it finishes creating the Original
			break;

		case kPloidy_Diploid:
			assert ((mHaploData == NULL) and (mDiploData != NULL));
			// assert (mOriginalDiploData != NULL);
			// CHANGE: as above
			break;
			
		default:
			assert (false);
			break;
	}
	return (mPloidy);
}


const char* MultiLocusModel::GetDataString (UInt iRowIndex, UInt iColIndex)
{
	static string theReturnStr = "";
	
	switch (GetPloidy())
	{
		case kPloidy_Haploid:
			return ((*mHaploData)[iRowIndex][iColIndex]).c_str();
			break;
	 
		case kPloidy_Diploid:
			theReturnStr = (*mDiploData)[iRowIndex][iColIndex].alleleA + '/';
			theReturnStr +=(* mDiploData)[iRowIndex][iColIndex].alleleB;
			return theReturnStr.c_str();
			break;	
			
		default:	 
			assert (false);	// shouldn't ever get here
			break;
	}
	
	assert (false);					// shouldn't get to this point
	return theReturnStr.c_str(); 	// to keep compiler happy
}


// *** LOADING ***********************************************************/
#pragma mark --

// CHANGE: the throws are caught in the application layer now, so there
// is need to clean up here, as the app cleans up by deleting the whole
// model. Also this was creating a potential memeory problem where the 
// matrix was deleted twice.
void MultiLocusModel::
ParseInput (ifstream& ioInputFile, const char* iDataFileName)
{
	// configure scanner
	StreamScanner	theScanner (ioInputFile);
	theScanner.SetComments ("", "");
	theScanner.SetLineComment ("#");

	// get the first line & detect format
	// count seperators to see if it's diploid, count tabs for cols
	string 			theInLine;
	theScanner.ReadLine (theInLine);		
	theScanner.Rewind();	// roll back to beginning
	
	int theNumSeps = count (theInLine.begin(), theInLine.end(), '/');
	
	// CHANGE: we have to detect empty columns, so we split the line
	// based on tabs, and trim empty columns at the end of the line.
	//	int theNumCols = count (theInLine.begin(), theInLine.end(), '\t') + 1;
	vector<string>	theSplitStr;
	std::back_insert_iterator< vector<string> > theSplitIter(theSplitStr);
	split (theInLine, theSplitIter, '\t');
	int theRawNumCols = theSplitStr.size();
	while ((theSplitStr.size() != 0) and (theSplitStr.back() == ""))
		theSplitStr.pop_back();
	int theNumCols = theSplitStr.size();
	if (theRawNumCols != theNumCols)
		cout << "Warning: there are empty columns in the input data." << endl;
		
	// detect format & init correct matrix
	if (theNumSeps == 0)		// haploid
	{
		ParseHaploidInput (theScanner, theNumCols);
	}
	else							// diploid
	{
		if (theNumSeps != theNumCols)
			throw ParseError ("missing column delimiter", iDataFileName);
		ParseDiploidInput (theScanner, theNumCols);
	}
	BackupOriginal ();
	DetermineDimensions ();
	mDataName = iDataFileName;
}


void
MultiLocusModel::ParseHaploidInput (StreamScanner& iScanner, UInt iNumCols)
{
	mPloidy = kPloidy_Haploid;
	mHaploData = new MATRIX(tAllele);
	int theNumRows = 0;
	
	// while the eof has not been reached
	while (iScanner)
	{
		// for each line
		vector<tAllele>	theDataRow;
		tAllele				theCurrAllele;
		string				theInToken;
		char					theInChar;

		// first check that it isn't an empty line
		iScanner.ReadChar(theInChar);
		if (isMemberOf(theInChar, "\r\n"))
			break;
		else
			iScanner.UnreadChar(theInChar);
			
		theNumRows++;
		for (int i = 0; i < int (iNumCols - 1); i++)
		{
			// get allele token
			iScanner.ReadUntil (theInToken, "\t");
			eraseFlankingSpace (theInToken); 
			if (not IsValidAllele (theInToken))
				throw ParseError (iScanner.GetLineIndex (), "illegal allele");
			theCurrAllele = theInToken;
			theDataRow.push_back (theCurrAllele);
			// consume seperator
			iScanner.ReadChar (theInChar);
			if (theInChar != '\t')
			{
				throw ParseError (iScanner.GetLineIndex (),
					"missing column delimiter");
			}
		}
		// for the last allele on the line
		iScanner.ReadLine (theInToken);
		eraseFlankingSpace (theInToken); 
		if (not IsValidAllele (theInToken))
		{
			// do -1 on the line number because you've finished the
			// line you're on
			throw ParseError (iScanner.GetLineIndex () - 1, "illegal allele");
		}
		theCurrAllele = theInToken;
		theDataRow.push_back (theCurrAllele);
	
		mHaploData->push_back (theDataRow);
	}
	
	assert ((int) mHaploData->size() == theNumRows);
}


void
MultiLocusModel::ParseDiploidInput (StreamScanner& iScanner, UInt iNumCols)
{
	mPloidy = kPloidy_Diploid;
	mDiploData = new MATRIX(tAllelePair);
	int theNumRows = 0;
	
	// while the eof has not been reached
	while (iScanner)
	{
		vector<tAllelePair>	theDataRow;
		tAllelePair				theCurrAllele;
		string					theInToken;
		char						theInChar;


		cout << "started reading row" << endl;
		// first check that it isn't an empty line
		iScanner.ReadChar(theInChar);
		if (isMemberOf(theInChar, "\r\n"))
			break;
		else
			iScanner.UnreadChar(theInChar);
			
		// for each line
		theNumRows++;
		theCurrAllele.transNumDTypes = 0;
		
		for (int i = 0; i < int (iNumCols - 1); i++)
		{
			// get first allele
			iScanner.ReadUntil (theInToken, "/");
			eraseFlankingSpace (theInToken); 
			if (not IsValidAllele (theInToken))
				throw ParseError (iScanner.GetLineIndex (), "illegal allele");
			theCurrAllele.alleleA = theInToken;
			// consume separator
			iScanner.ReadChar (theInChar);
			if (theInChar != '/')
				throw ParseError (iScanner.GetLineIndex (), "missing allele seperator");
			// get second allele
			iScanner.ReadUntil (theInToken, "\t");
			eraseFlankingSpace (theInToken); 
			if (IsValidAllele (theInToken) == false)
				throw ParseError (iScanner.GetLineIndex (), "illegal allele");
			theCurrAllele.alleleB = theInToken;
			// store allele pair
			theDataRow.push_back (theCurrAllele);
			// consume dividing character
			iScanner.ReadChar (theInChar);
			if (theInChar != '\t')
				throw ParseError (iScanner.GetLineIndex (), "missing column delimiter");
		}
		// for last column on line
		// get first allele
		iScanner.ReadUntil (theInToken, "/");
		eraseFlankingSpace (theInToken); 
		if (not IsValidAllele (theInToken))
			throw ParseError (iScanner.GetLineIndex (), "illegal allele");
		theCurrAllele.alleleA = theInToken;
		// consume seperator
		iScanner.ReadChar (theInChar);
		if (theInChar != '/')
			throw ParseError (iScanner.GetLineIndex (), "missing allele seperator");
		// get second allele at end of line
		iScanner.ReadLine (theInToken);
		eraseFlankingSpace (theInToken); 
		if (not IsValidAllele (theInToken))
			throw ParseError (iScanner.GetLineIndex () - 1, "illegal allele");
			// do - 1 on the line number above because you've finished the line you're on
		theCurrAllele.alleleB = theInToken;
		// store allele pair
		theDataRow.push_back (theCurrAllele);
	
		mDiploData->push_back (theDataRow);
	}
	
	
	assert ((int) mDiploData->size() == theNumRows);
}


// is it an allowable allele? Also can the data be ranked?
bool MultiLocusModel::IsValidAllele (string& iAlleleStr)
{
	// is the data rankable?
 	if (not IsAlleleRankable (iAlleleStr))
 		mIsDataRankable = false;
 		
	// is the allele unknown?
	if (IsMissing (iAlleleStr))
		return true;
	// must otherwise consist of an alphanumeric string
	for (int i = 0; i < (int) iAlleleStr.size(); i++)
	{
		if (not isalnum (iAlleleStr[i]))
			return false;
	}
	return true;
}


// Essentially asking "is it an integer?"
bool MultiLocusModel::IsAlleleRankable (string& iAlleleStr)
{
	// is the data rankable?
	if (IsMissing (iAlleleStr))
		return true;
	string::iterator p = find_if (iAlleleStr.begin(), iAlleleStr.end(),
		IsNotDigit);
 	if (p != iAlleleStr.end()) // non-digits found
 		return false;
 	else
 		return true;
}


// need to build this function so it can act as functor inside 
// IsValidAllele()
bool IsNotDigit (char& iChar)
{
	return (not isdigit (iChar));
}



// *** MANIPULATIONS *****************************************************/
#pragma mark --

// BACKUP DATASET
// MAkes a copy of the dataset in the backup slot, deletes any that was 
// previously left there
void MultiLocusModel::BackupWorkingData ()
{
	// preconditions: only 1 data slot should have data and at most
	// only one backup slot should have data.
	assert ((mHaploData != NULL) or (mDiploData != NULL));
	assert ((mBackupHaploData == NULL) or (mBackupDiploData == NULL));
	
	switch (GetPloidy())
	{
		case kPloidy_Haploid:
			if (mBackupHaploData != NULL)
				delete mBackupHaploData;
			mBackupHaploData = new MATRIX(tAllele);
			*mBackupHaploData = *mHaploData;
			break;
			
		case kPloidy_Diploid:
			if (mBackupDiploData != NULL)
				delete mBackupDiploData;
			mBackupDiploData = new MATRIX(tAllelePair);
			*mBackupDiploData = *mDiploData;
			break;
			
		default:
			// should never reach here
			assert (false);
			break;
	}
}


void MultiLocusModel::RestoreWorkingData ()
{
	// preconditions: only 1 data slot should have data and at most
	// only one backup slot should have data.
	assert ((mHaploData != NULL) or (mDiploData != NULL));
	assert ((mBackupHaploData != NULL) or (mBackupDiploData != NULL));
	
	switch (GetPloidy())
	{
		case kPloidy_Haploid:
			assert (mHaploData != NULL);
			delete mHaploData;
			mHaploData = new MATRIX(tAllele);
			*mHaploData = *mBackupHaploData;
			break;
			
		case kPloidy_Diploid:
			assert (mDiploData != NULL);
			delete mDiploData;
			mDiploData = new MATRIX(tAllelePair);
			*mDiploData = *mBackupDiploData;
			break;
			
		default:
			// should never reach here
			assert (false);
			break;
	}
}


// BACKUP ORIGINAL
// Makes a copy of the dataset in the backup original slot. This should happen
// when the dataset is read in and never at any other time.
void MultiLocusModel::BackupOriginal ()
{
	// preconditions: only one dataset, no backup of the original has occured
	// before.
	assert ((mHaploData == NULL) or (mDiploData == NULL));
	assert ((mHaploData != NULL) or (mDiploData != NULL));
	assert ((mOriginalHaploData == NULL) and (mOriginalDiploData == NULL));
	
	switch (GetPloidy())
	{
		case kPloidy_Haploid:
			assert (mHaploData != NULL);
			mOriginalHaploData = new MATRIX(tAllele);
			*mOriginalHaploData = *mHaploData;
			break;
			
		case kPloidy_Diploid:
			assert (mDiploData != NULL);
			mOriginalDiploData = new MATRIX(tAllelePair);
			*mOriginalDiploData = *mDiploData;
			break;
			
		default:
			// should never reach here
			assert (false);
			break;
	}
}

// RESTORE ORIGINAL
// For when the user decides to "include all" and the original dataset
// must be restored. This is only called in that circumstance. 
void MultiLocusModel::RestoreOriginal ()
{
	// preconditions: only 1 data slot and 1 backup slot should have data
	assert ((mHaploData == NULL) or (mDiploData == NULL));
	assert ((mHaploData != NULL) or (mDiploData != NULL));
	assert ((mOriginalHaploData == NULL) or (mOriginalDiploData == NULL));
	assert ((mOriginalHaploData != NULL) or (mOriginalDiploData != NULL));
	
	switch (GetPloidy())
	{
		case kPloidy_Haploid:
			assert (mHaploData != NULL);
			delete mHaploData;
			mHaploData = new MATRIX(tAllele);
			*mHaploData = *mOriginalHaploData;
			break;
			
		case kPloidy_Diploid:
			assert (mDiploData != NULL);
			delete mDiploData;
			mDiploData = new MATRIX(tAllelePair);
			*mDiploData = *mOriginalDiploData;
			break;
			
		default:
			// should never reach here
			assert (false);
			break;
	}
	
	DetermineDimensions ();
}


// DETERMINE DIMENSIONS
// When a dataset is manipulated by restoring from the original, including
// or excluding data
void MultiLocusModel::DetermineDimensions ()
{
	int theNumCols = GetNumCols ();
	int theNumRows = GetNumRows ();
	
	// reset all the partitioning data	
	mLinkages.SetNumElements (theNumCols);
	mLinkages.SplitAll ();
	mPops.SetNumElements (theNumRows);
	mPops.MergeAll ();
	mNumPairsSites = theNumCols * (theNumCols - 1) / 2;
	mNumPairsIsolates = theNumRows * (theNumRows - 1) / 2;
	
	mIsDataRankable = IsDataRankable ();
}


// Essentially asking "is it an integer?"
bool MultiLocusModel::IsDataRankable ()
{
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		for (int j = 0; j < (int) GetNumCols(); j++)
		{
			if (GetPloidy() == kPloidy_Haploid)
			{
				if (not IsAlleleRankable ((*mHaploData)[i][j]))
					return false;
			}
			else
			{
				if (not IsAlleleRankable ((*mDiploData)[i][j].alleleA))
					return false;
				if (not IsAlleleRankable ((*mDiploData)[i][j].alleleB))
					return false;
			}
		}
	}
	
	// if get this far, it must all be ok
	return true;
}


// *** DATASET TRANSFORMATION ********************************************/
#pragma mark --

void MultiLocusModel::IncludeAllData ()
{
	RestoreOriginal ();
	mExcludeIso = mExcludeLoci = false;
}


// Return false if it is not possible to exclude isolates as
// this would leave an empty dataset.
bool MultiLocusModel::ExcludeMissingIso ()
{
	// check it will not exclude all data
	if (IsRowMissing ())
		return false;
	
	// collect the indexes of "missing" rows
	vector <int>	theDeadRows;	
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		if (IsRowMissing (i))
			theDeadRows.push_back (i);
	}
	
	// delete the marked ones & adjust parameters
	for (int i = (theDeadRows.size() - 1); 0 <= i; i--)
		DeleteRow (theDeadRows[i]);
	DetermineDimensions ();
	mExcludeIso = true;
	
	// return sucess
	return true;
}


// Return false if it is not possible to exclude loci as
// this would leave an empty dataset.
bool MultiLocusModel::ExcludeMissingLoci ()
{
	// check it will not exclude all data
	if (IsColMissing ())
		return false;
	
	// collect the indexes of "missing" columns
	vector <int>	theDeadCols;	
	for (int i = 0; i < (int) GetNumCols(); i++)
	{
		if (IsColMissing (i))
			theDeadCols.push_back (i);
	}
	
	// delete the marked ones & adjust parameters
	for (int i = (theDeadCols.size() - 1); 0 <= i; i--)
		DeleteCol (theDeadCols[i]);
	DetermineDimensions ();
	mExcludeLoci = true;
	
	// return sucess
	return true;
}


void MultiLocusModel::DeleteCol (UInt iColIndex)
{
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		if (GetPloidy() == kPloidy_Haploid)
			(*mHaploData)[i].erase ((*mHaploData)[i].begin() + iColIndex);
		else
			(*mDiploData)[i].erase ((*mDiploData)[i].begin() + iColIndex);
	}
}

void MultiLocusModel::DeleteRow (UInt iRowIndex)
{
	if (GetPloidy() == kPloidy_Haploid)
		mHaploData->erase (mHaploData->begin() + iRowIndex);
	else
		mDiploData->erase (mDiploData->begin() + iRowIndex);
}


// *** SHUFFLING OPS *****************************************************/
#pragma mark --

// SHUFFLE DATASET
// Just like the name suggests. Immediately devolves to shuffling within
// the individual populations. 
void MultiLocusModel::ShuffleDataset ()
{
	for (int i = 0; i < mPops.GetNumParts (); i++)
	{
		int theFromIndex, theToIndex;
		mPops.GetBounds (i, theFromIndex, theToIndex);
		ShufflePop (theFromIndex, theToIndex);
	}
}


// SHUFFLE POPULATION
// Shuffle between the populations given. Note that under the default
// settings (no populations) this will shuffle the whole thing.
void MultiLocusModel::ShufflePop (int iFrom, int iTo)
{
	assert ((0 <= iFrom) and (iFrom < (int) GetNumRows()));
	assert ((0 <= iTo) and (iTo < (int) GetNumRows()));
	assert (iFrom <= iTo);
	
	// if it's a population of size 1, do nothing
	if (iFrom == iTo)
	{
		return;
	}
	else
	{
		// else, shuffle each linkage group
		for (int i = 0; i < mLinkages.GetNumParts (); i++)
		{
			int theFromIndex, theToIndex;
			mLinkages.GetBounds (i, theFromIndex, theToIndex);
			ShuffleBlock (theFromIndex, theToIndex, iFrom, iTo);
		}
	}
}


// SHUFFLE BLOCK
// Shuffle the block of alleles, within the range of isolates given
void MultiLocusModel::ShuffleBlock (int iFromAllele, int iToAllele,
	int iFromIso, int iToIso)
{
	// the old/ normal shuffling proceedure, where everything moves
	// for every pop within range, swap block with another pop in range 
	for (int i = iFromIso; i <= iToIso; i++)
	{
		int theNewPosn = mRng.UniformWhole (iFromIso, iToIso);
		if (theNewPosn != i)
		{
			// if not staying in place, swap every allele in block
			for (int j = iFromAllele; j <= iToAllele; j++)
				SwapAllele (j, i, theNewPosn);
		}
	}
}


// SWAP ALLELE
// Exchange the alleles at iAllelePosn between isolates iFromIso and
// iToIso
void MultiLocusModel::SwapAllele (int iAllelePosn, int iFromIso, int iToIso)
{
	assert ((0 <= iAllelePosn) and (iAllelePosn < (int) GetNumCols()));
	assert ((0 <= iFromIso) and (iFromIso < (int) GetNumRows()));
	assert ((0 <= iToIso) and (iToIso < (int) GetNumRows()));

	// The new shuffling procedure, where missing data is held place. If
	// either allele is missing then return from this function without
	// doing anything. 
	if (mDoMissingShuffle == kMissing_Fixed)
	{
		if (IsMissing (iFromIso, iAllelePosn) or
			IsMissing (iToIso, iAllelePosn))
			return;
	}
		
	// ... but if missing data is not fixed in place or neither allele
	// is missing, just to a normal swap.
	if (GetPloidy() == kPloidy_Diploid)
	{
		tAllelePair	theSwapData = (*mDiploData) [iFromIso][iAllelePosn];
		(*mDiploData) [iFromIso][iAllelePosn] = (*mDiploData) [iToIso][iAllelePosn];
		(*mDiploData) [iToIso][iAllelePosn] = theSwapData;
	}
	else
	{
		assert (GetPloidy() == kPloidy_Haploid);
		
		tAllele	theSwapData = (*mHaploData) [iFromIso][iAllelePosn];
		(*mHaploData) [iFromIso][iAllelePosn] = (*mHaploData) [iToIso][iAllelePosn];
		(*mHaploData) [iToIso][iAllelePosn] = theSwapData;
	}
}


void MultiLocusModel::InitDTypeTranslations ()
{
	// This provides translations of the diplotypes for PAUP output.
	// Note under this scheme the ordering of alleles in pairs _is not_
	// important, i.e. b/a is recognised as the same as a/b.
	// TO DO: break this up so it only calculates dtypes once
	bool	theDipTypeUnique;	
	mDiploTrans.clear();	// array of unique dtypes
	
	for (int i = 0; i < (int) GetNumCols(); i++ )	// foreach loci ...
	{
		for (int k = 0; k < (int) GetNumRows(); k++ )	// foreach isolate ...
		{
			theDipTypeUnique = true;
			
			// compare to the dtypes previous stored in gTransData
			for (int m = 0; m < (int) mDiploTrans.size(); m++)
			{	
				// !! this looks like an error (comparing to an uninited
				// array element), but isn't - you don't traverse the target 
				// array until you have put elements in it.
				ASSERT_VALIDINDEX(k,i);
				
				if (Distance ((*mDiploData)[k][i], mDiploTrans[m]) == 0)
				{
					theDipTypeUnique = false;
					// so all allelepairs of same dtype have same code
					(*mDiploData)[k][i].transNumDTypes = mDiploTrans[m].transNumDTypes;
					break;
				}
			}
			
			// if it is not already there, put it there
			if (theDipTypeUnique == true)
			{
				int theNumDipTypes = mDiploTrans.size ();
				(*mDiploData)[k][i].transNumDTypes = GenerateDTypeSymbol (theNumDipTypes);
				mDiploTrans.push_back ((*mDiploData)[k][i]);
			}
		}
	}
	
	// prepares stepMat, a matrix of the distances between different diplotypes
	int theNumDipTypes = mDiploTrans.size ();
	vector<int>	theDipTypes (theNumDipTypes, 0);
	mStepMatrix.clear ();
	mStepMatrix.resize (theNumDipTypes, theDipTypes);
	
	for (int i = 0; i < theNumDipTypes; i++ )
	{
		for (int j = 0; j < theNumDipTypes; j++ )
		{
			mStepMatrix[i][j] = Distance(mDiploTrans[i], mDiploTrans[j]);
		}
	}
}


// GENERATE DIPLOTYPE SYMBOL
// Given a number from 0 upwards, generates a char that is that dtype symbol
char MultiLocusModel::GenerateDTypeSymbol (UInt iDipTypeIndex)
{
	if (52 <= iDipTypeIndex)
		throw FormatError("More than 52 diplotypes");

	if (iDipTypeIndex < 26)
	{
		return ('A' + iDipTypeIndex);
	}
	else
	{
		assert (iDipTypeIndex < 52);
		return ('a' + iDipTypeIndex);
	}
}


// *** FILE & STREAM WRANGLING *******************************************/
#pragma mark --


void MultiLocusModel::InitFileWithSettings (ofstream& iFileStream)
{
	assert (iFileStream);
	
	PrintSettings (iFileStream);
	iFileStream << endl;
	PrintDataSet (iFileStream);
	iFileStream << endl;
	iFileStream << "---" << endl;
	iFileStream << endl;
	
	iFileStream.setf(ios::showpoint);
}


void MultiLocusModel::InitStatsFile (ofstream& iStatsStream)
{
	assert (iStatsStream);

	iStatsStream << "Diversity Stats from Data:" << endl;
	iStatsStream << "--------------------------" << endl;
	iStatsStream << endl;

	
	InitFileWithSettings (iStatsStream);
	
	iStatsStream << "Replicate\tNumDiff\tMaxFreq\tDiver\tPrCompat\t"
		<< "IndAssoc\trBarD\trBarS" << endl << endl;
}


void MultiLocusModel::InitThetaFile (ofstream& iThetaStream)
{
	assert (iThetaStream);

	iThetaStream << "Calculation Theta bar, Observed Data:" << endl;
	iThetaStream << "-------------------------------------" << endl;
	iThetaStream << endl;

	InitFileWithSettings (iThetaStream);
}


void MultiLocusModel::InitPaupFile (ofstream& iPaupStream)
{
	assert (iPaupStream);

	iPaupStream << "#NEXUS" << endl;
	iPaupStream << "[!Generated by MultiLocus, "
		<< "http://www.bio.ic.ac.uk/evolve/software/multilocus/]" << endl;
	iPaupStream << endl;

	iPaupStream << "[" << endl;
	iPaupStream << "Produced from data:" << endl;
	iPaupStream << "-------------------" << endl;
	iPaupStream << endl;
	PrintSettings (iPaupStream);
	iPaupStream << endl;
	PrintDataSet (iPaupStream);
	iPaupStream << "]" << endl;
	iPaupStream << endl;
	
	iPaupStream << "BEGIN PAUP;" << endl;
	iPaupStream << "\tset maxtrees=1000 increase=no nowarnreset nostatus;"
		<< endl;
	string	theLogFileName (mDataName);
	StringConcat (theLogFileName, ".lengths", 31);
	iPaupStream << "\tlog file=" << theLogFileName
		<< " replace; log stop; log start append; log stop;" << endl;
	iPaupStream << endl;
	
	// do additional preparations for outputting in PAUP format
	// TO DO: break this up so it only calculates dtypes once
	if (GetPloidy() == kPloidy_Diploid)
	{
		InitDTypeTranslations ();
		
		// prints out the translations of the distinct diplotypes?
		iPaupStream << "\t" << "[Diplotype translations]" << endl;
		for (int i = 0; i < (int) mDiploTrans.size(); i++ )
		{
			iPaupStream << "\t[ " << mDiploTrans[i].alleleA
				<< "/" << mDiploTrans[i].alleleB << " --> "
				<< mDiploTrans[i].transNumDTypes << " ]" << endl;
		}
		
		iPaupStream << endl;
	}
}


void MultiLocusModel::InitPairsFile (ofstream& iPairsStream)
{
	assert (iPairsStream);

	iPairsStream << "Pairs from Data:" << endl;
	iPairsStream << "----------------" << endl;
	iPairsStream << endl;

	InitFileWithSettings (iPairsStream);

	// ... print header for the pair file ...
	iPairsStream << "Replicate\t";
	int theNumCols = GetNumCols ();
	for (int i = 0; i < theNumCols - 1; i++)
	{
		for (int j = i + 1; j < theNumCols; j++)
			iPairsStream << i+1 << "&" << j+1 << "\t";
	}
	iPairsStream << endl << endl;
}


void MultiLocusModel::InitPlotFile (ofstream& ioPlotStream)
{
	assert (ioPlotStream);

	ioPlotStream << "Diversity vs. Number of Loci Sampled" << endl;
	ioPlotStream << "------------------------------------" << endl;
	ioPlotStream << endl;

	InitFileWithSettings (ioPlotStream);

	ioPlotStream << "#Loci_Sampled" << "\t"
		<< "Mean_#Genotypes" << "\t" << "Std_Error" << "\t"
		<< "Mean_Diversity"  << "\t" << "Std_Error";

// !!! FOR TESTING, NOT FOR GENERAL RELEASE !!!
#ifdef PAPER_HACK
	ioPlotStream << "\t" << "indAssoc" << "\t" << "Std_Error"
		<< "\t" << "rBarD"  << "\t" << "Std_Error";
#endif
// !!! END TESTING SECTION !!!
		
	ioPlotStream << endl << endl;
}


// *** OUTPUT ************************************************************/
#pragma mark --

// PRINT SETTINGS
// For noting the settings in any file.
void MultiLocusModel::PrintSettings (ostream& oSettingsStream)
{
	// print linkages
	int theNumParts = mLinkages.GetNumParts();
	if (theNumParts == 1)
		oSettingsStream << "There is 1 linkage group: ";
	else
		oSettingsStream << "There are " << theNumParts << " linkage groups: ";
	for (int i = 0; i < theNumParts; i++)
	{
		int theStart, theEnd;
		mLinkages.GetBounds (i, theStart, theEnd);
		if (theStart == theEnd)
			oSettingsStream << theStart + 1 << ' ';
		else
			oSettingsStream << theStart + 1 << '-' << theEnd + 1 << ' ';
	}
	oSettingsStream << endl;
	
	// print populations
	theNumParts = mPops.GetNumParts();
	if (theNumParts == 1)
		oSettingsStream << "There is 1 population: ";
	else
		oSettingsStream << "There are " << theNumParts << " populations: ";
	for (int i = 0; i < theNumParts; i++)
	{
		int theStart, theEnd;
		mPops.GetBounds (i, theStart, theEnd);
		if (theStart == theEnd)
			oSettingsStream << theStart + 1 << ' ';
		else
			oSettingsStream << theStart + 1 << '-' << theEnd + 1 << ' ';
	}
	oSettingsStream << endl;
	
	// print data params
	// Change: (00.1.24) No longer tell user about rankable data, it
	// might confuse them.
	/*
	if (mIsDataRankable)
		oSettingsStream << "Data rankable";
	else
		oSettingsStream << "Data not rankable";
	*/
	if (mExcludeLoci)
		oSettingsStream << "Loci with missing data excluded";
	if (mExcludeIso)
		oSettingsStream << "Isolates with missing data excluded";
	if (not (mExcludeIso or mExcludeLoci))
		oSettingsStream << "All datapoints included";
	oSettingsStream << "." << endl;
}


void MultiLocusModel::PrintDataSet (ostream& ioOutStream)
{
	// first find out how much space we have to allow for each col
	// have a width of 3 minimum
	int theMaxSize = 3;
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		for (int j = 0; j < (int) GetNumCols(); j++)
		{
			string theDataString (GetDataString (i,j));
			if (theMaxSize < (int) theDataString.size())
				theMaxSize = theDataString.size();
		}
	}
	
	// print the header
	ioOutStream << "Iso   Loci: ";
	for (int i = 0; i < (int) GetNumCols(); i++)
	{
		// print leading guff
		ioOutStream << right << setw (theMaxSize + 1) << i+1;		
	}
	ioOutStream << endl << endl;

	// for each row (isolate)
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		// print leading guff
		ioOutStream << right << setw (3) << i+1 << "       : ";
		
		// for each column (site) in row
		for (int j = 0; j < (int) GetNumCols(); j++)
		{
			ioOutStream << setw (theMaxSize + 1) << GetDataString (i,j);
		}	
		ioOutStream << endl;
	}
}


// *** CALCULATIONS ******************************************************/

// *** DIVERSITY CALCULATIONS ********************************************/
#pragma mark --


// PLOT DIVERSITY
// !! To Do: really this should be combined with CalcNumDiff in some way 
// to prevent redundancy of code
// !! To Do: can the Chao method be used to examine sampling effort?
// CHANGE: (00.1.25) Yargh. Because of the missing data problem (see
// CalcNumDiff()) this has to be changed.
// CHANGE: (00.2.10) As well as the mean, need the standard error as
// well which means that data needs to be collected instead of summed.
// CHANGE: (00.8.5) for the purposes of publication, the hacks below are
// included.
void MultiLocusModel::PlotDiv (int iNumSamples, ofstream& ioPlotStream)
{	
	// 1. init output file
	assert (ioPlotStream);

	InitPlotFile(ioPlotStream);
	
	// 2. count genotypes for each size
	int 					theNumIso = GetNumRows();
	int 					theNumLoci = GetNumCols();
	CombinationMill	theComboMill (theNumLoci);
	double				theSqNumIsolates = theNumIso * theNumIso;
	
	// DBG_BLOCK (iNumSamples = 2);
	
	// do size i
	for (int i = 1; i <= theNumLoci; i++)
	{		
		// the number of loci we are actually sampling
		UInt theActualNumSamples = iNumSamples;
		if (i == theNumLoci)
			theActualNumSamples = 1;

		// 3. count genotypes for every sampling
		valarray<double>	theNumGtypesArr(theActualNumSamples);
		valarray<double>	theDivArr(theActualNumSamples);

// !!! FOR TESTING, NOT FOR GENERAL RELEASE !!!
#ifdef PAPER_HACK
		cout << "*** " << i << endl;
		valarray<double>	theIaArr(theActualNumSamples);
		valarray<double>	theRbarDArr(theActualNumSamples);
#endif
// !!! END TESTING SECTION !!!
			
		for (int j = 0; j < (int) theActualNumSamples; j++)
		{
			// 4. which loci are to be sampled?
			Combination theLociSample;
			theComboMill.GetRandomCombination (theLociSample, i);
			theLociSample.Sort();
			
			// 4a. do calculations for Gtypes & diversity
			int			thePairNum = 0;
			vector<int> theIsoDistArray(mNumPairsIsolates,0);
			
			// for every pair of isolates ...
			for (int m = 0; m < theNumIso - 1; m++ )
			{
				for (int n = m + 1; n < theNumIso; n++ )
				{
					assert (thePairNum < (int) mNumPairsIsolates);
					
					// for every site selected, sum the distances involved
					for (int o = 0; o < (int) theLociSample.Size(); o++ )
					{
						int theLociIndex = theLociSample.at(o);
						
						int theDist = (GetPloidy() == kPloidy_Haploid) ?
							Distance ((*mHaploData)[m][theLociIndex], (*mHaploData)[n][theLociIndex]) :
							Distance ((*mDiploData)[m][theLociIndex], (*mDiploData)[n][theLociIndex]);
						
						theIsoDistArray.at(thePairNum) += theDist;					
					}
					thePairNum++;
				}
			}
			
			// now have a vector of isolate distances, theIsoDistArray
			// theTotalNumGTypes +=	CountGtypesFromDist (theIsoDistArray);
			// theTotalDiversity += CalcDivFromDist (theIsoDistArray);
			theNumGtypesArr[j] = CountGtypesFromDist (theIsoDistArray);
			theDivArr[j] = CalcDivFromDist (theIsoDistArray);
			
// !!! FOR TESTING, NOT FOR GENERAL RELEASE !!!
#ifdef PAPER_HACK
			// A. calculate matrix of distances betwen isolates
			vector<int> theHackDistArr (mNumPairsIsolates,0);
			int			theNumSelSites = theLociSample.Size ();
			
			assert (0 <= theNumSelSites);
			thePairNum = 0;

			// Note we were originally using i-j-k counters in this loop, which may
			// have been causing havoc with the outer loop
			// for every pair of isolates ...
			for (int a = 0; a < theNumIso - 1; a++ )
			{
				for (int b = a + 1; b < theNumIso; b++ )
				{
					assert ((0 <= thePairNum) and (thePairNum < mNumPairsIsolates));
					
					// for every site ...
					for (int c = 0; c < theNumSelSites; c++ )
					{
						UInt theSiteIndex = theLociSample[c];
						
						ASSERT_VALIDINDEX(a,theSiteIndex);
						ASSERT_VALIDINDEX(b,theSiteIndex);
						
						int theDist = (GetPloidy() == kPloidy_Haploid) ?
							Distance ((*mHaploData)[a][theSiteIndex], (*mHaploData)[b][theSiteIndex]) :
							Distance ((*mDiploData)[a][theSiteIndex], (*mDiploData)[b][theSiteIndex]);					
						assert (0 <= theDist);
						theHackDistArr.at(thePairNum) += theDist;
					}
					assert (0 <= theHackDistArr.at(thePairNum));
					thePairNum++;
				}
			}
			
			// DBG_VECTOR (&theHackDistArr);
			
			// B. calculate sumOfDist, sumOfSqDist and varDistObs
			// Note we were originally using i counters in this loop, which may
			double theSumDist = 0, theSumSqDist = 0;
			for (int a = 0; a < mNumPairsIsolates; a++)
			{
				UInt theTmpDist = theHackDistArr.at(a);
				theSumDist += theTmpDist;
				theSumSqDist += theTmpDist * theTmpDist;
			}
			
			double theVarDistObs = (theSumSqDist - theSumDist /
				(double) mNumPairsIsolates * theSumDist)
				/ (double) mNumPairsIsolates;


			// C. calculate theSumVarDist
			double theSumVarDist;
			vector<double>	theVarDist (theNumSelSites, 0.0);
			
			if (1 < theNumSelSites)
			{

				// for every loci ...
				for (int a = 0; a < theNumSelSites; a++)
				{
					UInt theSiteIndex = theLociSample.at(a);

					long theSumDist = 0, theSumSquares = 0;
					
					// for every unique pair of isolates
					for (int b = 0; b < (theNumIso - 1); b++)
					{
						for (int c = b + 1; c < theNumIso; c++)
						{
							ASSERT_VALIDINDEX(b,theSiteIndex);
							ASSERT_VALIDINDEX(c,theSiteIndex);
							
							// sum the distances and squares of distances
							int theDistance = (GetPloidy() == kPloidy_Haploid)
								? Distance ((*mHaploData)[b][theSiteIndex], (*mHaploData)[c][theSiteIndex])
								: Distance ((*mDiploData)[b][theSiteIndex], (*mDiploData)[c][theSiteIndex]);
						
							assert (0 <= theDistance);
							theSumDist += theDistance;
							theSumSquares += (theDistance * theDistance);
						}
					}
					
					assert (0 <= theSumDist);
					assert (0 <= theSumSquares);
					
					theVarDist.at(a) = ((double) theSumSquares - ((double)
						(theSumDist * theSumDist) / (double) mNumPairsIsolates)) /
						(double) mNumPairsIsolates;
					// theSumVarDist += theVarDist.at(i);
				}
				theSumVarDist = sum (theVarDist.begin(), theVarDist.end());
			}
			else
				theSumVarDist = 0.0;
			// DBG_VECTOR (&theVarDist);

			// D. calc the maximum sum of covariance = Sum (sqr(var1*var2))
			double 				theMaxSumCov = 0.0;
			// For every unique pair of sites ...
			for (int a = 0; a < (theNumSelSites - 1); a++)
			{
				for (int b = a + 1; b < theNumSelSites; b++)
				{
					theMaxSumCov += sqrt ((double) theVarDist[a] * (double) theVarDist[b]);
				}
			}
			assert (0 <= theMaxSumCov);
			
			// E. calculate & store rBarD and Ia
			if (1 < theNumSelSites)
			{
				theIaArr[j] = (theVarDistObs / theSumVarDist) - 1.0;
				if (0 < theMaxSumCov)
					theRbarDArr[j] = (theVarDistObs - theSumVarDist) / (2 * theMaxSumCov);
				else
					theRbarDArr[j] = 0.0;
			}
			else
				theRbarDArr[j] = theIaArr[j] = 0.0;
			
			// assert ((-1 <= theRbarDArr[j]) and (theRbarDArr[j] <= 1.0));
			if (not ((-1 <= theRbarDArr[j]) and (theRbarDArr[j] <= 1.0)))
			{
				DBG_MSG ("rbar=" << theRbarDArr[j] << "; j=" << j
					<< "; theMaxSumCov=" << theMaxSumCov  << "; theSumVarDist=" << theSumVarDist
					 << "; theVarDistObs=" << theVarDistObs << "; Ia=" << theIaArr[j]
					 << "; theNumSelSites=" << theNumSelSites);
				assert (false);
			}
#endif
// !!! END TESTING SECTION !!!
		}
		
		double	theMeanNumGtypes, theNumGtypesError;
		double	theMeanDiv, theDivError;

		if (theActualNumSamples == 1)
		{
			theMeanNumGtypes = theNumGtypesArr[0];
			theMeanDiv = theDivArr[0];
			theNumGtypesError = 0.0;
			theDivError = 0.0;
		}
		else
		{
			theMeanNumGtypes = StdError (theNumGtypesError, theNumGtypesArr);
			theMeanDiv = StdError (theDivError, theDivArr);
		}

// !!! FOR TESTING, NOT FOR GENERAL RELEASE !!!
#ifdef PAPER_HACK
		double	theMeanIa, theIaError;
		double	theMeanRbarD, theRbarDError;

		if (theActualNumSamples == 1)
		{
			theMeanIa = theIaArr[0];
			theMeanRbarD = theRbarDArr[0];
			theIaError = 0.0;
			theRbarDError = 0.0;
		}
		else
		{
			theMeanIa = StdError (theIaError, theIaArr);
			theMeanRbarD = StdError (theRbarDError, theRbarDArr);
		}
#endif
// !!! END TESTING SECTION !!!

		// print results to output
		// ioPlotStream << i << "\t" 
		// 	<< (double(theTotalNumGTypes) / double(iNumSamples)) << "\t"
		// 	<< (double(theTotalDiversity) / double(iNumSamples)) << endl;
		ioPlotStream << i << "\t" 
			<< theMeanNumGtypes << "\t" << theNumGtypesError << "\t"
			<< theMeanDiv << "\t" << theDivError;

// !!! FOR TESTING, NOT FOR GENERAL RELEASE !!!
#ifdef PAPER_HACK
		ioPlotStream << "\t" << theMeanIa << "\t" << theIaError
			<< "\t" << theMeanRbarD << "\t" << theRbarDError;
#endif
// !!! END TESTING SECTION !!!
			
		ioPlotStream << endl;
	}
}

	
void MultiLocusModel::CalcDiversity
(bool iDoPairwiseStats, int iNumRandomizations, bool iDoPaupOutput,
	ofstream& iStatsStream, ofstream& iPairsStream, ofstream& iPaupStream)
{
	// 1. do necessary preparatory calculations
	CalcVarDistances();
	// Change: (00.1.25) not needed anymore
	// CalcVarSimilarityCoeff();
	if (mIsDataRankable)
		PrepRBarSCalc();
	
	// 2. init vars & output files
	vector<double> thePairPVals;						// for pairwise calcs
	vector<double>	thePairwiseR;						// for pairwise calcs
	vector<UInt> thePVals (kPval_Size, 0);			// for standard stats
	// the saved value for the original data so we can calc pvals
	int		theNumDiffOrig, theMaxFreqOrig;
	double	theDiversityOrig, thePorpCompatOrig, theIndexAssocOrig,
				theRBarDOrig, theRBarSOrig;
	
	InitStatsFile (iStatsStream);
	if (iDoPaupOutput)
		InitPaupFile (iPaupStream);
	if (iDoPairwiseStats)
	{
		InitPairsFile (iPairsStream);
		thePairPVals.resize (mNumPairsSites, 0.0);
		thePairwiseR.resize (mNumPairsSites, 0);		
	}
	
	// 3. and do stats for every randomization	
	for (int i = 0; i < iNumRandomizations + 1; i++)
	{
		// Signal progress of randomizations.
		// To Do: This is a bloody awful nasty hack that breaks the
		// model-app barrier and will give us grief elsewhere. Find a
		// better way to do this.
		if (((i % kRandomProgressStep) == 0) and (i != 0))
			cout << "Doing randomization " << i << " of "
				<< iNumRandomizations << " ..." << endl;
		
		// !! create / load data if necessary
		// !! if randomising data, the first time through loop just save the
		// numbers. Subsequent times, restore and shuffle.
		if (iNumRandomizations)	
		{
			if (i == 0)
				BackupWorkingData ();
			else
				ShuffleDataset ();
		}
		
		// calculate general diversity
		double	theDiversity = 0.0;
		int		theNumDiff = 0;
		int		theMaxFreq = 0;
		CalcNumDiff (theDiversity, theNumDiff, theMaxFreq);
		
		// calculate the porportion incompatible
		double	thePorpCompat;
		CalcPorpCompat (thePorpCompat);
		
		// calc the index of associations and the rbars
		double theIndexAssoc, theRBarD, theRBarS = 0.0;
		CalcIndexAssocRBarD(theIndexAssoc, theRBarD);
		if (mIsDataRankable) 
			CalcRBarS (theRBarS); 

		// Change: in this version we no longer check for theta bar here
		
		// Make calculations for p-values
		// If it is the first time through the loop 
		if (i == 0)
		{
			theNumDiffOrig = theNumDiff;
			theMaxFreqOrig = theMaxFreq;
			theDiversityOrig = theDiversity;
			thePorpCompatOrig = thePorpCompat;
			theIndexAssocOrig = theIndexAssoc;
			theRBarDOrig = theRBarD;
			theRBarSOrig = theRBarS;
		}
		else	// TO DO: really, there has to be a neater way to do this ...
		{
			if (theNumDiff <= theNumDiffOrig)
				thePVals[kPval_NumDiff]++;
			if (theMaxFreq >= theMaxFreqOrig)
				thePVals[kPval_MaxFreq]++;
			if (theDiversity <= theDiversityOrig)
				thePVals[kPval_Diversity]++;
			if (thePorpCompat >= thePorpCompatOrig)
				thePVals[kPval_PorpCompat]++;
			if (theIndexAssoc >= theIndexAssocOrig)
				thePVals[kPval_IndexAssoc]++;
			if (theRBarD >= theRBarDOrig)
				thePVals[kPval_RBarD]++;
			// Change: (00.1.24) ... but it's more complicated than that. First we
			// have to neatly allow for the fact that rBarS may not be calculated.
			// Secondly it's got an odd distribution and we have to show the p
			// value for the result being this extreme in _either_ direction. So -
			if (mIsDataRankable)
			{
				// if we're calculating RBarS
				// To Do: Is this treatment correct is rBarSOrig is 0?
				if (((theRBarSOrig < 0) and (theRBarS <= theRBarSOrig)) or
					((0 <= theRBarSOrig) and (theRBarSOrig <= theRBarS)))
				{
					thePVals[kPval_RBarS]++;
				}
			}
		}

		// print out stats to stats stream
		// XXX: wtf?
		// assert (iStatsStream != NULL);

		if (i == 0)
		{
			iStatsStream << "Observed";
		}
		else
		{
			iStatsStream << i;
		}
				
		iStatsStream << "\t" << theNumDiff << "\t" << theMaxFreq << "\t"
			<< theDiversity << "\t" << thePorpCompat << "\t" << theIndexAssoc
			<< "\t" << theRBarD << "\t";
		if (not mIsDataRankable)
			iStatsStream << "N/A" << endl;
		else
			iStatsStream << theRBarS << endl;
		
		if (iDoPairwiseStats)
		{
			assert (iPairsStream);

			// CHANGE: (00.1.24) Nasty to have to pass a flag that changes the
			// behaviour of the function but I'm a little stuck here. There doesn't
			// seem to be any elegant way of percolating communication to the pairwise
			// calculation from this loop without replicating a bunch of code.
			// The best of a few bad choices.
			// TO DO: Hey! thePairwiseR doesn't seem to be used at all!
			if (i == 0)	
			{
				// if the original data
				iPairsStream << "Observed" << "\t";
				CalcPairwiseStats (iPairsStream, thePairwiseR, thePairPVals, kOriginalData);
				iPairsStream << std::endl;
			}
			else
			{
				// if a randomization
				iPairsStream << i << "\t";
				CalcPairwiseStats (iPairsStream, thePairwiseR, thePairPVals, kRandomData);
			}
		}

		if (iDoPaupOutput)
		{
			iPaupStream << "BEGIN DATA;" << endl;
			iPaupStream << "\tDIMENSIONS ntax=" << (int) GetNumRows()
				<< " nchar=" << (int) GetNumCols() << "; format respectcase missing=? "
				<< "symbols=\"0123456789abcdefghijklmnopqrstuvwxyz"
				<< "ABCDEFGHIJKLMNOPQRSTUVWXYZ\";" << endl;
			iPaupStream << "\tMATRIX" << endl;
			iPaupStream << "\t[!";
			if (i == 0)
				iPaupStream << "Observed";
			else
				iPaupStream << "Replicate " <<  i;
			iPaupStream << "]" << endl;

			OutputAsPaup (iPaupStream);
		}
			
		// restore data to pristine condition if need be
		if (iNumRandomizations and (i != 0))	
			RestoreWorkingData ();
	}
	
	// 4. if there have been randomizations, output p values & tidy up
	
	if (iNumRandomizations)
	{
		assert (iStatsStream);
		iStatsStream << endl << "P_Values";
	
		for (int i = 0; i < kPval_Size; i++)
		{
			iStatsStream << "\t";


			// we have to handle rBarS a little different due to the nature of
			// it's distribution and the fact that it may not be calculated
			// To Do: check this
			if (i == kPval_RBarS)
			{
				if (not mIsDataRankable)
				{
					iStatsStream << "N/A";
				}
				else
				{
					UInt theRsPval = thePVals[kPval_RBarS];
					if (theRsPval == 0)
						iStatsStream << "< " << (2.0 / double(iNumRandomizations));
					else
						iStatsStream << (2 * double(theRsPval) / double(iNumRandomizations));
				}
			}
			else
			{
				if (thePVals[i] == 0)
					iStatsStream << "< " << (1.0 / double(iNumRandomizations));
				else
					iStatsStream << (double(thePVals[i]) / double(iNumRandomizations));
			}
		}

		iStatsStream << endl;
	
		// restore dataset to condition before randomizations
		RestoreWorkingData ();
	}
				

	// !! Output p-values for pairwise stats
	// CHANGE: (00.2.7) Due to the fact that stats cannot be calculated
	// for some columns (see comments for CalcPairwiseStats()), we have
	// to detect pairs containing such columns and print "N/A" for them.
	// This solution is a trifle inelegant.
	if (iDoPairwiseStats)
	{
		assert (iPairsStream);

		iPairsStream << endl << "P Values\t";
		
		int	theSitePrIndex = 0;
		int	theNumCols = GetNumCols();
		
		// count through the pairs
		for (int i = 0; i < theNumCols - 1; i++)
		{
			for (int j = i + 1; j < theNumCols; j++)
			{
				if ((mVarDist[i] == 0) or (mVarDist[j] == 0))
				{
					iPairsStream << "N/A";				
				}
				else
				{
					if (thePairPVals[i] == 0)
					{
						if (iNumRandomizations)
							iPairsStream << "< " << (1.0 / (double) iNumRandomizations);
						else
							iPairsStream << "N/A";
					}
					else
					{
						iPairsStream << "< " << ((double) thePairPVals[i]
							/ (double) iNumRandomizations);
					}
				}
		
				// space to next entry
				iPairsStream << "\t";
				// increment to next pair
				theSitePrIndex++;
			}
		}
		
		// finished printing P-values
		iPairsStream << endl;
	}

}


// CALCULATE ISOLATE DISTANCE ARRAY
// Fill the supplied vector with the distances between the various isolate
// pairs. Distance is by default calculated as relaxed (unknown alleles
// match any known allele), minimizing the distances and number of distinct
// genotypes. The alternative, using strict distances, assumes that unknown
// alleles do not match and thus maximizes distances and number of genotypes.
void MultiLocusModel::
CalcIsoDistArray (vector<int>& oDistArray, distance_t iIsDistStrict)
{
	int			theNumIso = GetNumRows ();
	int			theNumSites = GetNumCols ();
	long			thePairNum = 0;

	oDistArray.clear();
	oDistArray.resize(mNumPairsIsolates,0);
	
	// for every pair of isolates ...
	for (int i = 0; i < theNumIso - 1; i++ )
	{
		for (int j = i + 1; j < theNumIso; j++ )
		{
			// for every site ...
			for (int k = 0; k < theNumSites; k++ )
			{
				int theDist;
				if (iIsDistStrict == kDistance_Relaxed)
				{
					theDist = (GetPloidy() == kPloidy_Haploid) ?
						Distance ((*mHaploData)[i][k], (*mHaploData)[j][k]) :
						Distance ((*mDiploData)[i][k], (*mDiploData)[j][k]);
				}
				else
				{
					assert (iIsDistStrict == kDistance_Strict);
					
					theDist = (GetPloidy() == kPloidy_Haploid) ?
						StrictDistance ((*mHaploData)[i][k], (*mHaploData)[j][k]) :
						StrictDistance ((*mDiploData)[i][k], (*mDiploData)[j][k]);
				}
				
				oDistArray[thePairNum] += theDist;
				assert (thePairNum < mNumPairsIsolates);
			}
			thePairNum++;
		}
	}
}


// COUNT GENOTYPES FROM DISTANCES
// Given an isolate-pair distance array (as generated by CalcIsoDistArray())
// count the number of unique genotypes. Note the answer will vary based on
// how the distances were generated.
UInt MultiLocusModel::
CountGtypesFromDist (vector<int>& oDistArray)
{
	vector<int> theGtypeFreq;

	CountFreqsFromDist (oDistArray, theGtypeFreq);

	// count the uniques
	int theNumUniqueGtypes = 0;
	for (int i = 0; i < (int) theGtypeFreq.size(); i++)
	{
		if (theGtypeFreq[i])
			theNumUniqueGtypes++;
	}
	
	return theNumUniqueGtypes;
}



// COUNT FREQUENCIES FROM DISTANCES
// Given an isolate-pair distance array (as generated by CalcIsoDistArray())
// counts how often genotypes occur. Note the answer will vary based on how
// the distances were generated.
void MultiLocusModel::
CountFreqsFromDist (vector<int>& oDistArray, vector<int>& oGtypeFreq)
{
	int			theNumIso = GetNumRows ();
	long			thePairNum = 0;
	
	oGtypeFreq.clear();
	oGtypeFreq.resize(theNumIso, 1);
	
	// for every pair of isolates ...
	for (int i = 0; i < theNumIso - 1; i++ )
	{
		for (int j = i + 1; j < theNumIso; j++ )
		{
			if ((oGtypeFreq[i] and oGtypeFreq[j])
				and (oDistArray[thePairNum] == 0))
			{
				oGtypeFreq[j]= 0;
				oGtypeFreq[i]++;
			}		

			thePairNum++;
		}
	}
}


// CALC DIVERSITY FROM DISTANCE
// Given an isolate-pair distance array (as generated by CalcIsoDistArray())
// calculates & returns diversity. Note: is robust to missing data.
double MultiLocusModel::CalcDivFromDist (vector<int>& oDistArray)
{
	int theTotalDiff = 0;
	for (int i = 0; i < (int) oDistArray.size(); i++)
	{
		if (0 < oDistArray[i])
			theTotalDiff++;
	}
	
	double theDiversity = double(theTotalDiff) / double(oDistArray.size());
	return theDiversity;
}


// CALC NUM DIFF
// !! ... and gMaxFreq and gDiver. Compare every isolate to those before
// them. The isolates array contains a number for each isolate that says
// how many times it occurs in the total set of isolates, i.e. how many
// copies are there of this? Except that (for duplicates) only the first
// entry is marked thus. All subsequent copies are marked 0.
// !! Calculations verified. gNDiff is the number of distinct genotypes
// (i.e. diff types of isolates), gMaxFreq is the greatest number of times
// any isolate appears in the sample and gDiver is the genetic diversity,
// (1 - SUM(p_i^2))(n/(n-1)) where p_i is the proportion of the ith
// genotype. 
// CHANGE: (00.1.25) There's a problem with the above algorithm. Say
// the first isolate A is placed in the "unique" array has missing data.
// Two other isolates (B and C, at least one of which has missing data)
// are compared to A and regarded as identical (thanks to missing data).
// Yet if these two were compared to each other independently, they need
// not be identical to each other if the missing loci are in different
// positions. Such incomplete isolates can form a chain spanning genotype
// space, each "identical" to their neighbours, yet not to any others.
// Hence the comparision must only use complete isolates and the diversity
// is a lower limit.
// TO DO: further to the above change, the inherent conservatism of
// this approach can be improved if as isolates are gathered into
// "identical" (equivalent) clusters, a prospective member must be 
// identical to all in the cluster. However this means that the membership
// of a cluster (and therefore MaxFreq etc.) can be dependent on the 
// order of joining (as an isolate with missing data could potentially 
// belong to several clusters):
//
//		ABC	- cluster 1
//		ABD	- cluster 2
//		AB?	- 1 or 2?
//
// I suppose we could allow those isolates that _unambiguously_ belong
// to a cluster to stay. Uncertain if this is a significant improvement.
// NumDiff would be still deterministic under this conditions I think.
void MultiLocusModel::CalcNumDiff (double& iDiversity, int& iNumDiff,
	int& iMaxFreq)
{
	vector<int> theIsoDistArray;
	CalcIsoDistArray (theIsoDistArray, kDistance_Relaxed);
	// DBG_VECTOR(&theIsoDistArray);
	
	iNumDiff = CountGtypesFromDist (theIsoDistArray);
	vector<int> theGtypeFreqArray;
	CountFreqsFromDist (theIsoDistArray, theGtypeFreqArray);
	iMaxFreq = 0;
	for (int i = 0; i < (int) theGtypeFreqArray.size(); i++)
	{
		if (iMaxFreq < theGtypeFreqArray[i])
			iMaxFreq = theGtypeFreqArray[i];
	}
	iDiversity = CalcDivFromDist (theIsoDistArray);

	assert (0 < iMaxFreq);
	assert (0 < iDiversity);
}


// *** OTHER CALCULATIONS ************************************************/
#pragma mark --


// CALC PROPORTION COMPATIBLE
// ... calc the proportion of compatiable pairs of loci as per
// Estabrook & Landrum (1975), Taxon v24 n5/6 p609.
// !! Looks good.
void MultiLocusModel::CalcPorpCompat (double& iPorpCompat)
{
	long	theNumIncompat = 0;
	int	theNumSites = GetNumCols ();
	int	theNumIso = GetNumRows ();
	
	// !! for every unique pair of sites - by matching every site (but the 
	// last) with every site after it in the order of isolates.
	for (int i = 0; i < theNumSites - 1; i++)
	{
		for (int j = i + 1; j < theNumSites; j++)
		{
			// ... so for every pair i & j ...
			
			// !! create and init the data structure for holding the list
			// of unique genotypes at (i & j)
			vector< vector<tAllele> >	theGenotypes;
			
			// for each isolate, looking at sites i & j, build a list of
			// unique genotypes
			for (int k = 0; k < theNumIso; k++)
			{
				// !! build a list of the genotypes from that isolate
				// if the characters are known in both sites of the pair
				vector< vector<tAllele> >	theNewGenotypes;
				if (not (IsMissing(k,i) or IsMissing(k,j)))
				{
					if (GetPloidy() == kPloidy_Haploid)
					{
						vector<tAllele>	theSitePair;
						theSitePair.push_back ((*mHaploData)[k][i]);
						theSitePair.push_back ((*mHaploData)[k][j]);
						theNewGenotypes.push_back (theSitePair);
					}
					else
					{
						// !! if diploid, decompose to haplotypes, unless a
						// double heterozygote. Note that if this is a double
						// homozygote, this will generate the same genotype twice
						// but the duplicate will be cut out at the next stage.
						if (IsHomozygous (k,i))
						{
							vector<tAllele>	theSitePair;
							theSitePair.push_back ((*mDiploData)[k][i].alleleA);
							theSitePair.push_back ((*mDiploData)[k][j].alleleA);
							theNewGenotypes.push_back (theSitePair);
							theSitePair[0] = (*mDiploData)[k][i].alleleA;
							theSitePair[1] = (*mDiploData)[k][j].alleleB;
							theNewGenotypes.push_back (theSitePair);
						}
						else if (IsHomozygous (k,j))
						{
							vector<tAllele>	theSitePair;
							theSitePair.push_back ((*mDiploData)[k][i].alleleA);
							theSitePair.push_back ((*mDiploData)[k][j].alleleA);
							theNewGenotypes.push_back (theSitePair);
							theSitePair[0] = (*mDiploData)[k][i].alleleB;
							theSitePair[1] = (*mDiploData)[k][j].alleleA;
							theNewGenotypes.push_back (theSitePair);
						}
					}
				}
				
				// !! iterate through vector of new genotypes and 
				// insert new/unique ones in vector
				while (theNewGenotypes.size() > 0)
				{
					vector<tAllele> theSitePair = theNewGenotypes.back ();
					bool theMatch = false;
					for (int m = 0; m < (int) theGenotypes.size(); m++)
					{
						if ((theSitePair[0] == theGenotypes[m][0])
							and (theSitePair[1] == theGenotypes[m][1]))
						{
							// this one's already there
							theMatch = true;
							break;
						}
					}
					
					// if this isolate doesn't match any seen store it
					if (theMatch == false)
						theGenotypes.push_back (theSitePair);
						
					// delete the pair just checked
					theNewGenotypes.pop_back ();
				}
			}

			// ... so by now we have an vector of unique gtypes (sites i & j wise)
			
			bool theGenotypeIsIncompat = false;
			while ((theGenotypes.size() != 0) and (theGenotypeIsIncompat == false))
			{
				// allocate and initialise lattice array
				vector< vector<tAllele> >	theGraph;
				vector<tAllele> 				theCurrGenotype;

				// remove an entry from genotypes array and put it in lattice
				theGraph.push_back (theGenotypes.back());
				theGenotypes.pop_back();
								
				// for every entry remaining in the genotype array
				for (int k = 0; k < (int) theGenotypes.size(); k++)
				{
					// long theNumDiffs1 = 0, theNumDiffs2 = 0;
					bool isGtype1Unique = true;
					bool isGtype2Unique = true;
					
					// compare it to the entries in the lattice
					for (int m = 0; m < (int) theGraph.size(); m++)
					{
						// theNumDiffs1 += Distance (theGenotypes[k][0], theLattice[m][0]);
						// theNumDiffs2 += Distance (theGenotypes[k][1], theLattice[m][1]);
						if (Distance (theGenotypes[k][0], theGraph[m][0]) == 0)
							isGtype1Unique = false;
						if (Distance (theGenotypes[k][1], theGraph[m][1]) == 0)
							isGtype2Unique = false;
					}
					
					// if both sites were not identical to all the sites in the lattice,
					// i.e. the pair must be unique on at least one axis or ...
					if ((isGtype1Unique == false) and (isGtype2Unique == false))
					{
						theGenotypeIsIncompat = true;
						break;
					}
					else if ((isGtype1Unique and (not isGtype2Unique)) or 
							((not isGtype1Unique) and isGtype2Unique)) 

						// essentially a xor condition
						// (((theNumDiffs1 == theLattice.size()) and 
					   //     (theNumDiffs2 < theLattice.size())) or
						//      ((theNumDiffs1 < theLattice.size()) and 
						//       (theNumDiffs2 == theLattice.size())))
					{
						// !! otherwise iff the allelepair is unique one (and just one)
						// axis, grow the lattice with it (and remove it from genotypes.
						// There shouldn't be a case where both sites match all the others,
						// because you've sorted for unique pairs above. Hence the assert to check.
						assert (not ((not isGtype1Unique) and (not isGtype2Unique)));
						           
						theGraph.push_back (theGenotypes[k]);	
						theGenotypes.erase (theGenotypes.begin() + k);
											
						// !! No, this is not a mistake ... althought it is messy. This is
						// so the for loop starts looking from the beginning again.
						k = -1;	
					}
						
				}
			}
			
			// So by now we have a lattice full of compatiable genotypes (i.e. ones that
			// can be reached on the grid. If any failed to be placed in the lattice, then
			// they are incompatiable and so is the sitepair. Now repeat for all site pairs.
			if (theGenotypeIsIncompat)
				theNumIncompat++;
		}
	}
	
	iPorpCompat = (double) (mNumPairsSites - theNumIncompat) / (double) mNumPairsSites;  
}


// CALC INDEX ASSOCIATION & RBAR_D
// !! cv Maynard Smith et al. (1993) PNAS v90 p4384.
// !! Extemporaneous digression on stats: if M loci (sites) are known in N
// individuals, and Pij is the frequency of the ith allele at the jth site,
// then Hj = (1 - Sum(Pij^2)) is the probability 2 random individuals differ
// at site j. There are NumPrs = N (N - 1) / 2 possible individuals.
// K is the number of sites they differ at (genetic distance).  Expected
// variance of distance is VkExp = Sum (Hj (1 - Hj)). The Index of
// association (the degree to which the observed confounds the expected) is
// Ia = VkObs / VkExp - 1. gExpVar is calculated in CalcVarSimilarityCoeff().
// !! Ia looks good, maybe problems with rBarD.
// !! By contrast rBarD is the variance of distances (VkObs) - the sum of
// the variances of the distances (gSumVarDist) over 2 x the sum of the
// covariances (gMaxSumCov1, both calc in CalcVarDistances).
// !! Note: implemetation differs from that in haploshuffle original.
void MultiLocusModel::CalcIndexAssocRBarD (double& oIndexAssoc, double& oRBarD)
{
	// create and init array for storing distances
	int			theNumIso = GetNumRows ();
	int			theNumSites = GetNumCols ();
	vector<int>	theSumDistArray;
	long			thePairNum = 0;
	
	CalcIsoDistArray (theSumDistArray, kDistance_Relaxed);

	double theSumDist = 0, theSumDistSq = 0;
	for (int i = 0; i < (int) mNumPairsIsolates; i++)
	{
		theSumDist += theSumDistArray[i];
		theSumDistSq += theSumDistArray[i] * theSumDistArray[i];
	}
	
	// !! Calculate the observed variance of distances. theVarDistObs2
	// differs solely in the final statement: (gNumPairsIsolates - 1)
	// vs (gNumPairsIsolates), and is used in rBarD.
	// CHANGE: theVarDistObs2 & theVarDistObs are essentially the same
	// two figures, but with a correction. theVarDistObs2 is the correct
	// one to use for both IndexAssoc & RBarD.
	
	// double theVarDistObs = (theSumDistSq - theSumDist /
	//	(double) mNumPairsIsolates * theSumDist) / (double) (mNumPairsIsolates - 1);
	double theVarDistObs2 = (theSumDistSq - theSumDist /
		(double) mNumPairsIsolates * theSumDist) / (double) mNumPairsIsolates;
		
	// !! Calc index of association by contrasting expected & observed 
	// variance of distances
	oIndexAssoc = (theVarDistObs2 / mSumVarDist) - 1.0;
	oRBarD = (theVarDistObs2 - mSumVarDist) / (2 * mMaxSumCov1);
	
	// DBG_VAL(oIndexAssoc);
	
	// TO DO: are these assertions correct? I think so
	// CHANGE: (00.9.19) Louise's bug, take 2. Think we're getting a rounding
	// error here that causes this assertion to fail incorrectly. Commented out.
	//
	// assert ((-1 <= oRBarD) and (oRBarD <= 1.0));
}


// OUTPUT AS PAUP
// Just as the title says: Called from ShuffleLoop if randomised datasets
// used.
// CHANGE: Fixed for diploid data.
void MultiLocusModel::OutputAsPaup (ofstream& iPaupStream)
{
	assert (iPaupStream);
	
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		iPaupStream << '\t' << "iso" << i + 1 << '\t';
		for (int j = 0; j < (int) GetNumCols(); j++)
		{
			if (GetPloidy() == kPloidy_Haploid)
				iPaupStream << GetDataString (i, j);
			else
				iPaupStream << ((*mDiploData)[i][j]).transNumDTypes << " ";
		}
		iPaupStream << endl;
	}
	iPaupStream << "\t;" << endl;
	iPaupStream << "ENDBLOCK;" << endl << endl;
	
	if (GetPloidy() == kPloidy_Diploid)	
	{
		// must output diplotypes translations
		int theNumDipTypes = mDiploTrans.size();
		
		iPaupStream << "BEGIN ASSUMPTIONS;" << endl;
		iPaupStream << '\t' << "usertype a=" << theNumDipTypes << endl;
		
		iPaupStream << '\t';
		for (int i = 0; i < theNumDipTypes; i++ )
			iPaupStream << mDiploTrans[i].transNumDTypes << " ";
		iPaupStream << endl;
		
		for (int i = 0; i < theNumDipTypes; i++ )
		{
			iPaupStream << '\t';
			for (int j = 0; j < theNumDipTypes; j++ )
				iPaupStream << mStepMatrix[i][j] << " ";
			iPaupStream << endl;
		}
		iPaupStream << "\t;" << endl;
		iPaupStream << "\ttypeset *b=a:all;" << endl << "ENDBLOCK;" << endl
			<< endl;
	}		

	iPaupStream << "BEGIN PAUP;" << endl;
	iPaupStream << '\t' << "hsearch addseq=random nreps=10 swap=none;"
		<< "log start;lenfit;log stop;" << endl;
	iPaupStream << "ENDBLOCK;" << endl << endl;
}


// CALCULATE VARIANCE OF DISTANCES
// !! Calculate the expected variance of distances, mSumVarDist (prev
// called gSumVar1). Works for > 2 alleles, missing data and diploid.
void MultiLocusModel::CalcVarDistances()
{	
	long theSumDist, theSumSquares;
	int theNumSites = GetNumCols ();
	int theNumIso = GetNumRows ();
	
	mVarDist.clear ();
	mVarDist.resize (theNumSites, 0.0);
	mSumVarDist = 0.0;
	
	// for every loci ...
	for (int i = 0; i < theNumSites; i++)
	{
		theSumDist = theSumSquares = 0;
		
		// for every unique pair of isolates
		for (int k = 0; k < (theNumIso - 1); k++)
		{
			for (int m = k + 1; m < theNumIso; m++)
			{ 
				ASSERT_VALIDINDEX(k,i);
				ASSERT_VALIDINDEX(m,i);

				// sum the distances and squares of distances
				int theDistance = (GetPloidy() == kPloidy_Haploid)
					? Distance ((*mHaploData)[k][i], (*mHaploData)[m][i])
					: Distance ((*mDiploData)[k][i], (*mDiploData)[m][i]);
			
				theSumDist += theDistance;
				theSumSquares += (theDistance * theDistance);
			}
		}
		
		mVarDist[i] = ((double) theSumSquares - ((double)
			(theSumDist * theSumDist) / (double) mNumPairsIsolates)) /
			(double) mNumPairsIsolates;
		mSumVarDist += mVarDist[i];
	}
	
	// DBG_VECTOR(&mVarDist);
	
	// Calculate the maximum sum of covariance maxCov1 = Sum (sqr(var1*var2))
	mMaxSumCov1 = 0.0;
	// For every unique pair of sites ...
	for (int i = 0; i < (theNumSites - 1); i++)
	{
		for (int j = i + 1; j < theNumSites; j++)
		{
			mMaxSumCov1 += sqrt ((double) mVarDist[i] * (double) mVarDist[j]);
		}
	}
}


// PREPARE rBarS CALCULATION
// !! Calculating rBarS: assume the alleles are ranked in terms of some
// increasing phenotype (e.g. body size etc.) We wish to ask whether
// alleles for these characters occur together. Prepare for this
// calculation by calculating gSumVar2, gMaxSumCov2.
// !! Looks right. 
void MultiLocusModel::PrepRBarSCalc()
{
	vector<double>		theVarSites (GetNumCols());
	
	mSumVar2 = mMaxSumCov2 = 0.0;

	// for each loci ...
	for (int i = 0; i < (int) GetNumCols(); i++)
	{
		long	theSumDataVals = 0;
		long	theSumSquares = 0;
		
		// for each isolate ...
		for (int k = 0; k < (int) GetNumRows(); k++)
		{
			long theSiteDataValue;
			if (GetPloidy() == kPloidy_Haploid)	// haplo
			{
				theSiteDataValue = String2Int ((*mHaploData)[k][i]);
			}
			else										// diplo
			{
				theSiteDataValue = String2Int ((*mDiploData)[k][i].alleleA)
					+ String2Int ((*mDiploData)[k][i].alleleB);
			}
			assert (theSiteDataValue >= 0);
			
			// add up the data values and their squares
			theSumDataVals += theSiteDataValue;
			theSumSquares += (theSiteDataValue * theSiteDataValue);
		}
		
		theVarSites[i] = ((double) theSumSquares - (double)
			theSumDataVals / (double) GetNumRows() * (double) theSumDataVals)
			/ (double) GetNumRows();
		mSumVar2 += theVarSites[i];
	}
	
	// !! for every pair of sites
	// (i.e. every site matched which every site after them)
	for (int i = 0; i < (int) (GetNumCols() - 1); i++)
	{
		for (int j = i + 1; j < (int) GetNumCols(); j++)
		{
			// calc the covariance as a product of the two sites variance
			mMaxSumCov2 += sqrt (theVarSites[i]*theVarSites[j]);			
		}
	}
}


void MultiLocusModel::CalcRBarS (double& oRBarS)
{
	long	theSumRanks = 0, theSumSqRanks = 0;
	int	theNumIso = GetNumRows();
	
	// for every isolate ...
	for (int i = 0; i < theNumIso; i++)
	{
		long theSumCharRank = 0;
		
		// !! for every site in that isolate, extract the rank of the 
		// character at the site, sum it.
		for (int j = 0; j < (int) GetNumCols(); j++)
		{
			int theCharRank;
			// at length, for edification and debugging
			if (GetPloidy () == kPloidy_Haploid)
			{
				if (IsMissing (i,j))
					theCharRank = 0;
				else
					theCharRank = String2Int ((*mHaploData)[i][j]);
			}
			else
			{
				if (IsMissing ((*mDiploData)[i][j].alleleA))
					theCharRank = 0;
				else
					theCharRank = String2Int ((*mDiploData)[i][j].alleleA);
					
				if (IsMissing ((*mDiploData)[i][j].alleleB))
					theCharRank += 0;
				else
					theCharRank += String2Int ((*mDiploData)[i][j].alleleB);
			}
			
			assert (theCharRank >= 0);
			theSumCharRank += theCharRank;
		}
		
		// sum it and sum the squares ...
		theSumRanks += theSumCharRank;
		theSumSqRanks += theSumCharRank * theSumCharRank;
	}
	
	double theVarSum2 = ((double) theSumSqRanks - (double) theSumRanks /
		(double) theNumIso * (double) theSumRanks) / (double) theNumIso;
	oRBarS = (theVarSum2 - mSumVar2) / (2 * mMaxSumCov2);

	assert ((-1 <= oRBarS) and (oRBarS <= 1.0));
}


/*
// CALCULATE VARIANCE OF SIMILARITY COEFFICIENT
// !! After Maynard Smith et al. Calculate the expected variance of
// similarity coefficients gExpVar = SUM(h(1-h)), where h is heterogeneity.
// Essentially it counts the different numbers of characters at each site.
// !! Verified. gExpVar is the expected variance at a loci
// TO DO: simplify calculation by doing an assign and then *=
// TO DO: hey, this isn't needed anymore! Cool! Check that!
void MultiLocusModel::CalcVarSimilarityCoeff ()
{
	mExpVar = 0.0;
	
	// for every loci do ...
	for (int i = 0; i < (int) GetNumCols(); i++ )
	{
  		// count the different alleles appearing at each loci
  		
  		// make and init array for counting characters
		Frequency 				theCharCount;
  		
  		// look at every isolate and "count" character
  		for (int k = 0; k < (int) GetNumRows(); k++)
  		{
	  		if (GetPloidy() == kPloidy_Haploid)
			{
				theCharCount.Increment ((*mHaploData)[k][i]);
			}
			else
			{
				theCharCount.Increment ((*mDiploData)[k][i].alleleA);
				theCharCount.Increment ((*mDiploData)[k][i].alleleB);
  			}
		}
		
  		// !! Laid out in excruciating detail so I know what's happening.
  		// Just get the number of known (non-?) alleles squared for later
  		// calcs.
  		long theNumUnknownAlleles, theNumKnownAlleles;
		theNumUnknownAlleles = theCharCount.Value (string("?"));		
  		if (GetPloidy() == kPloidy_Haploid)
  			theNumKnownAlleles = GetNumRows() - theNumUnknownAlleles;
  		else
  			theNumKnownAlleles = (2 * GetNumRows()) - theNumUnknownAlleles;
  		
  		long theSqNumKnownAlleles = theNumKnownAlleles * theNumKnownAlleles;
  		assert ((0 <= theNumUnknownAlleles) and (theNumUnknownAlleles <= GetNumRows()));
  		assert ((0 <= theNumUnknownAlleles) and (theNumUnknownAlleles <= GetNumRows()));
  		
  		// !! Calculate heterogenity = 1 - Sum(relative allele freq ^2)
  		double theHet = 1.0;
  		for (int i = 0; i < theCharCount.Size (); i++)
  		{
  			// at length for assurance
  			if (theCharCount.KeyByIndex (i) != "?")
  			{
	  			int theCharacterFreq = theCharCount.ValueByIndex (i);
	  			assert (0 < theCharacterFreq);
	  			assert (theCharacterFreq <= ((GetPloidy() == kPloidy_Haploid)?
	  				GetNumRows() : (2 * GetNumRows())));
  			
  				theHet -= (double) (theCharacterFreq * theCharacterFreq) /
  					(double) theSqNumKnownAlleles;
 				assert (0 < theHet);
  			}
  		}
 		theHet *= (double) GetNumRows() / (double) (GetNumRows() - 1);
 		assert ((0.0 <= theHet)  and (theHet <= 1.0));
  		
 		mExpVar += theHet * (1.0 - theHet);
	}
}
*/


// CALCULATE PAIRWISE STATS
// CHANGE: (00.1.25) An ugly hack is necessary here - in order to detect
// whether to store the value obtained or compare it, we have to pass in 
// a flag saying whether this is the original data or a randomization,
// rather than relying on the contents of oPairwiseRVals being 0.0 to
// signal this.
// TO DO: Perhaps we should return the result and let this decision be
// taken above.
void MultiLocusModel::CalcPairwiseStats (ofstream& oOutStream,
	vector<double>& oPairwiseRVals, vector<double>& oPVals, bool iIsOriginalData)
{	
	// actually do the calculations (for every randomization)
	int	theSitePr = 0;
	int	theNumSites = GetNumCols();
	
	for (int i = 0; i < theNumSites - 1; i++) 
	{		
		for (int j = i + 1; j < theNumSites; j++)
		{
			// CHANGE: (00.2.5) There's a subtle problem here. If one
			// loci is entirely homogenous (i.e has the same allele in all
			// isolates), the mVarDist[x] for that site is 0. Hence
			// theRPairwise for a pair including that site is infinite.
			// No Pval can be calculated for it, so "N/A" is printed.
			if ((mVarDist[i] == 0) or (mVarDist[j] == 0))
			{
				oOutStream << "N/A" << "\t";
			}
			else
			{
				// for every pair of sites
				double theSumDist = 0;
				double theSumSqDist = 0;
				
				for (int k = 1; k < (int) GetNumRows(); k++)
				{
					for (int m = 0; m < k; m++)
					{
						ASSERT_VALIDINDEX(k,i);
						ASSERT_VALIDINDEX(m,i);
						ASSERT_VALIDINDEX(k,j);
						ASSERT_VALIDINDEX(m,j);

						int theDist;
						
						if (GetPloidy() == kPloidy_Haploid)
						{
							theDist = Distance((*mHaploData)[k][i], (*mHaploData)[m][i])
								+ Distance((*mHaploData)[k][j], (*mHaploData)[m][j]);
						}
						else
						{
							assert (GetPloidy() == kPloidy_Diploid);
							theDist = Distance((*mDiploData)[k][i], (*mDiploData)[m][i])
								+ Distance((*mDiploData)[k][j], (*mDiploData)[m][j]);
						}
						
						theSumDist += theDist;
						theSumSqDist += (theDist * theDist);
					}
				} 
				
				double theVar = (theSumSqDist - theSumDist
					/ double(mNumPairsIsolates) * theSumDist)
					/ double(mNumPairsIsolates);
				double theRPairwise = (theVar - (mVarDist[i] + mVarDist[j]))
					/ (2.0 * sqrt(mVarDist[i] * mVarDist[j]));
				
				// CHANGE: an interesting bug/unbug. We had a situation here
				// where theRPairwise was 1.00000(...) but still apparently
				// greater than 1.0. Only (float(theRPairwise) <= 1.0) worked.
				// I hypothesise that rounding left some residue in the nth
				// decimal place and therefore the right think to do is to
				// comment out the test. (00.9.14 - "Louises bug")
				//
				// DBG_BLOCK
				// (
				// 	if (theRPairwise > 1.0)
				// 	{
				// 		DBG_VAL(theSumSqDist);
				// 		DBG_VAL(theSumSqDist);
				// 		DBG_VAL(mNumPairsIsolates);
				// 		DBG_VAL(theRPairwise);
				// 		DBG_VAL(theVar);
				// 		DBG_VAL(i);
				// 		DBG_VAL(j);
				// 		DBG_VAL(mVarDist[i]);
				// 		DBG_VAL(mVarDist[j]);
				// 		DBG_VAL(mVarDist.size());
				// 	}
				// )
				// assert (float(theRPairwise) <= double (1.0));
				
				if (iIsOriginalData == kOriginalData)
				{
					oPairwiseRVals[theSitePr] = theRPairwise;
				}
				else if (theRPairwise >= oPairwiseRVals[theSitePr])
				{
					assert (iIsOriginalData == kRandomData);
					oPVals[theSitePr]++;
				}
				oOutStream << theRPairwise << "\t";
			}
			// go on to next pair
			theSitePr++;
		}
	}
	
	oOutStream << endl;
}


// *** THETA CALCULATIONS ************************************************/
#pragma mark --

// FIND PARTITIONS LOOP
// The high level called by the app that controls the flow of the search
// for partitions. Note that if there there are no randomizations, the
// function breaks out at the halfway point before the loop.
UInt MultiLocusModel::FindPartsLoop
(ofstream& ioPartStream, UInt iNumRandomizations)
{
	// preconditions
	assert (GetPloidy() == kPloidy_Haploid);
	
	// Print header
	ioPartStream << "Testing for Partitions, Observed Data:" << endl;
	ioPartStream << "--------------------------------------" << endl;
	ioPartStream << endl;
	PrintSettings (ioPartStream);
	PrintDataSet (ioPartStream);
	ioPartStream << endl;
	ioPartStream << "----" << endl;
	ioPartStream << endl;
	
	// search the original data set
	int theNumPartsFound = FindParts (ioPartStream, 0);
	
	// if there are no randomizations finish here, return
	// the number of partitions found, and leave function
	if (iNumRandomizations == 0)
		return theNumPartsFound;
		
	// otherwise, if there are randomizations, backup data & go for it!
	ioPartStream << endl;
	ioPartStream << "----" << endl;
	ioPartStream << endl;
	BackupWorkingData ();
	
	for (int i = 1; i <= (int) iNumRandomizations; i++)
	{
		// !! Signal progress of randomizations.
		// TO DO: This is a bloody awful nasty hack that break the
		// model-app barrier and will give us grief elsewhere. Find a
		// better way to do this. Callback?
		// !! Note the progress step is lower for partition searching because
		// this operation is so slow.
		if (((i % (kRandomProgressStep / 4)) == 0) and (i != 0))
			cout << "Doing randomization " << i << " of "
				<< iNumRandomizations << " ..." << endl;

		// shuffle data & do calculations
		ShuffleDataset ();		
		theNumPartsFound += FindParts (ioPartStream, i);		
	}
	
	if (theNumPartsFound == 0)
		ioPartStream << "No partitions found" << endl;
	
	// restore dataset to original condition
	RestoreWorkingData ();
	return theNumPartsFound;
}


// FIND PARTITIONS
// Look for subpops in data. Should only be called for haploids. Nasty &
// computationally expensive.
// CHANGE: (00.1.24) Really there's little interest in the actual
// identity of partitions found in randomized data. Therefore we now only
// print the actual partition in the case of the original dataset. For
// the randomization we just print a summary. 
// TO DO: what to do when generating combinations that will result in
// multiple comparisions of the same partition. (i.e. picking 3 from 6).
// Eliminate the duplicates?
// TO DO: test new printing code
UInt MultiLocusModel::FindParts (ofstream& ioPartStream, UInt iRepNum)
{
	bool					theDatasetPrinted = false;
	int					theNumPartsFound = 0;
	TFrequency<int>	thePartFreq;
	
	// for every possible combination
	ComboMill< vector<UInt> >			theComboGen (GetNumRows());
	theComboGen.First();

	// the range of interesting sizes
	int 					theLowSize = 2;
	int					theHighSize = GetNumRows() / 2;

	while (theComboGen.IsLast() == false)
	{
		// if an interesting size, test it
		int theCurrSize = theComboGen.Size();
		if ((theLowSize <= theCurrSize) and (theCurrSize <= theHighSize))
		{
			// get the current combo
			// Note: this is bloody awkward, but it saves me having to rewrite
			// several associated functions
			vector<UInt>	theMemberArr(GetNumRows()),
								theCompArr(GetNumRows());
			vector<UInt>::iterator
								theComboStopIter,
								theCompStopIter;
			theComboGen.GetCurrent (theMemberArr.begin(), theComboStopIter);
			theComboGen.GetComplement (theCompArr.begin(), theCompStopIter);

			// make data structures that TestPart can accept			
			vector<int>	theShortComboArr (theMemberArr.begin(), theComboStopIter);
			vector<int>	theShortCompArr (theCompArr.begin(), theCompStopIter);
			
			if (theShortCompArr.size() < theShortComboArr.size())
				theShortCompArr.swap (theShortComboArr);
				
			// test it, if a hit, print it
			if (TestPart (theShortComboArr, theShortCompArr))
			{
				// print the datset once and once only for each replicate
				// that a partition is found in
				
				assert (ioPartStream);
				
				theNumPartsFound++;

				// if a partition is in the original dataset, print the
				// partitions.
				if (iRepNum == 0)
				{
					OutputPart (ioPartStream, theShortComboArr);
					ioPartStream << endl;
				}
				// if this is a randomization print a summary
				else
				{
					// if the header to this randomization has not been
					// printed already
					if (theDatasetPrinted == false)
					{
						ioPartStream << "----" << endl;
						ioPartStream << endl;
						ioPartStream << "*** Replicate " << iRepNum << endl;
						PrintDataSet (ioPartStream);
						PrintSettings (ioPartStream);
						ioPartStream << endl;
						
						theDatasetPrinted = true;
					}
					
					// print out the smallest "half" of the partition.
					ioPartStream << "* Partition of size " <<
						theShortComboArr.size() << " and " << theShortCompArr.size()
						<< " found." << endl;
				}
									
				int theArrSize = theShortComboArr.size();
				thePartFreq.Increment(theArrSize);
			}
		}
		
		// clock over to next size
		theComboGen.Next();
	}
	
	ioPartStream << endl;
	if (thePartFreq.Size() != 0)
	{
		ioPartStream << "Partition_Size" << "\t" << "Frequency" << endl;
		for (int i = 0; i < thePartFreq.Size(); i++)
		{
			ioPartStream << thePartFreq.KeyByIndex	(i) << "\t"
				<< thePartFreq.ValueByIndex (i) << endl;
		}
		ioPartStream << endl;
	}
	return theNumPartsFound;
}		

template <class forIter_t, typename elem_t>
bool is_member
(forIter_t iStartIter, forIter_t iStopIter, elem_t& iSrchTerm)
{
	if (find (iStartIter, iStopIter, iSrchTerm) == iStopIter)
		return false;
	else
		return true;
}

// TEST PARTITION
// Does this combination actually correspond to a real partition?
bool MultiLocusModel::TestPart
(vector<int>& iPart1, vector<int>& iPart2)
{
	int	theNumSites = GetNumCols();
	
	UNUSED (iPart2);
	
	// test the partition at every site
	for (int i = 0; i < theNumSites; i++)
	{
		Frequency theCharCount1, theCharCount2;
  		
  		// look at every isolate and "count" characters for each partition
  		for (int j = 0; j < (int) GetNumRows(); j++)
  		{
	  		if (is_member (iPart1.begin(), iPart1.end(), j))
			{
				theCharCount1.Increment ((*mHaploData)[j][i]);
			}
			else
			{
				assert (is_member (iPart2.begin(), iPart2.end(), j));
				theCharCount2.Increment ((*mHaploData)[j][i]);
  			}
		}
		
		// get rid of the count of unknown characters
		theCharCount1.Erase (kSymbol_Gap);
		theCharCount2.Erase (kSymbol_Gap);
		theCharCount1.Erase (kSymbol_Unknown);
		theCharCount2.Erase (kSymbol_Unknown);
		
		// see how many alleles the two partitions share. If it's
		// more than one, return false.
		// Step through every allele stored in 1 and see if it is
		// present in 2. If so, increment the number of stored alleles.
		int theNumSharedAlleles = 0;
		for (int j = 0; j < theCharCount1.Size(); j++)
		{
			string	theChar2Key = theCharCount1.KeyByIndex(j);
			if (theCharCount2.Value (theChar2Key) != 0)
				theNumSharedAlleles++;
			if (1 < theNumSharedAlleles)
				return false;
		}
	}
	
	// if we get this far, we must have found a partition.
	return true;		
}



// OUTPUT PARTITION
// Print a partition out nicely to a stream.
// CHANGE: (00.1.25) There seems little reason to print the contents of
// partitions found in randomised data sets, simply the fact that they are
// found is significant. Thus we only need the replicate number and the
// size of the partition found for the randomizations. This code thus now
// only serves to print the partitions found in the main set.
void MultiLocusModel::
OutputPart (ofstream& ioPartStream, vector<int>& iPart)
{
	//for (int i = 0; i < iPart.Size(); i++)
	// print header & sizes
	ioPartStream << "* Partition of size " << iPart.size() << " and "
		<< (int) GetNumRows() - iPart.size() << " found:" << endl;
	//for (int i = 0; i < iPart.Size(); i++)
	//	ioPartStream << " " << iPart[i];
	//ioPartStream << endl;

	// find out how much space we have to allow for each col
	int theMaxSize = 3;
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		for (int j = 0; j < (int) GetNumCols(); j++)
		{
			string theDataString (GetDataString (i,j));
			if (theMaxSize < (int) theDataString.size())
				theMaxSize = theDataString.size();
		}
	}
	
	// print individual isolates	
	for (int i = 0; i < (int) iPart.size(); i++)
	{
		// print leading guff
		ioPartStream << "Isolate " << right << setw (3) << iPart[i] << " : ";
		
		// for each column (site) in row
		for (int j = 0; j < (int) GetNumCols(); j++)
		{
			ioPartStream << setw (theMaxSize + 1) << GetDataString (iPart[i],j);
		}
		ioPartStream << endl;
	}
}


// *** THETA CALCULATIONS ************************************************/
#pragma mark --

// CALC THETA LOOP
// Handles the control flow for theta and bootstraps. Note that there is 
// a clash of semantics here as regards the term "population". In all
// other calculation a population is that unit that is shuffled _within_.
// Here the validity opf population is what is being tested, so we must
// shuffle _ignoring_ the populations. Thus populations are destroyed
// shuffling then restored before testing.
// CHANGE: (00.8.6) so missing elements shuffle with everything else, save
// the MissinShuffle flag, set it to allow movement and restore at the end
// of this process.
// TO DO: would be actually faster to now physically shuffle matrix, but
// just access via indices as individual stay together.
double MultiLocusModel::CalcThetaLoop
(ofstream& ioResults, UInt iNumRandomizations)
{
	// Print header
	InitThetaFile (ioResults);

	// calculate for original set
	double theThetaOrig;
	CalcTheta (theThetaOrig);
	ioResults << "Theta:\t" << theThetaOrig << endl;
		
	// if there are randomizations, backup dataset. Else finish here.
	if (iNumRandomizations == 0)
		return theThetaOrig;
		
	ioResults << endl;
	ioResults << "Randomizations" << endl;
	ioResults << "--------------" << endl;
	ioResults << endl;
	
	// save state so you can restore later
	BackupWorkingData ();
	missing_t theSaveVal = mDoMissingShuffle;
	mDoMissingShuffle = kMissing_Free;
	
	int thePVal = 0;	
		
	for (int i = 1; i <= (int) iNumRandomizations; i++)
	{
		// Signal progress of randomizations.
		// To Do: This is a bloody awful nasty hack that break the
		// model-app barrier and will give us grief elsewhere. Find a
		// better way to do this.
		if (((i % kRandomProgressStep) == 0) and (i != 0))
			cout << "Doing randomization " << i << " of "
				<< iNumRandomizations << " ..." << endl;

		// remove (but save) population boundaries
		Partition thePopBoundaries = mPops;
		mPops.MergeAll ();
		Partition theLinkBoundaries = mLinkages;
		mLinkages.MergeAll ();
		
		// shuffle
		ShuffleDataset ();
		
		// restore population boundaries
		mPops = thePopBoundaries;
		mLinkages = theLinkBoundaries;

		// do calculations
		double theThetaRand;
		CalcTheta (theThetaRand);
		
		// print out result
		ioResults << "Randomization #" << i << ":\t" << theThetaRand << endl;
	
		// do P value calculation
		if (theThetaOrig <= theThetaRand)
			thePVal++;
			
	}

	// restore priot state
	RestoreWorkingData ();
	mDoMissingShuffle = theSaveVal;
	
	// calculate & print overall P value.
	ioResults << endl;
	ioResults << "P value:\t";
	if (thePVal == 0)
		ioResults << "< " << (1.0 / (double) iNumRandomizations);
	else
		ioResults << double ((double) thePVal / (double) iNumRandomizations);
	ioResults << endl;
	
	// return original result
	return theThetaOrig;
}


// CALC THETA
// !! A measure of population differentiation, a la Weir (1996) "Genetic
// Data Analysis II" p170. Note populations of size 1 are allowed, unknown
// characters are deleted and for diploids both alleles are counted.
// Called by CalcThetaLoop().
// !! Note: populations of size 1 should be allowed, although they are not
// necessarily informative.
void MultiLocusModel::CalcTheta (double& oTheta)
{
	int   	theNumPops = mPops.GetNumParts ();
	int		theNumSites = GetNumCols ();
	int		theNumSitesSampled = 0;
	double	theSum_Q2 = 0.0, theSum_Q3 = 0.0;

	// for every site ...
	for (int i = 0; i < theNumSites; i++)
	{
	
		// create & init array holding allele frequency
		vector <Frequency>	theAlleleFreqs (theNumPops);
		
		// go through the isolates population by population
		for (int j = 0; j < theNumPops; j++)
		{
			// !! for the isolates in population j count the frequency
			// of allele characters within that population
			int			theStart, theEnd;
			mPops.GetBounds (j, theStart, theEnd);
			
			for (int k = theStart; k <= theEnd; k++)
			{
		  		if (GetPloidy() == kPloidy_Haploid)
		  		{
		  			theAlleleFreqs[j].Increment ((*mHaploData)[k][i]);
				}
				else
				{
		  			theAlleleFreqs[j].Increment ((*mDiploData)[k][i].alleleA);
		  			theAlleleFreqs[j].Increment ((*mDiploData)[k][i].alleleB);
				}
			}
		}
		
		// so at this point we have an array of the allele frequencies
		// for every population for a particular site (i)
		
		// Calculate the sum of the freqs and the sum of the square of
		// the freqs for each population. Ignore any unknown alleles.
		// Also get the total freqs for each allele.
		vector<long>	theSumArray (theNumPops, 0);
		vector<long>	theSumSqArray (theNumPops, 0);
		Frequency		theTotalFreqs;
		
		for (int j = 0; j < theNumPops; j++)
		{
			theAlleleFreqs[j].Erase(kSymbol_Unknown);
			theAlleleFreqs[j].Erase(kSymbol_Gap);
			theTotalFreqs.Add (theAlleleFreqs[j]);
			
			for (int k = 0; k < theAlleleFreqs[j].Size(); k++)
			{
				int theCharFreq = theAlleleFreqs[j].ValueByIndex (k);
				theSumArray[j] += theCharFreq;
				theSumSqArray[j] += (theCharFreq * theCharFreq);
			}
		}
		
		// How many alleles (samples) have we seen? How many different
		// alleles have we seen?
		int 	theNumSamples = theTotalFreqs.Value ();	// also sum of freqs					
		int	theNumAlleles = theTotalFreqs.Size();		// diff. alleles
		
		// Need to have seen at least 2 diff alleles to make extract
		// sensible data from a site.
		if (2 <= theNumAlleles)
		{
			theNumSitesSampled++; // how many sites have 2+ alleles
			double	theSumFreq, theSumSqFreq;
			double	the_NBar, the_Nc, the_X, the_Y, the_Q2, the_Q3;

			// Caclulate the sum of frequencies and the sum of squared freqs
			theSumFreq = theSumSqFreq = 0.0;
			for  (int k = 0; k < theNumPops; k++)
			{
				theSumFreq += theSumArray[k];
				theSumSqFreq += (theSumArray[k] * theSumArray[k]);
				//the_X += (theSumArray[k] / theSumSqArray[k]);
			}
			
			// Y, is the sum of the squares of allele freqs seen. CHECKED.
			// TWICE. And reformulated just to be sure.
			the_Y = 0.0;
			for (int k = 0; k < theTotalFreqs.Size(); k++)	// for each allele
			{
				int theAlleleSum = 0;
				string theKey = theTotalFreqs.KeyByIndex (k);
				for (int m = 0; m < theNumPops; m++) // for every pop
					theAlleleSum += theAlleleFreqs[m].Value(theKey);
				assert (0 < theAlleleSum);
				assert (theAlleleSum == theTotalFreqs.ValueByIndex(k));
				the_Y += theAlleleSum * theAlleleSum;
			}
			assert (0 < the_Y);

			// X. CHECKED.
			the_X = 0.0;
			for (int k = 0; k < theTotalFreqs.Size(); k++)	// for each allele
			{
				double theAlleleSum = 0;
				string theKey = theTotalFreqs.KeyByIndex (k);
				for (int m = 0; m < theNumPops; m++) // for every pop
				{
					double thePopVal = theAlleleFreqs[m].Value(theKey);
					assert (0 <= thePopVal);
					if (thePopVal != 0)
						theAlleleSum += double((thePopVal * thePopVal))
							/ double (theAlleleFreqs[m].Value());
				}
				assert (0 < theAlleleSum);
				the_X += theAlleleSum;
			}
			assert (0 < the_X);

			// NBar, mean individuals sampled per population. CHECKED.
			assert (theNumSamples == theSumFreq);
			the_NBar = double(theNumSamples) / double(theNumPops);
			
			// Nc, looks good. CHECKED.
			the_Nc = (1.0 / (double(theNumPops) - 1.0)) * 
				(theSumFreq - (theSumSqFreq / theSumFreq));
			assert (theSumFreq <= theSumSqFreq);
			
			// Q_2. CHECKED.
			the_Q2 = (the_X - theNumPops) / (theNumPops * (the_NBar - 1.0));
			
			// Q_3. Done in two parts for clarity. CHECKED.
			the_Q3 = (1.0 / (theNumPops * (theNumPops - 1.0) * the_NBar
				* the_Nc)) * (the_Y - ((the_NBar * (the_Nc - 1.0)
				/ (the_NBar - 1.0)) * the_X));
			the_Q3 += ((the_NBar - the_Nc) / (the_Nc * (the_NBar - 1.0))) *
				(1.0 - (the_X / (theNumPops - 1.0)));
				
			// add to global count. CHECKED.
			theSum_Q2 += the_Q2;
			theSum_Q3 += the_Q3;
		}
	}
	
	// need to have been able to sample at least one site
	if (0 < theNumSitesSampled)
	{
		// Result calculation. CHECKED.
		oTheta = (theSum_Q2 - theSum_Q3) / (theNumSitesSampled - theSum_Q3);
	}
	else
	{
		throw Error("Need to be able to sample at least 1 polymorphic locus");
	}	
}


// *** CALC THETA WITH SUBSET OF POPS
#pragma mark --

double MultiLocusModel::CalcThetaChoiceLoop
(ofstream& ioResults, Combination& iSelectedPops, UInt iNumRandomizations)
{
	// Print header
	InitThetaFile (ioResults);
	iSelectedPops.Sort();
	ioResults << "Populations selected for analysis:";
	for (int i = 0; i < (int) iSelectedPops.Size(); i++)
		ioResults << " " << iSelectedPops[i] + 1;
	ioResults << endl << endl;
	ioResults << "---" << endl;
	ioResults << endl;

	// calculate for original set
	double theThetaOrig;
	CalcThetaChoice (theThetaOrig, iSelectedPops);
	ioResults << "Theta:\t" << theThetaOrig << endl;
		
	// if there are randomizations, backup dataset. Else finish here.
	if (iNumRandomizations == 0)
		return theThetaOrig;
		
	ioResults << endl;
	ioResults << "Randomizations" << endl;
	ioResults << "--------------" << endl;
	ioResults << endl;
	BackupWorkingData ();
	int thePVal = 0;	
		
	for (int i = 1; i <= (int) iNumRandomizations; i++)
	{
		// Signal progress of randomizations.
		// To Do: This is a bloody awful nasty hack that break the
		// model-app barrier and will give us grief elsewhere. Find a
		// better way to do this.
		if (((i % kRandomProgressStep) == 0) and (i != 0))
			cout << "Doing randomization " << i << " of "
				<< iNumRandomizations << " ..." << endl;

		// remove (but save) population boundaries
		//Partition thePopBoundaries = mPops;
		//mPops.MergeAll ();
		Partition theLinkBoundaries = mLinkages;
		mLinkages.MergeAll ();
		
		// shuffle
		ShufflePops (iSelectedPops);
		
		// restore population boundaries
		//mPops = thePopBoundaries;
		mLinkages = theLinkBoundaries;
		
		// do calculations
		double theThetaRand;
		CalcThetaChoice (theThetaRand, iSelectedPops);
		
		// print out result
		ioResults << "Randomization #" << i << ":\t" << theThetaRand << endl;
	
		// do P value calculation
		if (theThetaOrig <= theThetaRand)
			thePVal++;
			
	}

	// restore dataset
	RestoreWorkingData ();
	
	// calculate & print overall P value.
	ioResults << endl;
	ioResults << "P value:\t";
	if (thePVal == 0)
		ioResults << "< " << (1.0 / (double) iNumRandomizations);
	else
		ioResults << double ((double) thePVal / (double) iNumRandomizations);
	ioResults << endl;
	
	// return original result
	return theThetaOrig;
}


// SHUFFLE POPS
// Shuffle the datset within only the populations give. This is a  wee bit 
// ugly but necessary as the meaning of "population" within the context of 
// the Theta calculation is slightly different to that outside. Note
// linkage groups are preserved.
void MultiLocusModel::ShufflePops (Combination& iSelectedPops)
{
	// collect vector of population indexes
	vector<int> theSelectedIsos;
	// step through the vector of populations and collate them
	//DBG_MSG("Shuffling pops");
	for (int i = 0; i < (int) iSelectedPops.Size (); i++)
	{
		//DBG_MSG("Selecting population " << i);
		int theFromIndex, theToIndex;
		mPops.GetBounds (i, theFromIndex, theToIndex);
		//DBG_MSG("Bounds are " << theFromIndex << " to " << theToIndex);
		for (int j = theFromIndex; j <= theToIndex; j++)
		{
			//DBG_VAL(j);
			theSelectedIsos.push_back(j);
		}
	}
	
	// for each linkage group, shuffle the isolates within
	for (int i = 0; i < mLinkages.GetNumParts (); i++)
	{
		int theFromLoci, theToLoci;
		mLinkages.GetBounds (i, theFromLoci, theToLoci);

		// for every isolate, swap with another isolate
		for (int j = 0; j < (int) theSelectedIsos.size(); j++)
		{
			// select a position in the isolate list
			int theOldPosn = theSelectedIsos[j];
			int theNewPosn = mRng.UniformWhole (theSelectedIsos.size());
			// if not itself
			if (theNewPosn != theOldPosn)
			{
				// lets do the swap! exchange every allele!
				for (int k = theFromLoci; k <= theToLoci; k++)
					SwapAllele (k, theNewPosn, theOldPosn);
			}
		
		}	
	}	
}


// CALC THETA CHOICE
// as above with the added option of searching only selected populations
void MultiLocusModel::CalcThetaChoice
(double& oTheta, Combination& iSelectedPops)
{
	iSelectedPops.Sort();
	int		theNumSelectedPops = iSelectedPops.Size ();
	int		theNumTotalPops = mPops.GetNumParts ();
	int		theNumSites = GetNumCols ();
	int		theNumSitesSampled = 0;
	double	theSum_Q2 = 0.0, theSum_Q3 = 0.0;

	// for every site ...
	for (int i = 0; i < theNumSites; i++)
	{
	
		// create & init array holding allele frequency
		vector <Frequency>	theAlleleFreqs (theNumSelectedPops);
		
		// go through the isolates population by population
		for (int j = 0; j < theNumSelectedPops; j++)
		{
			// !! for the isolates in population iSelectedPops[j] count the
			// frequency of allele characters within that population
			int			theStart, theEnd;
			mPops.GetBounds (iSelectedPops[j], theStart, theEnd);
			
			for (int k = theStart; k <= theEnd; k++)
			{
		  		if (GetPloidy() == kPloidy_Haploid)
		  		{
		  			theAlleleFreqs[j].Increment ((*mHaploData)[k][i]);
				}
				else
				{
		  			theAlleleFreqs[j].Increment ((*mDiploData)[k][i].alleleA);
		  			theAlleleFreqs[j].Increment ((*mDiploData)[k][i].alleleB);
				}
			}
		}
		
		// so at this point we have an array of the allele frequencies
		// for every population for a particular site (i)
		
		// Calculate the sum of the freqs and the sum of the square of
		// the freqs for each population. Ignore any unknown alleles.
		// Also get the total freqs for each allele.
		vector<long>	theSumArray (theNumSelectedPops, 0);
		vector<long>	theSumSqArray (theNumSelectedPops, 0);
		Frequency		theTotalFreqs;
		
		for (int j = 0; j < theNumSelectedPops; j++)
		{
			theAlleleFreqs[j].Erase(kSymbol_Unknown);
			theAlleleFreqs[j].Erase(kSymbol_Gap);
			theTotalFreqs.Add (theAlleleFreqs[j]);
			
			for (int k = 0; k < theAlleleFreqs[j].Size(); k++)
			{
				int theCharFreq = theAlleleFreqs[j].ValueByIndex (k);
				theSumArray[j] += theCharFreq;
				theSumSqArray[j] += (theCharFreq * theCharFreq);
			}
		}
		
		// How many alleles (samples) have we seen? How many different
		// alleles have we seen?
		int 	theNumSamples = theTotalFreqs.Value ();	// also sum of freqs					
		int	theNumAlleles = theTotalFreqs.Size();		// diff. alleles
		
		// Need to have seen at least 2 diff alleles to make extract
		// sensible data from a site.
		if (2 <= theNumAlleles)
		{
			theNumSitesSampled++; // how many sites have 2+ alleles
			double	theSumFreq, theSumSqFreq;
			double	the_NBar, the_Nc, the_X, the_Y, the_Q2, the_Q3;

			// Caclulate the sum of frequencies and the sum of squared freqs
			theSumFreq = theSumSqFreq = 0.0;
			for  (int k = 0; k < theNumSelectedPops; k++)
			{
				theSumFreq += theSumArray[k];
				theSumSqFreq += (theSumArray[k] * theSumArray[k]);
				//the_X += (theSumArray[k] / theSumSqArray[k]);
			}
			
			// Y, is the sum of the squares of allele freqs seen. CHECKED.
			// TWICE. And reformulated just to be sure.
			the_Y = 0.0;
			for (int k = 0; k < theTotalFreqs.Size(); k++)	// for each allele
			{
				int theAlleleSum = 0;
				string theKey = theTotalFreqs.KeyByIndex (k);
				for (int m = 0; m < theNumSelectedPops; m++) // for every pop
					theAlleleSum += theAlleleFreqs[m].Value(theKey);
				assert (0 < theAlleleSum);
				assert (theAlleleSum == theTotalFreqs.ValueByIndex(k));
				the_Y += theAlleleSum * theAlleleSum;
			}
			assert (0 < the_Y);

			// X. CHECKED.
			the_X = 0.0;
			for (int k = 0; k < theTotalFreqs.Size(); k++)	// for each allele
			{
				double theAlleleSum = 0;
				string theKey = theTotalFreqs.KeyByIndex (k);
				for (int m = 0; m < theNumSelectedPops; m++) // for every pop
				{
					double thePopVal = theAlleleFreqs[m].Value(theKey);
					assert (0 <= thePopVal);
					if (thePopVal != 0)
						theAlleleSum += double((thePopVal * thePopVal))
							/ double (theAlleleFreqs[m].Value());
				}
				assert (0 < theAlleleSum);
				the_X += theAlleleSum;
			}
			assert (0 < the_X);

			// NBar, mean individuals sampled per population. CHECKED.
			assert (theNumSamples == theSumFreq);
			the_NBar = double(theNumSamples) / double(theNumSelectedPops);
			
			// Nc, looks good. CHECKED.
			the_Nc = (1.0 / (double(theNumSelectedPops) - 1.0)) * 
				(theSumFreq - (theSumSqFreq / theSumFreq));
			assert (theSumFreq <= theSumSqFreq);
			
			// Q_2. CHECKED.
			the_Q2 = (the_X - theNumSelectedPops) / (theNumSelectedPops
				* (the_NBar - 1.0));
			
			// Q_3. Done in two parts for clarity. CHECKED.
			the_Q3 = (1.0 / (theNumSelectedPops * (theNumSelectedPops - 1.0)
				* the_NBar * the_Nc)) * (the_Y - ((the_NBar * (the_Nc - 1.0)
				/ (the_NBar - 1.0)) * the_X));
			the_Q3 += ((the_NBar - the_Nc) / (the_Nc * (the_NBar - 1.0))) *
				(1.0 - (the_X / (theNumSelectedPops - 1.0)));
				
			// add to global count. CHECKED.
			theSum_Q2 += the_Q2;
			theSum_Q3 += the_Q3;
		}
	}
	
	// need to have been able to sample at least one site
	if (0 < theNumSitesSampled)
	{
		// Result calculation. CHECKED.
		oTheta = (theSum_Q2 - theSum_Q3) / (theNumSitesSampled - theSum_Q3);
	}
	else
	{
		throw Error("Need to be able to sample at least 1 polymorphic locus");
	}	
}


// *** PRIMITIVES ********************************************************/
#pragma mark --

// DISTANCE
// !! Calculate the minimum distance between two characters. Essentially
// if either is unknown or they are the same, the distance is 0. Else it
// is 1.
// TO DO: should parameters be constant references?

// for haploid
int MultiLocusModel::Distance (tAllele iChar1, tAllele iChar2)
{
	return ((IsMissing(iChar1) or IsMissing(iChar2) or (iChar1 == iChar2)) ?
		0 : 1);
}

// for diploid
// Note the ordering of alleles in pairs is unimportant, i.e. b/a is
// recognised as the same as a/b.
int MultiLocusModel::Distance (tAllelePair iChar1, tAllelePair iChar2)
{
  	if (((IsMissing(iChar1.alleleA) or IsMissing(iChar2.alleleA) or
  		(iChar1.alleleA == iChar2.alleleA)) and (IsMissing(iChar1.alleleB) or
  		IsMissing(iChar2.alleleB) or (iChar1.alleleB == iChar2.alleleB)))
  		or ((IsMissing(iChar1.alleleA) or IsMissing(iChar2.alleleB) or
  		(iChar1.alleleA == iChar2.alleleB)) and (IsMissing(iChar1.alleleB) or
  		IsMissing(iChar2.alleleA) or (iChar1.alleleB == iChar2.alleleA))))
  	{
		return (0);
	}
  	else if ((not IsMissing(iChar1.alleleA)) and (not IsMissing(iChar2.alleleA)) and
  		(not IsMissing(iChar1.alleleB)) and (not IsMissing(iChar2.alleleB)) and
  		(iChar1.alleleA != iChar2.alleleA) and
  		(iChar1.alleleA != iChar2.alleleB)
  		and (iChar1.alleleB != iChar2.alleleA)
  		and (iChar1.alleleB != iChar2.alleleB))
  	{
  		return (2);
  	}
  	else
  	{
  		return (1);
  	}
}


// STRICT DISTANCE
// Works like distance except that alleles must be absolutely equal,
// ambiguity due to mssing characters is assumed to mean the worst.
// Again we have two basic versions, for haploid & diploid, and one
// that tests rows.
// !! Remember, this returns the distance between the two, so it's
// 0 for two equivalent alleles, 1 or more for the alleles they
// _may_ differ at

int MultiLocusModel::StrictDistance (tAllele iChar1, tAllele iChar2)
{
	if (IsMissing(iChar1) or IsMissing(iChar2))
		return 1;
	else if (iChar1 == iChar2)
		return 0;
	else
		return 1;
}

// !! To dissect the logic in this function, assume there are two diploid
// loci with alleles A/B and X/Y ...
int MultiLocusModel::
StrictDistance (tAllelePair iChar1, tAllelePair iChar2)
{
	if (StrictDistance(iChar1.alleleA, iChar2.alleleA) == 0)
	{
		if (StrictDistance(iChar1.alleleB, iChar2.alleleB) == 0)
			return 0;	// if A==X and B==Y
		else
			return 1;	// if A==X and B!=Y
	}
	else if (StrictDistance(iChar1.alleleA, iChar2.alleleB) == 0)
	{
		if (StrictDistance(iChar1.alleleB, iChar2.alleleA) == 0)
			return 0;	// if A==Y and B==X
		else
			return 1;	// if A==Y and B!=X
	}
	else if (StrictDistance(iChar1.alleleB, iChar2.alleleA) == 0)
	{
		return 1;
	}
	else if (StrictDistance(iChar1.alleleB, iChar2.alleleB) == 0)
	{
		return 1;	// if B == X or Y
	}
	else
	{
		return 2;	// if B != X or Y
	}
}

// ... and this compares isolates
UInt MultiLocusModel::StrictDistance (UInt iFromRowIndex, UInt iToRowIndex)
{
	int theSumDistances = 0;
	
	for (int i = 0; i < (int) GetNumCols(); i++)
	{
		// !! For every site, see if the allele is different & sum diffs
		theSumDistances += StrictDistance (iFromRowIndex, iToRowIndex, i);
	}
	
	return theSumDistances;
}

// ... and this compares isolates at a given loci
UInt MultiLocusModel::
StrictDistance (UInt iFromIso, UInt iToIso, UInt iTargetLoci)
{
	if (GetPloidy() == kPloidy_Haploid)
	{
		return StrictDistance ((*mHaploData)[iFromIso][iTargetLoci],
			(*mHaploData)[iToIso][iTargetLoci]);
	}
	else
	{
		return StrictDistance ((*mDiploData)[iFromIso][iTargetLoci],
			(*mDiploData)[iToIso][iTargetLoci]);
	}
}


// IS MISSING
// Is the allele (or allelepair) at this location missing any data?
bool MultiLocusModel::IsMissing (UInt iRowIndex, UInt iColIndex)
{
	if (GetPloidy() == kPloidy_Haploid)
	{
		//if ((*mHaploData)[iRowIndex][iColIndex] == kSymbol_Unknown)
		//	return true;
		if (IsMissing((*mHaploData)[iRowIndex][iColIndex]))
			return true;
	}
	else
	{
//		if ((*mDiploData)[iRowIndex][iColIndex].alleleA == kSymbol_Unknown)
//			return true;
//		if ((*mDiploData)[iRowIndex][iColIndex].alleleB == kSymbol_Unknown)
//			return true;
		if (IsMissing((*mDiploData)[iRowIndex][iColIndex].alleleA))
			return true;
		if (IsMissing((*mDiploData)[iRowIndex][iColIndex].alleleB))
			return true;
	}
	
	return false;
}

bool MultiLocusModel::IsMissing (const char* ikSymbol)
{
	if (string(kSymbol_Unknown) == ikSymbol)
		return true;
	if (string(kSymbol_Gap) == ikSymbol)
		return true;
	
	return false;
}

bool MultiLocusModel::IsMissing (const string& ikSymbol)
{
	return IsMissing (ikSymbol.c_str());
}


bool MultiLocusModel::IsHomozygous (UInt iRowIndex, UInt iColIndex)
{
	assert (GetPloidy() != kPloidy_Haploid);

	if ((*mDiploData)[iRowIndex][iColIndex].alleleA ==
		(*mDiploData)[iRowIndex][iColIndex].alleleB)
		return true;
	else
		return false;
}


// IS COL MISSING
// Does this column contain missing data?
bool MultiLocusModel::IsColMissing (UInt iColIndex)
{
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		if (IsMissing (i,iColIndex))
			return true;
	}
	return false;
}


// Are all cols missing data?
bool MultiLocusModel::IsColMissing ()
{
	for (int i = 0; i < (int) GetNumCols(); i++)
	{
		if (not IsColMissing (i))
			return false;
	}
	return true;
}


// IS ROW MISSING
// Does this row contain missing data?
bool MultiLocusModel::IsRowMissing (UInt iRowIndex)
{
	for (int i = 0; i < (int) GetNumCols(); i++)
	{
		if (IsMissing (iRowIndex, i))
			return true;
	}
	return false;
}


// Are all rows missing data?
bool MultiLocusModel::IsRowMissing ()
{
	for (int i = 0; i < (int) GetNumRows(); i++)
	{
		if (not IsRowMissing (i))
			return false;
	}
	return true;
}


// *** DEPRECIATED & TEST FUNCTIONS **************************************/

// *** END ***************************************************************/

