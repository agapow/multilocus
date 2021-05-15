/**************************************************************************
Error.h - general exception class

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- An exception class for throwing on error conditions, that contains info
  to base informative & accurate error handling & messaging.
- Note that like the C++ class it is derived on, the error message is
  extracted with the function what(). Classes derived from Error should
  ensure that errors are instantiated as MyError("a description of the
  specfic problem") and the message returned by what ().
- Methods & members:
  - Ctor () is a simply initialiser, sets the description to null.
  - Ctor (char*) inits the description with a string.
- The architecture of this component is that Error provides a mechanism
  for storing information, its immediate children define different
  classes of errors, their children define different contexts
  in which they might be called. For example ParseError represents all
  logical and format errors encountered during scanning and reading in.
  These errors have the general form "Parse Error (<location>, <message>)"
  which are specified by the subclasses.
  
Changes:
- 99.9.2: created.
- 99.9.15: expanded built-in types, changed name to Exception from
  SblException.
- 99.9.28: ... and thence to Error, as CodeWarrior keeps finding the std::
  "exception.h" rather than my "Exception.h" when it goes looking for the
  file, causing many compilation problems. Also had GetMsg outside the
  definition but because it lead to problems being multiply included in
  files and I didn't want to do a .cpp file just for it, so it's now
  inside the definition. 
- 00.3.1: this used to be a general header for Sibil but now has to be
  specifically included. (This allows Sibil.h to be the single obligate
  header, and trims a bit of fat.) Rehashed the class architecture and
  responsibilities as above.
- 00.10.31: removed any responsibility for formatting messages and 
  presentation from Error and made them now just containers of information.
  
To Do:
- Introduce namespaces

**************************************************************************/

#ifndef ERROR_H
#define ERROR_H


// *** INCLUDES

#include "Sbl.h"
#include <string>
#include <cstring>
#include <exception>

SBL_NAMESPACE_START


// *** CONSTANTS & DEFINES


// *** CLASS DECLARATION *************************************************/
#pragma mark Basic error

class Error: public std::exception
{
public:

// *** LIFECYCLE
	Error	()
		: mDesc ("general error")
		{}
		
	Error	(const char* iWhat)
		: mDesc (iWhat)
		{}

	~Error () throw()
		{}

// *** ACCESSORS		
	const char* what () const throw()
		{ return mDesc.c_str ();}

	std::string		mDesc;	// specific description of error
	std::string		mFile;	// file the error was thrown from
	int				mLine;	// the line number the error was thrown from
	
// *** DEPRECIATED & DEBUG

// *** INTERNALS	
private:
};



// *** PARSE ERRORS *****************************************************/
#pragma mark -

// *** BASIC PARSE ERROR
#pragma mark Basic parse error

class ParseError : public Error
//: thrown when there is a problem converting a representation into data
{ 
public:
	ParseError ()
		: Error ("parse error"),
		mSource (""), mLineNum(0), mColNum(0), mPosn(0)
		{}
	ParseError (const char* ikDesc, const char* ikSrcName = "")
		: Error (ikDesc), mSource (ikSrcName)
		{}
	ParseError (long iLineNum, const char* ikDesc)
		: Error (ikDesc), mLineNum (iLineNum)
		{}
	
	~ParseError () throw ()
		{}

	std::string		mSource;
	long				mLineNum;
	long				mColNum;
	long				mPosn;
};


class EndOfFileError : public ParseError
//: thrown when an end of file is unexpectedly encountered
{ 
public:
	EndOfFileError ()
		: ParseError ("end-of-file unexpectedly encountered")
		{}
	EndOfFileError (const char* ikDesc, const char* ikSrcName = "")
		: ParseError (ikDesc, ikSrcName)
		{}
		
	~EndOfFileError () throw ()
		{}
};


class ExpectedError : public ParseError
//: thrown when an unexpected symbol or token is encountered
{ 
public:
	ExpectedError ()
		: ParseError ("encountered an unexpected symbol")
		{}
	ExpectedError (const char* ikExpSymbol, const char* ikEncSymbol = "")
		{
			UNUSED (ikEncSymbol); // XXX: shut that compiler up
			
			mDesc = "expected \'";
			mDesc += ikExpSymbol;
			mDesc += "\'";
			if (std::strcmp (ikEncSymbol, "")) // if encountered symbol supplied
			{
				mDesc += ", found \'";
				mDesc += ikEncSymbol;			
				mDesc += "\' instead";
			}
		}

	~ExpectedError () throw ()
		{}
};


class FormatError : public ParseError
//: thrown when source is an unknown or invalid format
// TO DO: need better name
{ 
public:
	FormatError ()
		: ParseError ("source is an unknown or invalid format")
		{}
	FormatError (const char* iMsg)
		: ParseError (iMsg)
		{}
		
	~FormatError () throw ()
		{}
};


class ConversionError : public ParseError
//: thrown when a conversion between types fails
// TO DO: need better name
{ 
public:
	ConversionError ()
		: ParseError ("cannot convert string to target type")
		{}
	ConversionError (const char* ikSrc, const char* ikDest = "")
		{
			mDesc = "cannot convert \'";
			mDesc += ikSrc;
			mDesc += "\'";
			if (std::strcmp (ikDest, "")) // if encountered symbol supplied
			{
				mDesc += " to ";
				mDesc += ikDest;
			}
		}
		
	~ConversionError () throw ()
		{}
};


// *** FILE I/O ERRORS ***************************************************/
#pragma mark -

// *** BASIC FILE ERROR
#pragma mark Basic file error

class FileError : public Error
//: Thrown when there is a general problem with an external file.
{ 
public:
	FileError ()
		: Error ("file error"), mPath ("")
		{}
	FileError (const char* ikDesc, const char* ikFileName = "")
		: Error (ikDesc), mPath (ikFileName)
		{}

		
	~FileError () throw ()
		{}
	
	std::string		mPath;
};


// *** FILE IO ERROR
#pragma mark File missing error

class FileIOError : public FileError
//: Thrown when an external file cannot be read or written to
{ 
public:
	FileIOError ()
		: FileError ("file I/O error")
		{}
	FileIOError (const char* ikDesc, const char* ikFileName = "")
		: FileError (ikDesc, ikFileName)
		{}
		
	~FileIOError () throw ()
		{}
};


class FileReadError : public FileIOError
//: thrown when an external file cannot be read from
{ 
public:
	FileReadError ()
		: FileIOError ("could not read from file")
		{}
	FileReadError (const char* ikDesc, const char* ikFileName = "")
		: FileIOError (ikDesc, ikFileName)
		{}
		
	~FileReadError () throw ()
		{}
};


class FileWriteError : public FileIOError
//: thrown when an external file cannot be written to
{ 
public:
	FileWriteError ()
		: FileIOError ("could not write to file")
		{}
	FileWriteError (const char* ikDesc, const char* ikFileName = "")
		: FileIOError (ikDesc, ikFileName)
		{}

		
	~FileWriteError () throw ()
		{}
};


class FileOpenError : public FileIOError
//: thrown when an external file cannot be opened
{ 
public:
	FileOpenError ()
		: FileIOError ("could not open file for I/O")
		{}
	FileOpenError (const char* ikDesc, const char* ikFileName = "")
		: FileIOError (ikDesc, ikFileName)
		{}
		
	~FileOpenError () throw ()
		{}
};


// *** FILE MISSING ERROR
#pragma mark File missing error

class MissingFileError : public FileError
//: thrown when an external file cannot be located.
{ 
public:
	MissingFileError ()
		: FileError ("file could not be located")
		{}
	MissingFileError (const char* ikDesc, const char* ikFileName = "")
		: FileError (ikDesc, ikFileName)
		{}
		
		
	~MissingFileError () throw ()
		{}		
};



// *** MISC ERRORS *******************************************************/
#pragma mark -

#pragma mark Unimplemented error

class UnimplementedError : public Error
//: thrown when a function is called but has not been written yet.
{ 
public:
	UnimplementedError ()
		: Error ("unimplemented function or command")
		{}
	UnimplementedError (const char* iDesc)
		: Error (iDesc)
		{}
		
	~UnimplementedError () throw ()
		{}
};


#pragma mark Index error

class IndexError : public Error
//: thrown when a container is accessed at an out of bounds location
{ 
public:
	IndexError ()
		: Error ("container indexed at bad location")
		{}
	IndexError (const char* iDesc)
		: Error (iDesc)
		{}

	~IndexError () throw ()
		{}
};



SBL_NAMESPACE_STOP

#endif

// *** END ***************************************************************/



