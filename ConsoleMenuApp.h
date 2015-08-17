/**************************************************************************
ConsoleMenuApp - simple menu oriented console application

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About:
- Derive from this to make bog-standard, C++ apps using menus.

**************************************************************************/

#ifndef CONSOLEMENUAPP_H
#define CONSOLEMENUAPP_H


// *** INCLUDES

#include "Sbl.h"
#include "ConsoleApp.h"
#include "CommandMgr.h"

SBL_NAMESPACE_START


// *** CONSTANTS & DEFINES

// *** CLASS DECLARATION *************************************************/

class ConsoleMenuApp: public ConsoleApp
{
public:
	// Lifecycle
	ConsoleMenuApp		() {};	// use defaults
	~ConsoleMenuApp	() {};
		
	// Services		
	virtual void	Run			();							// main event loop
	
	virtual void	LoadMenu		() = 0;						// _must_ override
	virtual bool	UpdateCmd	(cmdId_t iCmdId) = 0;	// _must_ override
	virtual void	ObeyCmd		(cmdId_t iCmdId) = 0;	// _must_ override
	
	// UI functions
	cmdId_t		AskUserCommand		(CommandMgr *iCommandMgr, const char* iMenuTitle = NULL);
	void			PrintMenuChoice	(cmdInfo_t *iCommandPtr);

protected:
	CommandMgr	mMainCommands;	// main event loop commands
};


SBL_NAMESPACE_STOP

#endif

// *** END ***************************************************************/



