/**************************************************************************
StringUtils.cp - assorted string utility functions 

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://mesa@agapow.net> <http://www.agapow.net/software/mesa//>

About:
- Uses C (not C++) string manipulation fxns.
- Some functions for C strings (char*), Pascal strings (uchar*) and C++ /
  STL strings.
- There are obviously many variants that could be expanded upon here, e.g.
  strip extension functions for STL strings and so on. These will be done
  when and if they are needed.
- Note the pascal string functions have a distinct prefix "PStr" due to
  the compiler recognising PStrings (unsigned char*) as (char*) all the
  time.
  
Changes:
- 99.9.2: Created.
- 99.10.6: added Stl category
- 00.07.24: tested & verified all but depreciated functions.

To Do:
- StrConcat overloading to take stlString, pascalString?
- are there C++ equivalents?
- should all these fxns have the same names for each, e.g. not PStrConcat,
  StlStrConcat, CStrConcat but all named as StrConcat?
- place breakpoints in depreciated so I know they are being called?
- inline important functions
- expand functionality as above
- enum AddEllipsis constants?
- A PString class?
- The init/ulong/type used for indexing etc. situation is fairly screwed
  up in this module. Best to get type 

**************************************************************************/


// *** INCLUDES
#pragma mark Includes

#include "StringUtils.h"
#include "Error.h"
//#include <cstring>
#include <cstdlib>
#include <cassert>
#include <algorithm>

using std::atoi;
using std::atof;
using std::strlen;
using std::toupper;
using std::tolower;
using std::isspace;
using std::vector;
using std::string;
using std::count;

SBL_NAMESPACE_START


// *** CONSTANTS & DEFINES
#pragma mark Constants

const char kChar_Ellipsis = char (201);


// *** MAIN BODY *********************************************************/
#pragma mark -
bool isMemberOf (char iSearchChar, const string& iTargetStr)
//: Can the search term character be found in the target c-string?
// An overload of the container utility function for the purposes of
// brevity.
{
	return isMemberOf (iSearchChar, iTargetStr.begin(), iTargetStr.end());
}

bool isMemberOf (char iSearchChar, const char* iTargetStr)
//: Can the search term character be found in the target stl-string?
// See isMemberOf (char, string) for further notes.
{
	return isMemberOf (iSearchChar, iTargetStr, iTargetStr +
		std::strlen (iTargetStr));
}

bool isMemberOf (const char* iSearchStr, const string& iTargetStr)
//: Can any character in the search terms be found in the target c-string?
// An overload of the container utility function for the purposes of
// brevity.
{
	return isMemberOf (iSearchStr, iSearchStr + strlen (iSearchStr),
		iTargetStr.begin(), iTargetStr.end());
}

bool isMemberOf (const char* iSearchStr, const char* iTargetStr)
//: Can any character in the search terms be found in the target stl-string?
// See isMemberOf (char*, string) for further notes.
{
	return isMemberOf (iSearchStr, iSearchStr + strlen (iSearchStr),
		iTargetStr, iTargetStr + std::strlen (iTargetStr));
}



// *** TRANSFORMATION FUNCTIONS ******************************************/
#pragma mark -// These will convert, in place, a string in a defined way.
// TO DO: generalised transformation function?

// *** CASE CONVERSION

void toUpper (string& ioTargetString)
//: Convert all characters in the target stl-string to upper case.
// An overload of the templated function for the purposes of brevity.
// Note this conversion happens in place.
{
	toUpper (ioTargetString.begin(), ioTargetString.end());
}

void toUpper (char* ioTargetCString)
//: Convert all characters in the target c-string to upper case.
// See toUpper (string) for further notes.
{
	toUpper (ioTargetCString, ioTargetCString +
		std::strlen (ioTargetCString));
}

void toLower (string& ioTargetString)
//: Convert all characters in the target stl-string to lower case.
// See toUpper (string) for further notes.
{
	toLower (ioTargetString.begin(), ioTargetString.end());
}

void toLower (char* ioTargetCString)
//: Convert all characters in the target c-string to upper case.
// See toUpper (string) for further notes.
{
	toLower (ioTargetCString, ioTargetCString +
		std::strlen (ioTargetCString));
}



// *** STRIP WHITESPACE
// TO DO: Trim Internal Space?

string::size_type eraseLeadingSpace (string& ioTargetStr)
//: delete any whitespace at the string front & return the number removed.
// Whitespace is that defined by isspace(): space, tabs, formfeeds, eoln
// characters.
{
	string::size_type	theInitSize = ioTargetStr.size();
	while (ioTargetStr.size() and isspace(ioTargetStr[0]))
		ioTargetStr.erase(ioTargetStr.begin());
	return (theInitSize - ioTargetStr.size());
}

string::size_type eraseTrailingSpace (string& ioTargetStr)
//: delete any whitespace at the string end & return the number removed.
// See eraseLeadingSpace() for further notes.
{
	string::size_type theInitSize = ioTargetStr.size();
	while (ioTargetStr.size() and isspace(ioTargetStr[ioTargetStr.size() - 1]))
		ioTargetStr.erase(ioTargetStr.end() - 1);
	return (theInitSize - ioTargetStr.size());
}

string::size_type eraseFlankingSpace (string& ioTargetStr)
//: delete whitespace characters at either end & return the number removed.
// See eraseLeadingSpace() for further notes.
{
	string::size_type theLoss = 0;
	theLoss += eraseTrailingSpace (ioTargetStr);
	theLoss += eraseLeadingSpace (ioTargetStr);
	return theLoss;
}


string::size_type eraseInternalSpace (string& ioTargetStr)
//: delete whitespace characters at either end & return the number removed.
// See eraseLeadingSpace() for further notes.
{
	string::size_type theLoss = 0;

	for (string::iterator q = ioTargetStr.begin(); q != ioTargetStr.end();)
	{
		if (isspace (*q))
		{
			ioTargetStr.erase (q);
			theLoss++;
		}
		else
		{
			q++;
		}
	}

	return theLoss;
}

string::size_type eraseAllSpace (string& ioTargetStr)
//: delete all flanking or intrenal whitespace characters
// See eraseLeadingSpace() for further notes.
{
	string::size_type theLoss = 0;
	theLoss += eraseInternalSpace (ioTargetStr);
	theLoss += eraseFlankingSpace (ioTargetStr);
	return theLoss;
}


string::size_type reduceSpace (std::string& ioTargetStr)
//: reduce any runs of whitespace to a single character
{
	string::size_type theLoss = 0;
	bool	thePrevCharWasSpace = false;

	for (string::iterator q = ioTargetStr.begin(); q != ioTargetStr.end();)
	{
		if (isspace (*q))
		{
			if (thePrevCharWasSpace)
			{
				ioTargetStr.erase (q);
				theLoss++;
			}
			else
			{
				thePrevCharWasSpace = true;
				q++;
			}
		}
		else
		{
			thePrevCharWasSpace = false;
			q++;
		}
	}

	return theLoss;
}


// *** CONVERSION FXNS ***************************************************/
#pragma mark -
// !! To Do: these could be improved by pushing the string into a 
// stringstream and hence to an int, thuse allowing numbers like "40,000"
// to be valid numbers where <locale> was invoked. See example below.
// !! To Do: what happens if the conversion isn't possible?

double toDouble (const std::string& ikSrcStr)
//: Attempt to convert & return the contents of the string as a double.
{
	// Preconditions:
	std::string theValidSymbols("1234567890.+-");
	if (not isSubsetOf (ikSrcStr.begin(), ikSrcStr.end(),
		theValidSymbols.begin(), theValidSymbols.end()))
	{
		//throw ConversionError (ikSrcStr.c_str(), "double");
		// TO DO
	}

	// Main:
	return std::strtod (ikSrcStr.c_str(), NULL);
}

long toLong (const std::string& ikSrcStr)
//: Attempt to convert & return the contents of the string as a double.
{
	return std::strtol (ikSrcStr.c_str(), NULL, 0);
}


bool isReal (const std::string& ikTargetStr)
//: is this string a floating point number?
{
	std::string theValidChars ("+-.1234567890");

	// the first character must be +, -, ., or a digit
	if (not isSubsetOf (ikTargetStr.begin(), ikTargetStr.begin() + 1,
		theValidChars.begin(), theValidChars.end()))
		return false;

	// all subsequent must be , ., or a digit
	if (not isSubsetOf (ikTargetStr.begin() + 1, ikTargetStr.end(),
		theValidChars.begin() + 2, theValidChars.end()))
		return false;

	// 1 and only 1 decimal place
	if (std::count (ikTargetStr.begin(), ikTargetStr.end(), '.') != 1)
		return false;

	// otherwise it's all fine
	return true;
}


bool isWhole (const std::string& ikTargetStr)
//: is this string a whole number?
{
	std::string theValidChars ("+-1234567890");

	// if the targetstring is "0" is okay
	if (ikTargetStr == "0")
		return true;

	// the first character must be +, -, ., or a nonzero digit
	if (not isSubsetOf (ikTargetStr.begin(), ikTargetStr.begin() + 1,
		theValidChars.begin(), theValidChars.end() - 1))
		return false;

	// all subsequent must be digits
	if (not isSubsetOf (ikTargetStr.begin() + 1, ikTargetStr.end(),
		theValidChars.begin() + 2, theValidChars.end()))
		return false;

	// otherwise it's all fine
	return true;
}


// *** DEPRECATED & DEBUG FUNCTIONS *************************************/
#pragma mark -
#if 1
// To run test function, set to 1.
// Note this may generate a "function has no prototype" warning.

	void testStringUtils ()
	//: Simply a test suite for the functions within this module.
	{
		DBG_MSG ("*** Testing string utils ...");

		DBG_MSG ("");
		DBG_MSG ("* Testing membership functions:");

		std::string theStr1 ("abc");
		std::string theStr2 ("c");
		std::string theStr3 ("dfb");
		DBG_MSG ("String 1 is " << theStr1);
		DBG_MSG ("String 2 is " << theStr2);
		DBG_MSG ("String 3 is " << theStr3);

		bool theResult[6];
		theResult[0] = isMemberOf (theStr1.c_str(), theStr2);
		theResult[1] = isMemberOf (theStr1.c_str(), theStr3);
		theResult[2] = isMemberOf (theStr2.c_str(), theStr1);
		theResult[3] = isMemberOf (theStr2.c_str(), theStr3);
		theResult[4] = isMemberOf (theStr3.c_str(), theStr1);
		theResult[5] = isMemberOf (theStr3.c_str(), theStr2);

		DBG_MSG ("The seq membership results are:");
		DBG_MSG ("  is string 1 in string 2 (expect t): " << theResult[0]);
		DBG_MSG ("  is string 1 in string 3 (expect t): " << theResult[1]);
		DBG_MSG ("  is string 2 in string 1 (expect t): " << theResult[2]);
		DBG_MSG ("  is string 2 in string 3 (expect f): " << theResult[3]);
		DBG_MSG ("  is string 3 in string 1 (expect t): " << theResult[4]);
		DBG_MSG ("  is string 3 in string 2 (expect f): " << theResult[5]);

		DBG_MSG ("The single membership results are:");
		DBG_MSG ("  is b in string 1 (expect t): " << isMemberOf ('b',
			theStr1));
		DBG_MSG ("  is b in string 2 (expect f): " << isMemberOf ('b',
			theStr2));
		DBG_MSG ("  is b in string 3 (expect t): " << isMemberOf ('b',
			theStr3));

		DBG_MSG ("Finished testing string utils.");
	}


#endif



// *** PASCAL STRING FXNS ************************************************/
#pragma mark -
// *** CONVERSION

void P2StlStr (const uchar* iSrcPStr, string& ioDestStlStr)
{
	char  theTmpCStr[256];
	P2CStr (iSrcPStr, theTmpCStr);
	ioDestStlStr = theTmpCStr;
}

void P2CStr (const uchar* iSrcPStr, char* ioDestCStr)
{
	// if the string is longer than 255, clip it to fit
	int theLength = int (iSrcPStr[0]);
	if (255 < theLength)
		theLength = 255;

	for (int i = 0; i < theLength; i++)
	{
		ioDestCStr[i] = char (iSrcPStr[i+1]);
	}
	ioDestCStr[theLength] = '\0';
}

void C2PStr (const char* iSrcCStr, uchar* ioDestPStr)
{
	// if the string is longer than 255, clip it to fit
	ulong  theLength = strlen (iSrcCStr);
	if (255 < theLength)
		theLength = 255;

	ioDestPStr[0] = uchar (theLength);
	for (int i = 0; i < (int) theLength; i++)
	{
		ioDestPStr[i+1] = uchar (iSrcCStr[i]);
	}
}

void Stl2PStr (const string& iSrcStlStr, uchar* ioDestPStr)
{
	// if the string is longer than 255, clip it to fit
	int theLength = int (iSrcStlStr.length());
	if (255 < theLength)
		theLength = 255;

	ioDestPStr[0] = uchar (theLength);
	for (int i = 0; i < theLength; i++)
	{
		ioDestPStr[i+1] = uchar (iSrcStlStr[i]);
	}
}


void PStrCopy (uchar* iSrcPString, uchar* ioDestPString)
{
	std::memcpy (ioDestPString, iSrcPString, ulong (iSrcPString[0] + 1));
}


// *** DEPRECATED & TEST FUNCTIONS **************************************/
#pragma mark -
// *** STRING MEMBER
// Is this character within this string?
bool StrMember (const char* iTargetStr, char iSearchChar)
{
// old version
//	int theTargetLen = std::strlen (iTargetStr);
//	for (int i = 0; i < theTargetLen; i++)
//	{
//		if (iTargetStr[i] == iSearchChar)
//			return true;
//	}
//	return false;
	return isMemberOf (iSearchChar, iTargetStr);
}


// *** TRANSFORMATION FXNS
// Old names for case conversion & space stripping fxns

void MakeUppercase (string& ioTargetString)
{
	toUpper (ioTargetString);
}

void MakeUppercase (char* ioTargetCString)
{
	toUpper (ioTargetCString);
}

void MakeLowercase (string& ioTargetString)
{
	toLower (ioTargetString);
}

void MakeLowercase (char* ioTargetCString)
{
	toLower (ioTargetCString);
}

string::size_type StripLeadingWhitespace (string& ioTargetStr)
{
	string::size_type	theInitSize = ioTargetStr.size();
	while (ioTargetStr.size() and isspace(ioTargetStr[0]))
		ioTargetStr.erase(ioTargetStr.begin());
	return (theInitSize - ioTargetStr.size());
}

string::size_type StripTrailingWhitespace (string& ioTargetStr)
{
	string::size_type	theInitSize = ioTargetStr.size();
	while (ioTargetStr.size() and isspace(ioTargetStr[ioTargetStr.size()]))
		ioTargetStr.erase(ioTargetStr.end() - 1);
	return (theInitSize - ioTargetStr.size());
}

string::size_type StripFlankingWhitespace (string& ioTargetStr)
{
	string::size_type theLoss = 0;
	theLoss += StripTrailingWhitespace (ioTargetStr);
	theLoss += StripLeadingWhitespace (ioTargetStr);
	return theLoss;
}


// *** TOKENIZING FXNS

int Split (string& iSrcString, vector<string>& ioDestVector, char iDelimiter)
{
	string::size_type		theStrLen = iSrcString.length();
	string	theDestString;
	int		theNumTokens = 0;

	for (string::size_type i = 0; i < theStrLen; i++)
	{
		if (iSrcString[i] == iDelimiter)
		{
			ioDestVector.push_back (theDestString);
			theDestString = "";
			theNumTokens++;
		}
		else
		{
			theDestString += iSrcString[i];
		}
	}

	ioDestVector.push_back (theDestString);
	return (theNumTokens + 1);
}

void Merge (vector<string>& iSrcVector, string& oDestString, char const iDelimiter)
{
	if (iSrcVector.size() == 0)
		return;

	vector<string>::iterator p = iSrcVector.begin();
	while (p != (iSrcVector.end() - 1))
	{
		oDestString += *p;
		oDestString += iDelimiter;
		p++;
	}
	oDestString += *p;
}


// *** CONVERSION FXNS

int String2Int (string& iStlString)
{
	// stringstream theTmpStream (iStlString);
	// int theRetInt;
	// theTmpStream >> theRetInt;
	// return theRetInt;
	return atoi (iStlString.c_str());
}

double String2Dbl (string& iStlString)
{
	return atof (iStlString.c_str());
}


// *** PASCAL STRING FXNS

void MakeStlString (uchar* iDestPString, string& oStlString)
{
	// oStlString = ReturnCString (iDestPString);
	oStlString.assign ((char*) iDestPString + 1, iDestPString[0]);
}

uchar* ReturnPString (char* iCString)
{
	static uchar	theBuffer[256];
	string::size_type 		theLength = strlen(iCString);

	// if the string is longer than 255, clip it to fit
	if (255 < theLength)
		theLength = 255;

	theBuffer[0] = uchar (theLength);
	for (string::size_type i = 0; i < theLength; i++)
	{
		theBuffer[i+1] = uchar (iCString[i]);
	}

	return theBuffer;
}

const uchar* ReturnPString (string& iStlString)
{
	static uchar	theBuffer[256];
	string::size_type theLength = iStlString.length();

	// if the string is longer than 255, clip it to fit
	if (255 < theLength)
		theLength = 255;

	theBuffer[0] = uchar (theLength);
	for (string::size_type i = 0; i < theLength; i++)
	{
		theBuffer[i+1] = uchar (iStlString[i]);
	}

	return theBuffer;
}

void MakePString (string& iStlString, uchar* ioDestPString)
{
	string::size_type theLength = iStlString.length();

	// if the string is longer than 255, clip it to fit
	if (255 < theLength)
		theLength = 255;

	ioDestPString[0] = uchar (theLength);
	for (string::size_type i = 0; i < theLength; i++)
	{
		ioDestPString[i+1] = uchar (iStlString.at(i));
	}
}

void MakePString (uchar* iSrcPString, uchar* ioDestPString)
{
	std::memcpy (ioDestPString, iSrcPString, ulong (iSrcPString[0] + 1));
}

uchar* MakePString (string *iStlString)
{
	static uchar	theBuffer[256];
	string::size_type		theLength = iStlString->length();

	// if the string is longer than 255, clip it to fit
	if (255 < theLength)
		theLength = 255;

	theBuffer[0] = uchar (theLength);
	for (string::size_type i = 0; i < theLength; i++)
	{
		theBuffer[i+1] = uchar (iStlString->at(i));
	}

	return theBuffer;
}

uchar* MakePString (char *iCString)
{
	static uchar	theBuffer[256];
	ulong 			theLength = ulong (strlen(iCString));

	// if the string is longer than 255, clip it to fit
	if (255 < theLength)
		theLength = 255;

	theBuffer[0] = uchar (theLength);
	for (ulong i = 0; i < theLength; i++)
	{
		theBuffer[i+1] = uchar (iCString[i]);
	}

	return theBuffer;
}


// *** PREVIOUSLY DEPRECATED
// *** DEPRECATED & TEST FUNCTIONS **************************************/
#pragma mark -
bool isFloat (const std::string& ikTargetStr)
{
	return isReal (ikTargetStr);
}

// STRIP STRING SUFFIX
// Note this strips the suffix including the "."
bool PStrStripSuffix (uchar* iPString)
{
	// step back along string until you find suffix start
	int theLength = iPString[0];
	int i;
	for (i = theLength; (0 < i) and iPString[i] != '.'; i--)
		/* empty loop */ ;

	// if found beginning of suffix
	if (iPString[i] == '.')
	{
		iPString[i] = '\0';
		iPString[0] = uchar (i - 1);

		return true;
	}
	else
	{
		return false;
	}
}


// REPLACE SUFFIX
bool PStrReplaceSuffix (uchar* iDestPString, uchar* iSuffixPString, int iSizeLimit, bool iAddEllipsis)
{
	if (not StripPStrSuffix (iDestPString))
	{
		// if strip failed
		PStrConcat (iDestPString,iSuffixPString,iSizeLimit,iAddEllipsis);
		return false;
	}
	else
	{
		PStrConcat (iDestPString,iSuffixPString,iSizeLimit,iAddEllipsis);
		return true;
	}
}


// PASCAL STRING CONCAT
// the Pascal version
void PStrConcat
(uchar* ioDestStr, uchar* iSuffixStr, int iSizeLimit, bool iAddEllipsis)
{
	string::size_type theDestLen = ioDestStr[0];
	string::size_type theSuffLen = iSuffixStr[0];
	string::size_type theXsLen = theDestLen + theSuffLen - iSizeLimit;

	if (0 < theXsLen)
	{
		// if dest string must be trimmed to fit length
		// shift end token to necessary position
		ioDestStr[theDestLen - theXsLen + 1] = '\0';
		if (iAddEllipsis)
			ioDestStr[theDestLen - theXsLen] = uchar (kChar_Ellipsis);
		ioDestStr[0] -= theXsLen;
		theDestLen = ioDestStr[0];
	}

	// then just join them together & incr length
	std::memcpy (&ioDestStr[theDestLen + 1], &iSuffixStr[1], theSuffLen);
	ioDestStr[0] += theSuffLen;
}


bool StripPStrSuffix	( uchar* ioPString )
{
	return PStrStripSuffix (ioPString);
}


bool ReplacePStrSuffix
( uchar* ioDestPStr, uchar* iSuffixPStr, int iSizeLimit, bool iAddEllipsis)
{
	return PStrReplaceSuffix (ioDestPStr, iSuffixPStr, iSizeLimit, iAddEllipsis);
}


// STRING CONCAT
// The raison d'etre of this function is to allow the safe construction 
// of file names, say by adding a suffix on to the end of the original
// file name. The optional argument sticks an ellipsis (...) where the 
// trimming took place.
void StringConcat
// CHANGES:
// (01.5.29) changed string:;size_type to long because it was an unsigned type
// and thus XsLen could never be negative. Also deleted ellipsis option
// because it's not portable.
(string& ioDestStr, const char* iSuffixStr, int iSizeLimit)
{
	long theDestLen = ioDestStr.size();
	long theSuffLen = std::strlen (iSuffixStr);
	long theXsLen = theDestLen + theSuffLen - iSizeLimit;

	if (0 < theXsLen)
	{
		// if dest string must be trimmed to fit length
		// shift end token to necessary position
		ioDestStr.erase (ioDestStr.size() - theXsLen, theXsLen);
	}

	// then just join them together
	ioDestStr += iSuffixStr;
}


// *** TEST & DEBUG FUNCTIONS

#ifdef SBL_DBG

#include <iterator>

void StringUtils_Test ()
{ 
	DBG_MSG ("*** Testing StringUtils");

	std::string		theStlString ("   It was the   best of   times   ");
	char*				theCString = "   It was the   worst of   times  ";
	DBG_MSG ("StlString is \"" << theStlString << "\"");
	DBG_MSG ("CString is \"" << theCString << "\"");

	DBG_MSG ("Test membership fxns ...");
	DBG_MSG ("h found in StlString: " << isMemberOf ('h', theStlString));
	DBG_MSG ("h found in StlString 8-16: " << isMemberOf ('h', theStlString.begin()
		+ 8, theStlString.begin() + 16));
	DBG_MSG ("h found in CString: " << isMemberOf ('h', theCString));

	DBG_MSG ("Test transformation fxns ...");
	toUpper (theStlString.begin(), theStlString.begin() + 12);
	DBG_MSG ("StlString toupper 0-12: \"" << theStlString << "\"");
	toUpper (theStlString);
	DBG_MSG ("StlString toupper: \"" << theStlString << "\"");
	toLower (theCString);
	DBG_MSG ("CString tolower: \"" << theCString << "\"");
	std::string theDummyStr;
	theDummyStr = theStlString;
	eraseLeadingSpace (theDummyStr);
	DBG_MSG ("StlString erase leading: \"" << theDummyStr << "\"");
	theDummyStr = theStlString;
	eraseTrailingSpace (theDummyStr);
	DBG_MSG ("StlString erase trailing: \"" << theDummyStr << "\"");
	theDummyStr = theStlString;
	eraseFlankingSpace (theDummyStr);
	DBG_MSG ("StlString erase flanking: \"" << theDummyStr << "\"");

	DBG_MSG ("Test split & join ...");
	vector<string>		theStrArr;
	std::back_insert_iterator< vector<string> >  theArrIter (theStrArr);
	split (theStlString, theArrIter);
	DBG_MSG ("The split result array:");
	vector<string>::iterator p;
	for (p = theStrArr.begin(); p != theStrArr.end(); p++)
		DBG_MSG ("\"" << *p << "\"");
	std::string theTmpStr;
	join (theStrArr.begin(), theStrArr.end(), theTmpStr, '*');
	DBG_MSG ("The join string: " << theTmpStr);

	DBG_MSG ("Test conversions ...");
	DBG_MSG ("Convert: \"" << 1.0000 << "\"-to-\"" << toString (1.0000) << "\"");
	DBG_MSG ("Convert: \"" << 123 << "\"-to-\"" << toString (123) << "\"");
	DBG_MSG ("Convert: \"" << 's' << "\"-to-\"" << toString ('s') << "\"");
	theTmpStr = "23.45678";
	DBG_MSG ("Convert: \"" << theTmpStr << "\"-to-\"" << toDouble (theTmpStr) << "\"");
	theTmpStr = "23456";
	DBG_MSG ("Convert: \"" << theTmpStr << "\"-to-\"" << toLong (theTmpStr) << "\"");
	uchar* thePString = "\pa pascal string";
	string theConvStlStr;
	P2StlStr (thePString, theConvStlStr);
	DBG_MSG ("Convert a pascal string: \"" << theConvStlStr << "\"");
	char	theConvCStr[256];
	P2CStr (thePString, theConvCStr);
	DBG_MSG ("Convert a pascal string: \"" << theConvCStr << "\"");




	DBG_MSG ("*** Finished testing StringUtils");
}

#else
	void StringUtils_Test ();
#endif


// *** SUFFIX FUNCTIONS **************************************************/
#pragma mark -// The raison d'etre of these functions is to allow the safe construction 
// of file names, say by adding a suffix on to the end of the original
// file name. The optional argument sticks an ellipsis (...) where the 
// trimming took place.

void AddExtension
(char* ioDestStr, const char* ikSuffixStr, int iSizeLimit,
bool iAddEllipsis)
{
	string::size_type theDestLen = std::strlen (ioDestStr);
	string::size_type theSuffLen = std::strlen (ikSuffixStr);
	string::size_type theXsLen;

	// 3 case of calculating the excess length
	switch (iSizeLimit)
	{
		// no limit: trust user has provided dest string of adequate length
		case kStr_NoLimit:
			theXsLen = 0;
			break;

		// natural limit: an error since there is no natural limit to C
		// strings, so there is no safe number
		case kStr_NaturalLimit:
			assert(false);
			break;

		// finally: use a supplied limit which in the case of kStr_Limit
		// is 255
		case kStr_Limit:
		default:
			theXsLen = theDestLen + theSuffLen - iSizeLimit;
			break;
	}

	if (0 < theXsLen)
	{
		// if dest string must be trimmed to fit length
		// shift end token to necessary position
		ioDestStr[theDestLen - theXsLen] = '\0';
		if (iAddEllipsis)
			ioDestStr[theDestLen - theXsLen - 1] = kChar_Ellipsis;
	}

	// then just join them together
	std::strcat (ioDestStr,ikSuffixStr);
}


void AddExtension
(uchar* ioDestStr, uchar* iSuffixStr, int iSizeLimit, bool iAddEllipsis)
{
	string::size_type theDestLen = ioDestStr[0];
	string::size_type theSuffLen = iSuffixStr[0];
	string::size_type theXsLen;

	// 3 case of calculating the excess length
	switch (iSizeLimit)
	{
		// no limit: trust user has provided dest string of adequate length
		case kStr_NoLimit:
			theXsLen = 0;
			break;

		// natural limit: for pascal strings this is 255
		case kStr_NaturalLimit:
			theXsLen = theDestLen + theSuffLen - 255;
			break;

		// finally: use a supplied limit which in the case of kStr_Limit
		// is 255
		case kStr_Limit:
		default:
			theXsLen = theDestLen + theSuffLen - iSizeLimit;
			break;
	}

	if (0 < theXsLen)
	{
		// if dest string must be trimmed to fit length
		// shift end token to necessary position
		ioDestStr[theDestLen - theXsLen + 1] = '\0';
		if (iAddEllipsis)
			ioDestStr[theDestLen - theXsLen] = uchar (kChar_Ellipsis);
		ioDestStr[0] -= theXsLen;
		theDestLen = ioDestStr[0];
	}

	// then just join them together & incr length
	std::memcpy (&ioDestStr[theDestLen + 1], &iSuffixStr[1], theSuffLen);
	ioDestStr[0] += theSuffLen;
}


void AddExtension
(string& ioDestStr, const char* iSuffixStr, int iSizeLimit, bool iAddEllipsis)
{
	string::size_type theDestLen = ioDestStr.size();
	string::size_type theSuffLen = std::strlen (iSuffixStr);
	string::size_type theXsLen = theDestLen + theSuffLen - iSizeLimit;

	if (0 < theXsLen)
	{
		// if dest string must be trimmed to fit length
		// shift end token to necessary position
		ioDestStr.erase (ioDestStr.size() - theXsLen, theXsLen);
		if (iAddEllipsis)
			ioDestStr[ioDestStr.size() - 1] = kChar_Ellipsis;
	}

	// then just join them together
	ioDestStr += iSuffixStr;
}


bool StripExtension	(uchar* ioPString)
{
	int theLength = ioPString[0];
	int i;

	// step back along string until you find suffix start
	for (i = theLength; (0 < i) and ioPString[i] != '.'; i--)
		/* empty loop */ ;

	// if found beginning of suffix
	if (ioPString[i] == '.')
	{
		ioPString[i] = '\0';
		ioPString[0] = uchar (i - 1);

		return true;
	}
	else
	{
		return false;
	}
}

void stripExt (string& ioStr)
{
	string::size_type theIndex = ioStr.find_last_of ('.');
	if ((theIndex != string::npos) and (theIndex != 0))
		ioStr.erase (ioStr.begin() + theIndex, ioStr.end());
}

bool StripExtension	(string& ioTargetString)
{
	string::size_type theLength = ioTargetString.length() - 1;
	string::size_type i;

	// step back along string until you find suffix start
	for (i = theLength; (0 < i) and ioTargetString[i] != '.'; i--)
		/* empty loop */ ;

	// if found beginning of suffix
	if (ioTargetString[i] == '.')
	{
		ioTargetString.erase (i);
		ioTargetString[i] = '\0';

		return true;
	}
	else
	{
		return false;
	}
}


bool ReplaceExtension
(uchar* ioDestPString, uchar* iSuffixPString,
int iSizeLimit, bool iAddEllipsis)
{
	if (not StripExtension (ioDestPString))
	{
		// if strip failed
		AddExtension (ioDestPString,iSuffixPString,iSizeLimit,iAddEllipsis);
		return false;
	}
	else
	{
		AddExtension (ioDestPString,iSuffixPString,iSizeLimit,iAddEllipsis);
		return true;
	}
}


bool ReplaceExtension
(string& ioTargetString, char* iSuffixCString,
int iSizeLimit, bool iAddEllipsis)
{
	if (not StripExtension (ioTargetString))
	{
		// if strip failed
		AddExtension (ioTargetString,iSuffixCString,iSizeLimit,iAddEllipsis);
		return false;
	}
	else
	{
		AddExtension (ioTargetString,iSuffixCString,iSizeLimit,iAddEllipsis);
		return true;
	}
}


// *** MEMBERSHIP FUNCTIONS
#pragma mark -


// Does it end with this suffix?
bool PStrEndsWith (uchar* iTargetPStr, uchar* iSuffixPStr)
{
	assert (iSuffixPStr[0] <= iTargetPStr[0]);

	int theSuffixStart = iTargetPStr[0] - iSuffixPStr[0] + 1;
	return (not std::memcmp (&iTargetPStr[theSuffixStart], &iSuffixPStr[1],
		iSuffixPStr[0]));
}


bool PStrEndsWith (uchar* iTargetPStr, char* iSuffixCStr)
{
	int theSuffixLength = int (std::strlen (iSuffixCStr));
	assert (theSuffixLength <= iTargetPStr[0]);

	int theSuffixStart = iTargetPStr[0] - theSuffixLength + 1;
	return (not std::memcmp (&iTargetPStr[theSuffixStart],
		&iSuffixCStr[0], ulong (theSuffixLength)));
}




#pragma mark -
// *** C STRING FUNCTIONS ***********************************************/
// Largely for converting Mac Pascal strings to more civilized and common
// forms.

// STRING CONCAT
// As noted above in the suffic functions, this is chiefly to allow the
// safe construction  of file names.
void StrConcat
(char* ioDestStr, char* iSuffixStr, int iSizeLimit, bool iAddEllipsis)
{
	long theDestLen = std::strlen (ioDestStr);
	long theSuffLen = std::strlen (iSuffixStr);
	long theXsLen = theDestLen + theSuffLen - iSizeLimit;

	if (0 < theXsLen)
	{
		// if dest string must be trimmed to fit length
		// shift end token to necessary position
		ioDestStr[theDestLen - theXsLen] = '\0';
		if (iAddEllipsis)
			ioDestStr[theDestLen - theXsLen - 1] = kChar_Ellipsis;
	}

	// then just join them together
	std::strcat (ioDestStr,iSuffixStr);
}


void StringConcat
(string& ioDestStr, const char* iSuffixStr, int iSizeLimit, bool iAddEllipsis)
{
	iAddEllipsis = iAddEllipsis; // just to shut compiler up
	StringConcat (ioDestStr, iSuffixStr, iSizeLimit);
}

SBL_NAMESPACE_STOP

// *** END ***************************************************************/
