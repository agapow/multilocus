/**************************************************************************
BasicScanner - abstract base class for a reader/lexer for parsing

Credits:
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- Aka reader, lexer, scanner, source iterator. Enables a source to be
  broken up into tokens or logical units.
- Derive from this to make scanners that handle various input sources,
  like streams, fstreams, arrays, buffers etc.
- Actually we differentiate between this and a lexer, because a lexer
  understands something of the syntax of the parsed langauge

**************************************************************************/

#ifndef BASICSCANNER_H
#define BASICSCANNER_H


// *** INCLUDES

#include "Sbl.h"

#include "StringUtils.h"
#include <string>
#include <vector>


SBL_NAMESPACE_START


// *** CONSTANTS & DEFINES

enum eoln_t
{
	kEoln_Unknown,
	kEoln_Dos,
	kEoln_Mac,
	kEoln_Unix
};

typedef int posn_t;

// Format codes

const char kFormatTab 		= 't';
const char kFormatNotTab 	= 'T';
const char kFormatNumber 	= 'n';
const char kFormatEoln		= 'l';

const bool kScan_Eat = true;
const bool kScan_DontEat = false;

const bool kScan_SkipSpace = true;
const bool kScan_DontSkipSpace = false;

const posn_t kScan_SrcBegin = 0;
const posn_t kScan_SrcEnd = -1;


// *** CLASS DECLARATION *************************************************/

class BasicScanner
{
public:
	// LIFECYCLE
				BasicScanner	()
					: mStartComment ("/*"),
					mStopComment ("*/"),
					mLineComment ("//"),
					mMeta ("#"),
					mSpace (" \r\n\t"),
					mEolnType (kEoln_Unknown),
					mCommentsAreSpace (true)
					{}
	virtual	~BasicScanner	() {};
		
	// ACCESS
	void 	SetComments	
		( const char* iStartToken, const char* iStopToken	)
		{ mStartComment = iStartToken; mStopComment = iStopToken; }
	void 	SetLineComment	( const char* iLineToken )
		{ mLineComment = iLineToken; }
	void 	SetMeta ( const char* iMetaToken )
		{ mMeta = iMetaToken; }
	void 	SetSpace ( const char* iCharSet )
		{ mSpace = iCharSet; }
		
	virtual UInt GetLineIndex ();

	// SERVICES
		
	// !! Primitives - low level, must be overridden in derived class.
	// !! Do not access these directly.
	
	// Get Char: get next raw char from source, return sucess, CountLines()
	virtual bool 	GetChar		(char& ch) = 0;
	// Go To: move to given posn, 0=start, -1=end, adjust line count
	virtual posn_t	Goto 			(posn_t iPosn) = 0;
	// Get Posn: grab current position for later use in GoTo()	
	virtual posn_t	GetPosn 		() = 0;
	// Cast as bool: is there anything left in the source?
	virtual operator	bool () const = 0;

	// !! Low Level Services - for infrequent but direct access 
	
	bool				ReadChar 	(char& oCurrChar);
	virtual bool 	PeekChar		(char& ch);
	virtual posn_t	Rewind 		();	
	virtual posn_t	Wind 			();
	virtual eoln_t	DetectEoln	();
	
	// !! High Level Services

	// Consume functions read characters until they encounter one that is
	// not in their set, then rollback to just before it
	char 	ConsumeWhile 		(const char *iCharSet);
	void 	ConsumeUntil 		(const char *iCharSet);
	void 	ConsumeUntilToken (const char *iToken, bool iEatToken = kScan_DontEat);
	char 	ConsumeSpace		();
	void 	ConsumeLine			();

	void	ReadCharThrow 		(char& ch, bool iSkipSpace = true);
	bool	ReadCharSkipSpace	(char& ch);

		
	

	void		ReadFormat 		(std::string& oToken, const char* ikFormat);
	void		ReadFormat		(std::vector<std::string>& oTokenVector, const char* ikFormat);

	void		ReadOne 			(std::string& ioToken, const char* iCharSet);
	void		ReadOneOrNone 	(std::string& ioToken, const char* iCharSet);
	
	char		Read				(std::string& ioToken, const char* iDelimiters, bool iEatDelim = kScan_DontEat);
	void		ReadExpected	(const char* iExpectedCStr);
	char 		ReadToken		(std::string& ioToken, const char* iDelimiters = "",
										bool iEatDelim = kScan_DontEat);
	void 		ReadLine			(std::string& ioLine, bool iEatSpace = false);
	void 		ReadLine			(std::string& ioLine, const char* iDelimiters, bool iEatSpace = false);

	void		ReadWhile		(std::string& ioToken, const char* iCharSet);
	char		ReadUntil		(std::string& ioToken, const char* iCharSet, bool iEatDelimiter = false);
	
	void		ReadNumberToken	(std::string& ioToken);
	void		ReadIntToken 		(std::string& ioToken);

	void 		UnreadToken	(std::string& iToken);
	void 		PeekToken	(std::string& oToken);

	
	
	int		ReadInt 			();
	double	ReadDbl 			();


// DEPRECIATED & DEBUG

	virtual void 	UnreadChar	(char ch) = 0;


// INTERNALS

private:
	// format parameters
	std::string	mStartComment, mStopComment, mLineComment, mMeta, mSpace;
	eoln_t	mEolnType;
	bool		mCommentsAreSpace;
	
	// helper functions
	bool		isMemberOf				(const char* ikCharSet, char iTestChar);
	void		SkipWhileComment 	();
	bool		IsCommentDelim		(std::string& theDelim);
};


SBL_NAMESPACE_STOP

#endif
// *** END ***************************************************************/