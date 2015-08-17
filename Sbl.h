/**************************************************************************
Sbl.h - library header

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London, Silwood
  Park, Ascot, Berks, SL5 7PY, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- The default header to the Sibil library which includes all the Sibil
  peculiar types, debug macros & necessary headers.
- The details for the debugging macros, as they are somewhat lengthy, are
  placed down below.
  
Changes:
- 99.9.29: created again.
- 00.3.1: put everything in one file/header for convenience. (Especially
  for users who might only want to pick and choose from parts of the
  library.) Removed the inclusion of Error.h (the standard exceptions)
  because while their use is common, it is not universal. Also introduced
  the namespace macros which allow 1/ a clean and obvious delimiting on
  of the namespace, 2/ use of the namespace to be optional.
- 00.7.25: started to split into seperate files again <sigh> because it's
  all just too bulky. Shifted types out first.
  
To Do:
- See notes for debugging below

**************************************************************************/


#ifndef SBL_H
#define SBL_H




// *** SIBIL NAMESPACE
// The Sibil namespace in on by default. To switch it off, define the
// below symbol.

#ifdef SBL_NAMESPACE_OFF
	#define SBL_NAMESPACE_START	
	#define SBL_NAMESPACE_STOP	
#else
	#define SBL_NAMESPACE_START	namespace sbl {
	#define SBL_NAMESPACE_STOP		}
#endif

// old usage depreciated
#define START_SBL_NAMESPACE	SBL_NAMESPACE_START
#define STOP_SBL_NAMESPACE		SBL_NAMESPACE_STOP


// get c++ unused var warning to shut up
#define UNUSED(expr) do { (void)(expr); } while (0)

// *** TYPES *************************************************************/

#include "SblTypes.h"


// *** DEBUGGING MACROS **************************************************/

#include "SblDebug.h"
			

#endif

// *** END ***************************************************************/



