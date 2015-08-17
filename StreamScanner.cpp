/**************************************************************************
StreamScanner - a reader/lexer for parsing a stream

Credits:
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://mesa@agapow.net> <http://www.agapow.net/software/mesa//>


About:
- breaks an istream up into logical units for parsing.

Changes:
- 99.9.30: Created.

To Do:
- should it throw when it reaches the end of file?
- correct treatmemnt of copy & assignment ctors?

**************************************************************************/


// *** INCLUDES

#include "StreamScanner.h"
#include "StringUtils.h"
#include <iostream>

using std::ios;
using std::istream;


SBL_NAMESPACE_START

// *** CONSTANTS & DEFINES

// *** STREAM SCANNER ****************************************************/

// *** LIFECYCLE *********************************************************/

// see header for implementation

// *** ACCESS ************************************************************/

// SET SOURCE
// Record the stream that we are reading from
void StreamScanner::SetSource (istream& iSrcStream)
{
	mSrcStreamP = &iSrcStream;
}


// *** SERVICES **********************************************************/
// These are the low level fxns that must be overridden in every derived
// class. 

// *** MOVEMENT FUNCTIONS
// For changing position within the stream

// GET POSITION
// Where	are we?
posn_t StreamScanner::GetPosn ()
{
	return mSrcStreamP->tellg ();
}


// GO TO
// Go to a designated position
// Note: Goto clears previously existing end-of-file flags, as one may be
// going to a position within the source. If the new posiiton is actually
// beyond the end, end() will be set when the next read takes place,
// which is the correct behaviour.
posn_t StreamScanner::Goto (int iPosn)
{
	posn_t theOldPosn = GetPosn ();
	mSrcStreamP->clear ();
	assert (mSrcStreamP->eof() != true);
	
	switch (iPosn)
	{
		case kScan_SrcBegin:
			mSrcStreamP->seekg (0, std::ios::beg);
			break;
			
		case kScan_SrcEnd:
			mSrcStreamP->seekg (0, ios::end);
			break;
			
		default:
			mSrcStreamP->seekg (iPosn);
			break;
	}
	
	return theOldPosn;
}


// *** LOW-LEVEL READING

// the lowest, most primitve reading function. Doesn't skip comments or do
// anything other than get the next raw character on the stream and keep
// track of line numbers.
bool StreamScanner::GetChar (char& oCurrChar)
{
	if (*mSrcStreamP)
	{
		mSrcStreamP->get (oCurrChar);
		
		// !! Change: (00.1.26) We need the next statement because
		// eof() is only set to true after you try to read past it.
		if (mSrcStreamP->eof())
			return false;
		
		return true;
	}
	else
	{
		return false;
	}
}


// *** DEPRECATED & TEST FUNCTIONS **************************************/

// UNREAD CHAR
// Return char to stream.
// !! To Do: can I make a fxn UngetChar() that must be overridden and then
// define this in the abstract base class so that the ASBC has the line
// counting behaviour?
void StreamScanner::UnreadChar (char iLastChar)
{
	mSrcStreamP->putback (iLastChar);
}


SBL_NAMESPACE_STOP

// *** END