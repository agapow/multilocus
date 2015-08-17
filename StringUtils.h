/**************************************************************************
StringUtils.h - assorted std::string utility functions 

Credits:
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- Some functions for C strings (char*), Pascal strings (uchar*) and C++ /
  STL strings.

Changes:
- 99.7.1: Released.
- 00.7.18: Generalised & rationalised isMemberOf functions, introduced test
  fxn, generalised case conversion fxns (i.e. added iterators), renamed
  whitespace stripping fxns (so "erase" indicates a removal in place).

To Do:
- Plenty. See notes below. Clean up interface.
- Comments under split / join

**************************************************************************/

#ifndef STRINGUTILS_H
#define STRINGUTILS_H


#include "Sbl.h"
#include "ContainerUtils.h"
#include <string>
#include <vector>
#include <cctype>
#include <sstream>


SBL_NAMESPACE_START

// *** CONSTANTS & DEFINES
#pragma mark Constants

enum case_t
{
	kCase_Upper,
	kCase_Lower,
	kCase_Mixed
};

const int	kStr_Limit 				= 255;
const int	kStr_NaturalLimit 	= -2;
const int	kStr_NoLimit 			= -1;
const bool	kStr_AddEllipsis 		= true;
const bool	kStr_DontAddEllipsis	= false;

const char	kStr_DefExtDelimiter	= '.';

typedef std::string::size_type	string_size_t;
//: So we don't get any compiler warnings about implicit conversions.


// *** MEMBERSHIP FUNCTIONS **********************************************/
#pragma mark "Membership fxns"

bool isMemberOf (char iSearchChar, const std::string& iTargetStr);
bool isMemberOf (char iSearchChar, const char* iTargetStr);
bool isMemberOf (const char* iSearchChar, const std::string& iTargetStr);
bool isMemberOf (const char* iSearchChar, const char* iTargetStr);


// *** TRANSFORMATION ****************************************************/
#pragma mark "Transformation"

// *** CASE CONVERSION

template <typename OutIter>
void
toUpper (OutIter iStartIter, OutIter iStopIter)
//: Convert all characters in the target sequence to upper case.
// Note this conversion happens in place.
{
	for (; iStartIter != iStopIter; iStartIter++)
	{
		*iStartIter = char (std::toupper(int(*iStartIter)));
	}
}

template <typename OutIter>
void
toLower (OutIter iStartIter, OutIter iStopIter)
//: Convert all characters in the target sequence to lower case.
// Note this conversion happens in place.
{
	for (; iStartIter != iStopIter; iStartIter++)
	{
		*iStartIter = char (std::tolower(int(*iStartIter)));
	}
}


void toUpper (std::string& ioTargetString);
void toUpper (char* ioTargetCString);
void toLower (std::string& ioTargetString);
void toLower (char* ioTargetCString);


// *** STRIP WHITESPACE
// TO DO: need for C strings?
string_size_t eraseLeadingSpace (std::string& ioTargetStr);						
string_size_t eraseTrailingSpace (std::string& ioTargetStr);						
string_size_t eraseFlankingSpace (std::string& ioTargetStr);						
string_size_t eraseInternalSpace (std::string& ioTargetStr);						
string_size_t eraseAllSpace (std::string& ioTargetStr);						
string_size_t reduceSpace (std::string& ioTargetStr);						



// *** TYPE CONVERSION ***************************************************/
#pragma mark "Type conversion"

template <typename X>
std::string
toString (X iSrcVal)
//: Attempt to convert and return any value as an stl-std::string.
// Not a very robust proceedure, with zero error checking, but it is very
// flexible ...
{						
	std::string				theDestStr;
	std::stringstream		theBuffer;
	
	theBuffer << iSrcVal;
	/*
	theBuffer >> theDestStr;
	return theDestStr;
	*/
	return theBuffer.str();
}

double	toDouble		(const std::string& ikSrcStr);
long		toLong		(const std::string& ikSrcStr);
bool		isReal		(const std::string& ikTargetStr);
bool		isWhole		(const std::string& ikTargetStr);

template <typename X>
void
stringTo (std::string& iSrc, X& iDest)
//: Attempt to convert a std::string and return it as the accompanying value.
{						
	std::ostringstream		theBuffer (iSrc);	
	theBuffer >> iDest;
}


// *** TOKENIZING & EXTRACTION *******************************************/
#pragma mark "Split & Join"

template <typename OUTITER>
int
split (std::string& iSrcString, OUTITER iResultIter, char iDelimiter = ' ')
//: Breaks a std::string into substrings by given delimiter.
{
// TO DO: general container manipulation?
// TO DO: const?
// TO DO: can be expanded to work with c strings and multi char delimiters.
// TO DO: extracting from strings according to formats or regex?
// TO DO: probably a better way of doing this

	string_size_t	theStrLen = iSrcString.length();
	std::string			theDestString;
	int				theNumTokens = 0;
	
	for (string_size_t i = 0; i < theStrLen; i++)
	{
		if (iSrcString[i] == iDelimiter)
		{
			*iResultIter = theDestString;
			iResultIter++;
			theDestString = "";
			theNumTokens++;
		}
		else
		{
			theDestString += iSrcString[i];
		}
	}
	
	*iResultIter = theDestString;
	return (theNumTokens + 1);
}


template <typename SRCITER>
void
join
(SRCITER iSrcStart, SRCITER iSrcStop, std::string& oDestString, char iDelimiter = ' ')
//: Fuse a container of strings into a single std::string, with given delimiter.
// Basically the opposite of Split(). An evolution of Merge(), renamed so
// it agrees with the name used in Perl & Python.
{						
// TO DO: general container manipulation?
// TO DO: const?
// TO DO: can be expanded to work with c strings and multi char delimiters.
// TO DO: probably a better way of doing this

	if (iSrcStart == iSrcStop)
		return;
	
	oDestString += *iSrcStart;
	iSrcStart++;
	
	for (; iSrcStart != iSrcStop; iSrcStart++)
	{
		oDestString += iDelimiter;
		oDestString += *iSrcStart;
	}
}


// *** PASCAL STRING FXNS ************************************************/
#pragma mark "Pascal std::string fxns"

void P2StlStr	(const uchar* iSrcPStr, std::string& ioDestStlStr);
void P2CStr		(const uchar* iSrcPStr, char* ioDestCStr);				

void C2PStr 	(const char* iSrcCStr, uchar* ioDestPStr);								
void Stl2PStr	(const std::string& iSrcStlStr, uchar* ioDestPStr);

void PStrCopy	(uchar* iSrcPString, uchar* ioDestPString);


// *** DEPRECIATED, DEBUG, DEVELOPMENT ***********************************/
#pragma mark "Depreciated, Debug, Development"

bool	StrMember	(const char* iTargetStr, char iSearchChar);

void	MakeUppercase	(std::string& ioTargetString);
void	MakeUppercase	(char* ioTargetCString);
void	MakeLowercase	(std::string& ioTargetString);
void	MakeLowercase	(char* ioTargetCString);

string_size_t	StripLeadingWhitespace	(std::string& ioTargetString);						
string_size_t	StripTrailingWhitespace	(std::string& ioTargetString);							
string_size_t	StripFlankingWhitespace	(std::string& ioTargetString);							

int 	Split	(std::string& iSrcString, std::vector<std::string>& ioDestVector,
					char iDelimiter);							
void 	Merge (std::vector<std::string>& iSrcVector, std::string& iDestString,
					char* iDelimiter = "");						

int 		String2Int 	(std::string& iStlString);
double 	String2Dbl	(std::string& iStlString);

uchar*			ReturnPString	(char* iCString);							
const uchar*	ReturnPString	(std::string& iStlString);

char* 	ReturnCString 		(uchar* iPString);							

uchar*	MakePString		(char* iCString);								
uchar*	MakePString		(std::string *iStlString);
void		MakePString		(std::string& iStlString, uchar* ioPString);
void		MakePString		(uchar* iSrcPString, uchar* ioDestPString);

void 		MakeStlString	(uchar* iDestPString, std::string& oStlString);

bool		StripPStrSuffix	(uchar* ioPString);
bool		ReplacePStrSuffix	(uchar* ioDestPStr, uchar* iSuffixPStr,
										int iSizeLimit = kStr_Limit,
										bool iAddEllipsis = kStr_DontAddEllipsis);

void	 StringUtils_Test ();


// *** TO BE PROCESSED ***************************************************/

// *** SUFFIX FUNCTIONS

bool		isFloat		(const std::string& ikTargetStr);

void	AddExtension		(char* ioDestStr, const char* ikSuffixStr,
									int iSizeLimit = kStr_Limit,
									bool kStr_AddEllipsis = kStr_DontAddEllipsis);
void	AddExtension		(uchar* ioDestStr, uchar* iSuffixStr,
									int iSizeLimit = kStr_NoLimit,
									bool kStr_AddEllipsis = kStr_DontAddEllipsis);
void	AddExtension		(std::string& ioDestStr, const char* iSuffixStr,
									int iSizeLimit = kStr_NoLimit,
									bool iAddEllipsis = kStr_DontAddEllipsis)	;	
												
bool	StripExtension		(uchar* ioPString);
bool	StripExtension		(std::string& ioTargetString);
void stripExt (std::string& ioStr);

bool	ReplaceExtension	(uchar* ioDestPString, uchar* iSuffixPString,
									int iSizeLimit = kStr_Limit,
									bool iAddEllipsis = kStr_AddEllipsis);
bool	ReplaceExtension	(std::string& ioDestPString, char* iSuffixCString,
									int iSizeLimit = kStr_Limit,
									bool iAddEllipsis = kStr_AddEllipsis);
bool ReplaceExtension 	(std::string& ioTargetString, char* iSuffixCString,
									int iSizeLimit, bool iAddEllipsis);
// To Do: default extension chars


// *** C STRING FUNCTIONS

void		StrConcat			(char* ioDestStr, char* iSuffixStr, int iSizeLimit,
										bool iAddEllipsis = false);
								
								
// *** PASCAL STRING FUNCTIONS

bool	PStrEndsWith		(uchar* iTargetPStr, uchar* iSuffixPStr);
bool	PStrEndsWith 		(uchar* iTargetPStr, char* iSuffixCStr);


void		PStrConcat			(uchar* ioDestStr, uchar* iSuffixStr,
										int iSizeLimit = kStr_Limit,
										bool iAddEllipsis = kStr_AddEllipsis);
										
bool		PStrStripSuffix	(uchar* ioPString);
bool		PStrReplaceSuffix	(uchar* ioDestPString, uchar* iSuffixPString,
										int iSizeLimit = kStr_Limit,
										bool iAddEllipsis = kStr_AddEllipsis);

bool		PStrEndsWith		(uchar* iTestPStr, uchar* iSuffixPStr);
bool		PStrEndsWith		(uchar* iTestPStr, char* iSuffixStr);


// *** STL STRING FUNCTIONS
								
void		StringConcat		(std::string& ioDestStr, const char* iSuffixStr,
										int iSizeLimit);

// *** LEGACY
void	StringConcat	(std::string& ioDestStr, const char* iSuffixStr,
								int iSizeLimit, bool iAddEllipsis);


SBL_NAMESPACE_STOP

#endif

// *** END ***************************************************************/
