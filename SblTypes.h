/**************************************************************************
SblTypes.h - library header

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- All the Sibil peculiar types. Mostly they are defined for brevity.
  
Changes:
- 00.7.25: moved out from "Sbl.h"
  
To Do:
- other types will no doubt be needed.

**************************************************************************/


#ifndef SBLTYPES_H
#define SBLTYPES_H



SBL_NAMESPACE_START

// *** UNSIGNED NUMBERS
typedef unsigned char	uchar;
typedef unsigned int		UInt;
typedef unsigned long	ulong;

// *** FIXED SIZE NUMBERS
// TO DO: unsure how to get size out of system, hence assert.
// assert (sizeof (ulong) == 4);
typedef ulong				word32;
typedef unsigned char	word8;
typedef word8 				byte;


SBL_NAMESPACE_STOP

#endif

// *** END ***************************************************************/



