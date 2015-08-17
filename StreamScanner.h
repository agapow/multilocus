/**************************************************************************
StreamScanner - a reader/lexer for parsing a stream

Credits:
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://p.agapow@ucl.ac.uk> <mail://a.burt@ic.ac.uk>
  <http://www.bio.ic.ac.uk/evolve/>

About:
- breaks an istream up into logical units for parsing.

**************************************************************************/


#ifndef STREAMSCANNER_H
#define STREAMSCANNER_H


// *** INCLUDES

#include "Sbl.h"
#include "BasicScanner.h"
#include <iostream>


SBL_NAMESPACE_START

// *** CONSTANTS & DEFINES

// *** CLASS DECLARATION *************************************************/

class StreamScanner: public BasicScanner
{
public:
// LIFECYCLE
	StreamScanner	()
		: mSrcStreamP (NULL)
		{}
	StreamScanner	(std::istream& iStream)
		: mSrcStreamP (&iStream)
		{}
	~StreamScanner	()
		{}
	
// ACCESS
	void	SetSource (std::istream& iSrcStream);

	// To Do: this should be in the ABC
	operator	bool () const
		{ return ((mSrcStreamP != NULL) and (bool (*mSrcStreamP))); }
	
// SERVICES
	// low level, obligate overrides
	posn_t	GetPosn 	();						// where	
	posn_t	Goto 		(int iPosn);			// go to given posn
	bool 		GetChar	(char& oCurrChar);	// get next char, return sucess


// DEPRECIATED

	void 		UnreadChar	(char iLastChar);

// INTERNALS
	
private:
	std::istream*	mSrcStreamP;
};


SBL_NAMESPACE_STOP

#endif

// *** END ***************************************************************/





