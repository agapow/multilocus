/**************************************************************************
BasicScanner - abstract base class for a reader/lexer for parsing

Credits:
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://mesa@agapow.net> <http://www.agapow.net/software/mesa//>


About:
- Aka reader, lexer, scanner, source iterator.
- Derive from this to make scanners that handle various input sources,
  like streams, fstreams, arrays, buffers etc.
- Enables a source to be broken up into tokens or logical units
- Note that none of the tokenizing functions clear the token reference
  passed in, so that complex tokens can be built in segments.

Changes:
- 99.9.30: Created.
- 00.2.22: Expanded tokenizing functions with ReadNumberToken(),
  ReadIntToken() and format functions.
- 00.3.9: big shakeup in the layout that will slightly break some code
  uses previous versions. Several of the pure abstract functions have moved
  to the base class and a number of the functions (like UnreadChar) have
  been depreciated. These is to decrease the amount of overriding needed
  in derived classes and because the functionality required by some
  cannot be guaranteed in derived classes. For example, Unread fxns have
  unpredictable semantics. What does it mean to "unread" a character?
  What if this character was not the char that was originally read off?
  How does it cope with information that has been filtered out of the 
  input (like comments)? Thus unread has been depreciated in favour of
  Peek() and Goto().
- 00.3.13: - Keeping track of line counts is a great hassle. It means that
  Goto must step over every char between the current and new positions to
  keep track  of how the line number changes. It strikes me that line number
  is called  for usually in error conditions, while Goto is called quite
  frequently. Maybe GetLineIndex() should calculate it on the fly. 
   
To Do:
- Should it throw when it reaches the end of file? How do we handle eof?
- Correct treatment of copy & assignment constructors?
- Case conversion. Embed in ReadChar primitive. Or have a class-wide
  Compare function with switchable behaviour.
- Have a no-throw switch for optional sections? or set a flag or fxn to
  determine behaviour in failure situations?
- Inline?
- Replace all calls to unread with peek
- Have a flag such that comments count as whitespace (instead of being
  effectively invisible). This means that comments will interupt tokens
  such that "ato[comment]ken" is interpreted as "ato" & "ken", instead of
  "atoken".
- Suggested terminology: a sequence is an ordered set of characters of
  any sort. A token is a sequence that is delimited by and may not include
  whitespace and any supplied delimiters. A set is an unordered collection
  of characters. Hence we can differentiate between ReadUntilSet (read
  chars until you find one of the set), ReadUntilToken (keep grabbing
  tokens until you get this one), ReadUntilSeq (read until this sequence
  crops up). Perhaps Set should be "Of" or "OneOf".
- There is an inconsistency in the design that bothers me. Consume/Skip
  fxns return the character that terminates their scan, while Read fxns
  return the success of the operation. Solutions (1) Change Consume
  fxns to return the sucess status, & rely on PeekChar to find out the
  terminating condition; (2) have a "Throw On EOF" flag that can be set
  during reading units that connot end abruptly.
  
**************************************************************************/


// *** INCLUDES
#pragma mark Include

#include "BasicScanner.h"

#include "StringUtils.h"
#include "Error.h"
//#include <cstring>
#include <cstdlib>
#include <cerrno>

using std::strlen;
using sbl::String2Int;
using sbl::String2Dbl;

SBL_NAMESPACE_START


// *** CONSTANTS & DEFINES

#pragma mark -
// *** BASIC SCANNER *****************************************************/

// *** LIFECYCLE *********************************************************/

// see header for implementation


// *** BASIC SERVICES ****************************************************/
// Should be no need to override as these use the abstract low level fxns
// that must be overridden in every derived class. However all these fxns
// are virtual so overriding is possible if need be.

// *** EOLN FUNCTIONS
// Remember we have no idea what we could be dealing with here - Unix,
// Dos or Mac sources.

// GET EOLN
// Attempts to detect what eoln characters this file is using. Originally
// this relied on pre-calculated totals of linefeeds and carriage returns
// performed during reading, but for reasons listed in Changes above, this
// was changed.
eoln_t BasicScanner::DetectEoln ()
{
	bool		isDetected = false;
	eoln_t	theResult = kEoln_Unknown;
	char		theInChar;
	posn_t	thePosn	= Rewind();	// bookmark posn & goto beginning
	
	while (GetChar (theInChar) and not isDetected)
	{
		switch (theInChar)
		{
			case '\n':
				char theNextChar;
				if (PeekChar (theNextChar))
				{
					if (theNextChar == '\r')
					{
						isDetected = true;
						theResult = kEoln_Dos; 	// \n\r is dos
					}
					else
					{
						isDetected = true;
						theResult = kEoln_Unix;	// \n is unix
					}
				}
				else
				{
					// else nothing left in file
					isDetected = true;
					theResult = kEoln_Unknown;
				}
				break;
				
			case '\r':
				isDetected = true;
				theResult = kEoln_Mac;				// \r is unix
				break;
				
			default:
				// do nothing
				break;
		}
	}
	
	// if you reach here without detection, it's a mystery
	Goto (thePosn);
	mEolnType = theResult;
	return theResult;
}		


UInt BasicScanner::GetLineIndex ()
{ 
	posn_t	theCurrPosn = Rewind ();
	UInt		theNumCr = 0;
	UInt		theNumLf = 0;

	// count the number of line character before current posn
	while (GetPosn() != theCurrPosn)
	{
		char	theCurrChar;
		
		GetChar (theCurrChar);
		
		if (theCurrChar == '\r')
			theNumCr++;
		else if (theCurrChar == '\n')
			theNumLf++;
	}
	
	// based on above totals and type of line-ending, how many lines?
	UInt theNumLines = 0;
	
	switch (mEolnType)
	{
		case kEoln_Unknown:
			if (theNumCr)
				theNumLines = (theNumCr + 1);
			else if (theNumLf)
				theNumLines = (theNumLf + 1);
			else
				theNumLines = 0;
			break;
			
		case kEoln_Dos:
			theNumLines = (theNumCr + 1);
			break;
			
		case kEoln_Mac:
			theNumLines = (theNumCr + 1);
			break;

		case kEoln_Unix:
			theNumLines = (theNumLf + 1);
			break;

		default:					
			assert (false);	// shouldn't get here
	}
	
	return theNumLines;			// to keep compiler quiet			
}


// *** LOW LEVEL READING & HELPERS
// It is through these functions that access to the source should be made,
// not via the primitives that must be overridden.

// READ CHAR
// Pop the next interesting (non-comment) char off the stream, and return
// success. Strips comments out. Returns failure (false) if the source runs
// out of data or other circumstances conspire to prevent data being read.
// Changes: 00.3.9 - changed this from _the_ reading primitive (which is
// now GetChar) to the call that delivers the filtered output, that which
// is stripped of comments etc. Thus ReadChar can now be defined in the 
// abstract base class, and need not be defined for all derived classes.
// This is the call that all high-level reading functions should use -
// direct calls to GetChar() are verboten.
// !! Changes: 00.3.13 - the comment consuming items are now in their own
// helper functions for clarity.
// !! Changes: 00.3.17 - set char to NULL in failure conditions, so that if
// the return status is ignored, the failure won't be quiet.
// !! Changes: 00.3.17 - allow comments to be space 
// !! To Do: maybe there should be a "compress space" flag where
// contiguous sequences of spaces are compressed into one.
// !! To Do: what about nested and line comments?
bool BasicScanner::ReadChar (char& oCurrChar)
{
	// eat any comment at the current position
	if (IsCommentDelim (mStartComment))
	{
		SkipWhileComment();
	
		// allows comments to be whitespace & interrupt tokens
		if (mCommentsAreSpace == true)
		{
			if (0 < mSpace.length())
				oCurrChar = mSpace[0];
			else 
				oCurrChar = ' ';
		}
	}
	else
	{
		// return the current char, fail if we've hit the end of the source
		if (not GetChar (oCurrChar))
		{
			// just in case the return status is ignored
			oCurrChar = 0;
			return false;
		}
	}
	return true;
}


// SKIP WHILE COMMENT
// If a comment starts at the current position, consume it. We don't
// assume that this is always called with a comment waiting, beacuse it
// may have wider uses in the future. Once again, less efficient but more
// robust.
// !! Note: liberal use of debugging code.
// !! Note: a difference between searching for comments starts and stops.
// If no stopComment is defined an error is thrown, because no stop can be
// found. If no startComment is defined, that's alright because that means
// no comment can ever be started.
// !! Note: if an end of source is encountered before the identity of a 
// delimiter is established, the delimiter is not detected. This is a 
// probable error condition, but we wait until we directly encounter the
// end of source before panicking. If end-of-source is encountered before
// a comment is closed, this is an error (according to C++ behaviour).
void BasicScanner::SkipWhileComment ()
{
	// if the current posn does not or cannot herald a comment, return
	if ((0 == mStartComment.length()) or (not IsCommentDelim (mStartComment)))
		return;
	
	// now we are starting a comment - consume the start
	DBG_BLOCK(std::string dbgStart);
	char	theNextChar;
	
	for (int i = 0; i < (int) mStartComment.length(); i++)
	{
		char theNextChar;
		GetChar(theNextChar);
		DBG_BLOCK(dbgStart += theNextChar);
	}
	DBG_BLOCK(assert (dbgStart == mStartComment));
	
	// look for the comment end
	if (0 == mStopComment.size())
		throw ParseError ("no comment stop defined");
		
	bool theStopIsFound = false;	
	do
	{
		// found it! stop here!
		if (IsCommentDelim (mStopComment))
		{
			theStopIsFound = true;
			break;
		}
	}
	while  (GetChar(theNextChar));
	
	// if the end isn't found, we have a run on comment or unexpected
	// end of source. Throw.
	if (theStopIsFound == false)
		throw EndOfFileError ("no comment stop defined");


	// consume the comment stop
	DBG_BLOCK(std::string dbgStop);
	for (int i = 0; i < (int) mStopComment.length(); i++)
	{
		char theNextChar;
		GetChar(theNextChar);
		DBG_BLOCK(dbgStop += theNextChar);
	}
	DBG_BLOCK(assert (dbgStop == mStopComment));
}	


// IS COMMENT DELIMITER
// An internal method that sees if a comment delimitor starts at the
// current posn. This combines the need for IsComment[Start|Stop]. For
// sucess (the delim is found) to be returned, the comment token must be
// completed in full. If it is interrupted by end of source, it does not
// count. It is left to higher functions like SkipComment() to handle
// these error conditions. (You never know where this fxn will be called
// from.) 
// !! Note: this way of processing comments is definitely less efficient
// but it has greater clarity and is more robust to changes. Also it means
// the function only returns from one location (not counting
// preconditions).
bool BasicScanner::IsCommentDelim (std::string& theDelim)
{
	// Precondition:
	// if there's no delim defined, there's no point in searching for it
	if (0 == theDelim.size())
		return false;

	// try and match to delimiter token
	bool		theSucess = false;
	posn_t	theStartPosn = GetPosn ();
	char		theNextChar;
	std::string	theTarget;

	// if necessary, look ahead the full length of delimComment token
	while (theTarget.length() < theDelim.length())
	{
		// if the source runs out, break
		if (not GetChar (theNextChar))
			break;

		// if the input doesn't match delimiter, break 
		theTarget += theNextChar;
		assert (*(theTarget.end() - 1) == theNextChar);
		if (theDelim.compare (0, theTarget.length(), theTarget) != 0)
			break;
	}
	
	if (theTarget == theDelim)
	{
		// we found a comment delim
		assert (theTarget.length() == theDelim.length());
		theSucess = true;
	}

	// in any event, rollback to start of delimiter
	Goto (theStartPosn);
	
	return theSucess;
}


// PEEK CHAR
// Peek at what is next in the stream without changing position. This must
// be done through the primitives (and not higher facilities like peek()
// in Stl streams) so line number and comment skipping can be done.
bool BasicScanner::PeekChar (char& oNextChar)
{
	posn_t	theCurrPosn = GetPosn ();
	bool theSucess = GetChar (oNextChar);
	Goto (theCurrPosn);
	return theSucess;
}


// REWIND
// Go to the beginning of the source, before anything has been read. In
// derived classes, GoTo() must be ideated so as to fulfill this need.
posn_t BasicScanner::Rewind ()
{
	return Goto (kScan_SrcBegin);
}


// WIND
// Go to the end of the source, after everything has been read. In
// derived classes, GoTo() must be ideated so as to fulfill this need.
posn_t BasicScanner::Wind ()
{
	return Goto (kScan_SrcEnd);
}


#pragma mark -
// *** CONSUME ***********************************************************/
// Read characters while a given condition is met (i.e. you encounter a
// char that is not in their set), then rollback to just before it.
// To Do: rename as "skip"?

// CONSUME WHILE
// Keep consuming while these characters are in the stream. Reset pointer
// to just before. Tested 99.10.8
// To Do: what happens if it never finds the token?
// Changes: 00.3.8 - now returns terminating char
char BasicScanner::ConsumeWhile (const char *iCharSet)
{
	char theInChar;
	
	// while the source still has something in it
	while (ReadChar (theInChar))
	{
		// if we meet something not in the stream
		if (not isMemberOf (iCharSet, theInChar))
		{
			// rollback & return
			UnreadChar (theInChar);
			return theInChar;
		}
	}
	
	// if you reach here, you've hit the end of the file
	return theInChar;
}


// CONSUME UNTIL
// Keep consuming until you meet one of these characters. Reset to just 
// before character. Tested 99.10.8
// To Do: what happens if it never finds the token?
void BasicScanner::ConsumeUntil (const char *iCharSet)
{
	char theInChar;
	
	// while the source still has something in it
	while (ReadChar (theInChar))
	{
		// if the char is in the delimiters list
		if (isMemberOf (iCharSet, theInChar))
		{
			// rollback & return
			UnreadChar (theInChar);
			return;
		} 
	}
}


// CONSUME UNTIL TOKEN
// Keep eating until you encounter this token. Reset position to just
// before token.
// Tested: 99.10.8
// To Do: what happens if it never finds the token?
// Changes: 00.3.6 - change so it can now eat stop token.
void BasicScanner::ConsumeUntilToken (const char *iToken, bool iEatStop)
{
	int 	theTokenLen = (int) strlen (iToken);
	char	theInChar;
	assert (0 < theTokenLen);
	
	while (ReadChar (theInChar))
	{
		// if found the first letter of the token
		if (iToken[0] == theInChar)
		{
			int thePosn = GetPosn () - 1;
			int i;
			for (i = 1; i < theTokenLen; i++)
			{
				if ((ReadChar (theInChar) and (iToken[i] == theInChar)))
				{
					// keep going
				}
				else
				{
					Goto (thePosn + 1);
					break;
				}
			}
			if (theTokenLen == i) // have found whole token
			{
				if (iEatStop == kScan_DontEat)
					Goto (thePosn);
				return;
			}
		} 
	}
}


// CONSUME SPACE
// Keep eating whitespace until you find something that isn't. Note this 
// only copes with the printables & common whitespace.
// Changes: 00.3.8 - now returns terminating char
char BasicScanner::ConsumeSpace ()
{
	return ConsumeWhile (mSpace.c_str());
}


// Keep until the end of line has been consumed. Note this relies
// upon the comment consuming functions leaving the eoln intact.
// Remember that this chews up until the end of the line.
void BasicScanner::ConsumeLine ()
{
	char theInChar;
	
	switch (mEolnType)
	{
		case kEoln_Unknown:
			ConsumeUntil ("\n\r");
			ReadChar (theInChar);
			break;
			
		case kEoln_Dos:
			ConsumeUntil ("\n");
			ReadChar (theInChar);
			ReadChar (theInChar);
			assert (theInChar == '\r');
			break;
			
		case kEoln_Mac:
			ConsumeUntil ("\r");
			ReadChar (theInChar);
			break;
			
		case kEoln_Unix:
			ConsumeUntil ("\n");
			ReadChar (theInChar);
			break;
			
		default:
			assert (false);
			break;
	}
}


#if 0
#pragma mark -
// *** READ FXNS *********************************************************/
// Read characters until a given condition is met, rollback to just before
// it and return read chars in buffer.

// READ FORMAT (string&, const char*)
// Read format and store token in single string. Throw if it doesn't
// match the obligate parts of the the format supplied.
// !! To Do: expand to include 
void BasicScanner::ReadFormat (std::string& oToken, const char* ikFormat)
{
	UInt theFormatLen = strlen(ikFormat);
	
	for (UInt i = 0; i < theFormatLen; i++)
	{
		char theTokenType = ikFormat[i];
		switch (theTokenType)
		{
			case kFormatTab:
			{
				char theInChar;
				ReadChar(theInChar);
				if (theInChar == '\t')
					oToken += "\t";
				else
					throw ExpectedError("tab", (std::string(1,theInChar)).c_str());
				break;
			}
			
			case 'w':
			{
				char theInChar;
				ReadChar(theInChar);
				if (theInChar == '\t')
					oToken += "\t";
				else
					throw ExpectedError("expected tab");
				break;
			}
		
			default:
				assert(false);
		}
	
	}	
}


// READ FORMAT (vector<string>&, const char*)
// Read format and store each token matching a format code in string in
// vector. Throw if it doesn't match format.
void BasicScanner::ReadFormat
(std::vector<std::string>& oTokenVector, const char* ikFormat)
{
	UInt theFormatLen = strlen(ikFormat);
	
	for (UInt i = 0; i < theFormatLen; i++)
	{
		char theTokenType = ikFormat[i];
		switch (theTokenType)
		{
			case 't':
			{
				char theInChar;
				ReadChar(theInChar);
				if (theInChar == '\t')
					oTokenVector.push_back(std::string("\t"));
				else
					throw ExpectedError("expected tab");
				break;
			}
			
			case 'w':
			{
				char theInChar;
				ReadChar(theInChar);
				if (theInChar == '\t')
					oTokenVector.push_back(std::string("\t"));
				else
					throw ExpectedError("expected tab");
				break;
			}
		
			default:
				assert(false);
		}
	}	
}
#endif

// *** READ PRIMITIVES
#pragma mark -

// IS MEMBER 
// Is the testchar (2nd arg) found in the first set (1st arg)? If
// so return true. Note there is an equivalent function in StringUtils but
// we put this here for cohesion.
bool BasicScanner::isMemberOf(const char* ikCharSet, char iTestChar)
{
	UInt theSetLen = strlen(ikCharSet);
	for (UInt i = 0; i < theSetLen; i++)
	{
		if (iTestChar == ikCharSet[i])
			return true;
	}
	return false;
}


// *** READ (Vanilla)
#pragma mark -

// To Do: this could be combined with ReadExpected or ReadToken? Anyway,
// merge it into main code stream.
void BasicScanner::ReadCharThrow (char& oChar, bool iSkipSpace)
{
	if (iSkipSpace)
		ConsumeSpace ();
	if (not ReadChar (oChar))
		throw EndOfFileError();
}

// get the next non-space char
bool BasicScanner::ReadCharSkipSpace (char& oChar)
{
	ConsumeSpace();
	return ReadChar (oChar);
}


// READ (string&, const char*)
// Read until one of the delimiters is found. Return stopping char.
// Changes: 00.3.7 - now returns stopping char, so calling fxns may get
// the character. Also allows consuming char.
char BasicScanner::Read
(std::string& ioToken, const char* iDelimiters, bool iEatDelimiter)
{
	char theInChar;
	ioToken = "";		// purge buffer
	
	// while the source still has something in it
	while (ReadChar (theInChar))
	{
		if (isMemberOf (iDelimiters, theInChar))
		{
			// if the input is a delimiter, rollback & leave
			if (iEatDelimiter == false)
				UnreadChar (theInChar);
			return theInChar;
		}
		else
		{
			// otherwise append it to the token
			ioToken.append (1, theInChar);
		}
	}
	
	// If you get here, the read has been terminated by the end of source.
	return 127;
}


// READ EXPECTED
// A shorthand fxn - if you know what token should be next, call this. It
// will throw if the said token does not arise.
// !! To Do: use delimiters.
// !! Note: this only copes with tokens, not characters!
void BasicScanner::ReadExpected	(const char* iExpectedCStr)
{
	std::string theExpToken;
	ReadToken (theExpToken);
	if (theExpToken != iExpectedCStr)
		throw ExpectedError (iExpectedCStr, theExpToken.c_str());
}


// READ LINE (string&)
// Read up to & including eoln, return result in buffer, excluding eoln.
// By default it does not trim whitespace
void 	BasicScanner::ReadLine (std::string& ioLine, bool iEatSpace)
{
	char theInChar;
	ioLine = "";		// purge buffer
	
	if (iEatSpace)
		ConsumeSpace();
		
	switch (mEolnType)
	{
		case kEoln_Unknown:
			Read (ioLine, "\n\r");
			if (ReadChar (theInChar))
			{
				if (theInChar == '\n')
				{
					if (ReadChar (theInChar) and (theInChar != '\r'))
						UnreadChar (theInChar);
				}
				else if (theInChar == '\r')
				{
					// do nothing
				}
				else
					assert (false);
			}
			break;
			
		case kEoln_Dos:
			Read (ioLine, "\n");
			ReadChar (theInChar);
			ReadChar (theInChar);
			assert (theInChar == '\r');
			break;
			
		case kEoln_Mac:
			Read (ioLine, "\r");
			ReadChar (theInChar);
			assert (theInChar == '\r');
			break;
			
		case kEoln_Unix:
			Read (ioLine, "\n");
			ReadChar (theInChar);
			assert (theInChar == '\n');
			break;
			
		default:
			assert (false);
			break;
	}
	
	if (iEatSpace)
	{
		int theLastCharPosn = (int) ioLine.find_last_not_of (mSpace);
		ioLine.erase ((ulong) theLastCharPosn + 1);
	}
}


// READ LINE (string&, const char*)
// Read up to & including eoln or delimiter, return result in buffer
// excluding eoln. Delimiters are not consumed.
void 	BasicScanner::ReadLine
(std::string& ioLine, const char* iDelimiters,  bool iEatSpace)
{
	char 		theInChar = '?';	// for debugging purposes
	std::string	theDelimiters (iDelimiters);
	ioLine = "";		// purge buffer

	if (iEatSpace)
		ConsumeSpace();
			
	switch (mEolnType)
	{
		case kEoln_Unknown:
			theDelimiters += "\n\r";
			Read (ioLine, theDelimiters.c_str());
			if (ReadChar (theInChar))
			{
				if (isMemberOf (iDelimiters, theInChar))
				{
					// if stopped by delimiter, leave on stream
					UnreadChar (theInChar);
				}
				else if (theInChar == '\n')
				{
					// if stopped by eoln, consume it
					if (ReadChar (theInChar) and (theInChar != '\r'))
						UnreadChar (theInChar);
				}			
				else if (theInChar == '\r')
				{
					// if Mac eoln, consume by doing nothing
				}
				else
					assert (false);
			}
			break;
			
		case kEoln_Dos:
			theDelimiters += "\n";
			Read (ioLine, theDelimiters.c_str());
			if (ReadChar (theInChar))
			{
				if (isMemberOf (iDelimiters, theInChar))
				{
					// if stopped by delimiter, leave on stream
					UnreadChar (theInChar);
				}
				else if (theInChar == '\n')
				{
					// if stopped by eoln, consume it
					ReadChar (theInChar);
				}
			}		
			break;
			
		case kEoln_Mac:
			theDelimiters += "\r";
			Read (ioLine, theDelimiters.c_str());
			// if stopped by delimiter, leave on stream
			if (ReadChar (theInChar) and isMemberOf (iDelimiters, theInChar))
				UnreadChar (theInChar);
			break;
			
		case kEoln_Unix:
			theDelimiters += "\n";
			Read (ioLine, theDelimiters.c_str());
			// if stopped by delimiter, leave on stream
			if (ReadChar (theInChar) and isMemberOf (iDelimiters, theInChar))
				UnreadChar (theInChar);
			break;
			
		default:
			assert (false);
			break;
	}
	
	if (iEatSpace)
	{
		ulong theLastCharPosn = ioLine.find_last_not_of (mSpace);
		ioLine.erase (theLastCharPosn + 1);
	}
}


// READ TOKEN
// Read the next contiguous sequence of non-whitespace characters, i.e we
// assume that the standard whitespace are delimiters for tokens.
// !! Changes: 00.3.7, introduced extra delimiters argument.
// !! To Do: should be more functions with different definitions of token
// delimitation?
char 	BasicScanner::ReadToken
(std::string& ioToken, const char* iDelimiters, bool iEatDelimiter)
{
	ConsumeSpace ();
	std::string theDelimiters = mSpace + iDelimiters;
	return Read (ioToken, theDelimiters.c_str(), iEatDelimiter);
}


// READ WHILE
// Keep building string while these characters are in the stream. 
void BasicScanner::ReadWhile (std::string& ioToken, const char* iCharSet)
{
	char theInChar;
	ioToken = "";		// purge buffer
	
	// while the source still has something in it
	while (ReadChar (theInChar))
	{
		if (not isMemberOf (iCharSet, theInChar))
		{
			// if the input is not in set, rollback & leave
			UnreadChar (theInChar);
			return;
		}
		else
		{
			// otherwise append it to the token
			ioToken.append (1, theInChar);
		}
	}
}


// READ ONE
// Read one char that must match one of the supplied set. If not, throw.
void BasicScanner::ReadOne (std::string& ioToken, const char* iCharSet)
{
	char theInChar;
	
	ReadChar (theInChar);
	
	// check for problem
	if (isMemberOf (iCharSet, theInChar))
	{
		// if successful
		ioToken += theInChar;
	}
	else
	{
		// otherwise there's a problem
		std::string theErrorStr ("got \"");
		theErrorStr += theInChar;
		theErrorStr += "\", expected one of \"";
		theErrorStr += iCharSet;
		theErrorStr += "\"";
		
		throw ParseError(theErrorStr.c_str());
	}	
}


// READ ONE OR NONE
// Read one char and match against supplied set. If no match, unread char
// and return.
void BasicScanner::ReadOneOrNone (std::string& ioToken, const char* iCharSet)
{
	char theInChar;
	
	ReadChar (theInChar);
	
	// if it's not a member of the set
	// check for problem
	if (isMemberOf (iCharSet, theInChar))
	{
		// if successful
		ioToken += theInChar;
	}
	else
	{
		// otherwise there's a problem
		UnreadChar (theInChar);
	}
}


// READ UNTIL
// Keep building string until these characters are in the stream. Normally
// this will read up until a delimiter and then stop _before_ the 
// delimiter.
// Changes: 00.3.7 - introduced EatDelimiter option and fxn returning
// the exact delimiter than stops it.
char BasicScanner::ReadUntil
(std::string& ioToken, const char* iCharSet, bool iEatDelimiter)
{
	char theInChar;
	ioToken = "";		// purge buffer
	
	// while the source still has something in it
	while (ReadChar (theInChar))
	{
		if (isMemberOf (iCharSet, theInChar))
		{
			// if the input is not in set, rollback & leave
			if (iEatDelimiter == false)
				UnreadChar (theInChar);
			return theInChar;
		}
		else
		{
			// otherwise append it to the token
			ioToken.append (1, theInChar);
		}
	}
	
	// just to keep the compiler quiet and catch errors - we should
	// never get to this point.
	assert (false);
	return '?';
}

void BasicScanner::PeekToken	(std::string& oToken)
{
	posn_t theSavePosn = GetPosn();
	ReadToken (oToken);
	Goto (theSavePosn);
}



// *** READ TYPES ********************************************************/
// Read basic types and return result in token string. Make as large a
// token as possible. Throw if unable to process.
// To Do: where are the throws?
#pragma mark -


// READ NUMBER TOKEN
// Return any number tokenized.  Note that these numbers may start with + or
// -. 
void BasicScanner::ReadNumberToken (std::string& ioToken)
{
	std::string	theIntPortion, thePoint, theFraction;
	
	ReadIntToken (theIntPortion);
	ReadWhile (thePoint, ".");
	ReadWhile (theFraction, "0123456789");

	ioToken += theIntPortion + thePoint + theFraction;
}


// READ INTEGER TOKEN
// Return any integer tokenized.  Note that these numbers may start with +
// or -. 
void BasicScanner::ReadIntToken (std::string& ioToken)
{
	std::string	theSign, theNumber, thePoint, theFraction;
	
	ReadOneOrNone (theSign, "-+");
	ReadWhile (theNumber, "0123456789");

	ioToken += theSign + theNumber;
}


// *** DEPRECATED & TEST FUNCTIONS **************************************/
#pragma mark -

// READ INT
// Return the integer representation of a string. Note that these numbers
// may start with + or -. 
// To Do: should I really be consuming space at the beginning of these?
int BasicScanner::ReadInt ()
{
	std::string	theToken;
	
	ReadToken (theToken);
	char** theEndOfNumber = NULL;
	long int theAnswer = std::strtol (theToken.c_str(), theEndOfNumber, 0);
	if (errno != 0)
	{
		std::string theMsg = "'";
		theMsg += theToken;
		theMsg += "' isn't an integer";
		throw Error (theMsg.c_str());
	}

	return theAnswer;
}


// READ DOUBLE
// Return the float representation of a string. Note that these numbers
// may start with + or -, and may feature scientific (e) representation.
// To Do: should I really be consuming space at the beginning of these?
double BasicScanner::ReadDbl ()
{
	std::string	theSign, theInt, thePoint, theFraction, theE, theExponent;
	
	ConsumeSpace ();
	ReadWhile (theSign, "-+");
	ReadWhile (theInt, "0123456789");
	ReadWhile (thePoint, ".");
	ReadWhile (theFraction, "0123456789");
	ReadWhile (theE, "Ee");
	ReadWhile (theExponent, "0123456789");
	theSign += theInt + thePoint + theFraction + theE + theExponent;
	return (String2Dbl (theSign));
}

void BasicScanner::UnreadToken	(std::string& iToken)
{
	for (std::string::size_type i = iToken.size() - 1; 0 <= 1; i--)
		UnreadChar (iToken[i]);
}


SBL_NAMESPACE_STOP

// *** END