/**************************************************************************
CommandMgr.h - a managed collection of commands and associated information

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.bio.ic.ac.uk/evolve/>

About
- see source for comments.

**************************************************************************/

#ifndef COMMANDMGR_H
#define COMMANDMGR_H


// *** INCLUDES

#include "Sbl.h"
#include <string>
#include <vector>


SBL_NAMESPACE_START

// *** CONSTANTS & DEFINES

typedef	int	cmdId_t;		// i.d. for commands sent to the application
//typedef	char	shortcut_t;	// single letter for menu commands

struct cmdInfo_t				// stores all the information about what 
{									// commands are currently available
	cmdId_t			mCmd;
	std::string		mShortcut;
	std::string		mMenuStr;
	bool				mActive;
};

// predefined cmd ids. Negative numbers are reserved by Sibil. Null is
// the command returned by failures in the commandgroup
const cmdId_t kCmd_Quit = -3;
const cmdId_t kCmd_Open = -2;
const cmdId_t kCmd_Null = -1;


// *** CLASS DECLARATION *************************************************/

class CommandMgr
{
public:
	// LIFECYCLE
	CommandMgr	();
	CommandMgr	(const char* iTitleCstr);
	~CommandMgr	();
				
	// ACCESS
	int  countCommands ()
		{ return int (mCommands.size()); }
		
	cmdId_t getCommandId	(int iPosn)
	{
		assert (0 <= iPosn);
		assert (iPosn <  countCommands ());
		return mCommands[iPosn].mCmd;
	}
	
	// MUTATORS
	void			SetConvertShortcut		(bool iConvert);
	
	cmdInfo_t*	GetCommand 					(int thePosn);
	
	int 			GetCommandWithShortcut (const char* iShortcutCstr);
	int 			GetCommandWithShortcut (std::string iShortcutStr)
		{ return GetCommandWithShortcut (iShortcutStr.c_str()); }
	
	void	SetCommandActive	(cmdId_t iTargetCmd, bool iActivity);
	void	SetCommandActive	(bool iActivity);
	
	void	AddCommand (cmdId_t iCmdId, const char* iShortcutCstr,
								const char *iTitleCstr);
	void	AddCommand (cmdId_t iCmdId, const char *iTitleCstr);
			
	// PUBLIC MEMBERS
	std::string					mTitle;
								
	// DEPRECIATED & DEBUG
	void		validate ();

	int		GetNumCommands ();
	int 		GetCommandWithShortcut (char iShortcut);
	void		AddCommand (cmdId_t theCmdNum, char theShortcut,
								const char *theCmdTitle);
	
	// INTERNALS
private:
	std::vector<cmdInfo_t>	mCommands;
	bool							mDoCaseConversion;
	UInt							mMaxNumberedCmds;
	
	void init (const char* iTitleCstr);
};

SBL_NAMESPACE_STOP

#endif

// *** END ***************************************************************/







