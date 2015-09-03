/**************************************************************************
SblDebug.h - library-wide debugging aids

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>
- The ctassert construct is taken from Kevin S Van Horn "Compile-Time
  Assertions in C++", _C++ Users Journal_, October 1997, and
  <http://www.xmission.com/~ksvhsoft/ctassert/ctassert.html>. 

About:
- Macros and functions for debugging. Included by the default Sibil header
  and inactivated if SBL_DBG is not defined.
- Note that internal variables (those created in macro expansion) are
  prefixed  by "dbg_" to preclude name collisions.
- All memory management macros assume that pointers should be non-zero &
  pointing at memeory of non-zero size.
- While all the macros are by definition public, the "Internals" are not
  intended to be called directly. So don't.
- DBG_RUBBISH: a magic value used to overwrite any memory that is about
  to be deleted so as to prevent accident reuse of released mem.
- ASSERT_PTR(ptr,size): is the pointer pointing at a non-zero location,
  to a block of non-zero size?
- SHRED(ptr,size): scribble garbage over a piece of memeory to prevent
  accidental reuse.
- PURGE(ptr): check a pointer is fine before releasing it and setting to
  zero.
- TRASH(ptr,size): shred the memory pointed at and null pointer. 
- gDebugStreamP / DBG_STREAM: the stream that messages are output to.
  Can be set by DBG_SETSTREAM(streamP).
- gDebugPrefixStr: the string that precedes all debug lines. Can be set
  by DBG_SETPREFIX(prefix).
- DBG_POSN(file,line): prints out the file * line no where the debug 
  call was made.
- DBG_PREFIX: the debug line prefix, set to prefix string & location.
- DBG_VAL(val): print out the name & value of a variable. Uses the
  internal DBG_PRINTVAL. For example:
		DBG_VAL (theVar);
- DBG_MSG(msg): send a message (string) to the debug stream. For
  example:
  		DBG_MSG("Making matrix with " << iNumRows);
- DBG_ARRAY(arrayP,n): print out the contents of an array or indexed
  container	given the size.
- DBG_VECTOR(vectorP): print out the contents of a vector or indexed
  container that answer the call size()
- DBG_BLOCK(block): define a block of code only to be executed in the
  debug version of the code. For example:
		DBG_BLOCK
		(
			DBG_VAL (theRPairwise);
			cout << i << endl;
			VerifyNodes();
		)

Changes:
- 00.7.25: split from main header due to bulk.
  
To Do:
- find a better rubbish value for shredding?
- better names?
- size of on *ptr?
- rtti
- start debug block & stop
- classid & names
- class::validate
- class::dump-describe
- class::test {internal ifdef only}
- array calls should validate pointers.  
- Unify names ("SBL_DBG ...")
- Introduce better logging & output functions.

**************************************************************************/

#pragma once
#ifndef SBLDEBUG_H
#define SBLDEBUG_H



// *** INCLUDE

#include <cassert>


// *** DEBUGGING OUTPUT **************************************************/

#ifdef DEBUG

	// *** INCLUDES
	// Necessary for debugging output
	
	#include <iostream>
	#include <string>

	// Internals
	// defined for brevity only, should be used only to define other
	// debug primitives and not used directly
	
	// !! get mutiply defined symbols on the below so replace them with hash defines
	// until I work it out
	// ostream*		gDebugStreamP 		= &cerr;
	// string		gDebugPrefixStr 	= "DEBUG: ";
	#define gDebugStreamP				(&(std::cerr))
	#define gDebugPrefixStr				"DEBUG: "
	
	#define DBG_STREAM					(*gDebugStreamP)
	#define DBG_POSN(file,line)		file << " (" << line << "): "
	#define DBG_PREFIX					DBG_STREAM << DBG_POSN( __FILE__, __LINE__)
	#define DBG_PRINTVAL(val)			"\"" << #val << "\" = [" << std::flush << (val) << "]"
	#define DBG_RUBBISH					((char) 169) /* 1001011 */
	
	// Public
	#define ASSERT_PTR(ptr,size)		assert (ptr != 0); assert (size != 0)
	#define SHRED(ptr,size)				ASSERT_PTR(ptr,size); \
												char* dbg_Ptr = (char*) ptr; \
												for (int dbg_i = 0; dbg_i < size; dbg_i++) \
												dbg_Ptr[dbg_i] =  DBG_RUBBISH;
	#define PURGE(ptr)					ASSERT_PTR(ptr,size); ptr = NULL
	#define TRASH(ptr,size)				SHRED(ptr,size); delete ptr; ptr = NULL
	// set output stream and debug line prefix
	#define DBG_SETSTREAM(streamP)	gDebugStreamP = streamP				
	#define DBG_SETPREFIX(prefix)		gDebugPrefixStr = prefix
	// print msg to debug stream				
	#define DBG_MSG(msg)					DBG_PREFIX << msg << std::endl;
	// print a variable and it's value to debug stream
	#define DBG_VAL(val)					DBG_PREFIX << DBG_PRINTVAL(val) << std::endl << std::flush
	// print contents of C array (or indexed container) 
	#define DBG_ARRAY(arrayP,n)		DBG_PREFIX << "Contents of array " << #arrayP << " at " << \
												(arrayP) << ":" << std::endl << std::flush; \
												for (int I = 0; I < n; I++) \
												DBG_PREFIX << "\tIndex [" << I << "]: " << \
												(*(arrayP))[I] << std::endl
	// print contents vector or indexed container
	#define DBG_VECTOR(vectorP)		DBG_ARRAY(vectorP,((vectorP)->size()))
	#define DBG_ANON_VECTOR(dbgVector, dbgWidth)	\
												DBG_PREFIX; \
												for (ulong dbgLoop = 0; dbgLoop < dbgVector.size(); dbgLoop++) \
												DBG_STREAM << dbgVector[dbgLoop] << " "; \
												DBG_STREAM << std::endl
	
	// call Dump() function on object 
	#define DBG_DUMP(object)			DBG_PREFIX << "*** Dumping object " << #object << " at " << \
												(&object) << ":" << std::endl << std::flush; object.Dump(); \
												DBG_PREFIX << "* Dump finishes " << std::endl << ::flush;
	
	/*
	template <class OBJECT>
	void
	SBL_DBG_DUMP (OBJECT& iDebugObj)
	//: print a representation of the objects contents by calling 
	{
		DBG_PREFIX << "*** Dumping object " << #iDebugObj << " at " <<
			(&iDebugObj) << ":" << std::endl << std::flush; object.dump();
		DBG_PREFIX << "* Dump finishes " << std::endl << ::flush;
	}
	*/
												
																								
	// call Dump() function on object 
	#define DBG_VALIDATE(object)		DBG_PREFIX << "*** Validating object " << #object << " at " << \
												(&object) << ":" << std::endl << std::flush; object.Validate(); \
												DBG_PREFIX << "* Validate finishes " << std::endl << std::flush;
	// define a block only to be executed in debug version 
	#define DBG_BLOCK(block)			block


	// *** COMPILE-TIME ASSERT
	// Use to test conditions at compile-time to proof against logic
	// errors. Call like this:
	// 	SBL_DBG_CTASSERT(1 == 1,isValid2);		// will suceed
	//		SBL_DBG_CTASSERT(1 == 0,isNotValid2);	// will fail
	// Note how the unused variable warning is suppressed
	// TO DO: how can this error report what causes it?
	template <bool dbgBOOLEXPR> 
	struct dbgCtAssert 
	{
		/* 1 if expression true, 0 otherwise */ 
		enum { N = (1 - 2 * int (!dbgBOOLEXPR)) };
		static char A[N];
	}; 

	template <bool dbgBOOLEXPR>
	char dbgCtAssert<dbgBOOLEXPR>::A[N]; 

	#define SBL_DBG_CTASSERT(dbgBoolExpr,dbgName)                      \
											dbgCtAssert<dbgBoolExpr> dbgName;    \
											dbgName = dbgName;
																						
#else

	
	// In a non debug situation, the debug calls are defined as nothing. To
	// my surprise the trailing semi-colons and empty statements aren't an
	// error in C++
	#define ASSERT_PTR(x,y)
	#define SHRED(x,y)
	#define PURGE(x)
	#define TRASH(x,y)
	#define DBG_SETSTREAM(x)		
	#define DBG_SETPREFIX(x)		
	#define DBG_MSG(x)				
	#define DBG_VAL(x)				
	#define DBG_ARRAY(x,y)		
	#define DBG_VECTOR(x)		
	#define DBG_BLOCK(x)		
	// TO DO: bad hack, just to shut up some error messages when DBg off
	#define DBG_PREFIX       std::cerr	
	#define DBG_STREAM       std::cerr			

	#define DEBUG_DUMP(x)
	#define SBL_DBG_CTASSERT(x)  	
#endif
			

// *** ASSERT ***************************************************/

// *** THROW IF FAIL
// Test the provided condition and if it fails, throw the provided error.
// Call as follows:
// ThrowIfFail<myErr>(*dataP != NULL);				// throw default-constructed error
// ThrowIfFail(false, fubar2("assert failed"));	// throw copy of provided error
// ThrowIfFail<fubar2>(false, "assert failed");	// construct exception from string
template <class ERROR>
inline void ThrowIfFail (bool iAssertion, const ERROR& irErr = ERROR())
	throw (ERROR)
{
	if (not iAssertion)
		throw irErr;
}


#endif

// *** END ***************************************************************/



