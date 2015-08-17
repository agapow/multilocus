/**************************************************************************
ConsoleApp - simple text console based application & UI functions

Credits:
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://mesa@agapow.net> <mail://a.burt@ic.ac.uk>
  <http://www.agapow.net/software/mesa//>

About:
- Derive from this to make bog-standard, C++ apps.

Changes:
- (99.8.20) created, approximately.
- (99.9.29) removed menu functions to derived class & made this a pure ABC.
- (00.5.5) starting catching all answers in strstreams and getlines and
  then validating them.
- (00.9.21) removed dependency on StringUtils so module is now standalone.
- (01.4.5) changed "ask" function to new & terser naming convention, call
	"askInt" methods "askInteger" to emphasis they now accept longs, so one
	functioin fits all (whole) numbers.

To Do:
- examine & fix all ugly stringstream functions
- fxns to come include logging and better reports
- modify remaining fxns to use getline

**************************************************************************/


// *** INCLUDES
#pragma mark Includes

#include "ConsoleApp.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include "StringUtils.h"

using std::atoi;
using std::atol;
using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::stringstream;
using std::string;
using std::vector;
using std::left;
using std::setw;
using std::noskipws;
using std::strlen;

SBL_NAMESPACE_START


// *** CONSTANTS & DEFINES
#pragma mark Constants

const string kDefaultAppTitle 	= "Standard Console App";
const string kDefaultAppVersion	= "0.1";
const string kDefaultAppCredit	= "Paul-Michael Agapow";


// *** PROTOTYPES, TEMPLATES, INTERNALS


// *** MAIN BODY *********************************************************/
#pragma mark -
// *** LIFECYCLE *********************************************************/

ConsoleApp::ConsoleApp ()
{
	// set aboutbox info to reasonable defaults
	mAppTitle = kDefaultAppTitle;
	mAppVersion = kDefaultAppVersion;
	mAppCredits = kDefaultAppCredit;
	
	// set screen layout to reasonable defaults
	mScreenColsWidth = 75;
	mAboutBoxWidth = 60;
	mPromptWidth = 60;
	mCmdsVisible = false;	
	CalcAboutBoxIndent();
}

ConsoleApp::~ConsoleApp ()
{
	// do nothing, nothing is needed as yet
}


// *** APPLICATION FLOW **************************************************/
#pragma mark -
void ConsoleApp::Startup ()
{
	ShowAboutBox ();
}


// QUIT
// Print a friendly farewell msg, override to do any tidying-up
void ConsoleApp::Quit ()
{
	cout << endl << "Goodbye";
	
	// if a proper app title has been defined
	if (mAppTitle != kDefaultAppTitle)
		cout << " from " << mAppTitle;
	cout << "!" << endl;
}


// *** DISPLAY & APPEARANCE FUNCTIONS *************************************/
#pragma mark -
// *** ABOUT BOX WRANGLING

void ConsoleApp::ShowAboutBox ()
{
	// check strings are defined and aren't too long
	assert ((mAppTitle.length() != 0) and
		(mAppTitle.length() <= (mAboutBoxWidth - 4)));
	assert ((mAppVersion.length() != 0) and
		(mAppVersion.length() <= (mAboutBoxWidth - 4)));
	assert ((mAppCredits.length() != 0) and
		(mAppCredits.length() <= (mAboutBoxWidth - 4)));
	for (ulong i = 0; i < mAppInfo.size (); i++)
		assert (mAppInfo[i].length() <= (mAboutBoxWidth - 4));
		
	// print title box
	// print the top
	cout << endl;
	PrintAboutBorder ();
	
	// print title & version, then credits
	if ((mAppTitle.length() + mAppVersion.length()) <= (mAboutBoxWidth - 4))
	{
		// print title and version together on one line if possible
		string theTitleVersion = mAppTitle + " v" + mAppVersion;
		PrintAboutString (theTitleVersion);
	}
	else
	{
		// else print on seperate lines
		PrintAboutString (mAppTitle);
		PrintAboutString (mAppVersion);
	}
	
	// print blank line then credits
	string theTmpStr ("");
	PrintAboutString (theTmpStr);
	PrintAboutString (mAppCredits);
	
	// print other info if there is any
	if (0 < mAppInfo.size())
	{
		vector<string>::iterator p = mAppInfo.begin();
		while (p != mAppInfo.end())
		{
			PrintAboutString (*p);
			p++;
		}
	}
	
	// finish the box
	PrintAboutBorder ();
	cout << endl;
}

void ConsoleApp::CalcAboutBoxIndent ()
{
	mAboutBoxIndent = (mScreenColsWidth - mAboutBoxWidth) / 2;
	assert (0 <= mAboutBoxIndent);
}

void ConsoleApp::PrintAboutString (string& iContentString)
{
	UInt	theLengthContent = (UInt) iContentString.length(), 
			theSpaceBefore = (UInt) (mAboutBoxWidth - 2 - theLengthContent) / 2,
			theSpaceAfter = (UInt) mAboutBoxWidth - 2 - theLengthContent - theSpaceBefore;

	for (int i = 0; i < (int) mAboutBoxIndent; i++)
		cout << " ";
	cout << "*";
	
	for (int i = 0; i < (int) theSpaceBefore; i++)
		cout << " ";
		
	cout << iContentString.c_str();
	
	for (int i = 0; i < (int) theSpaceAfter; i++)
		cout << " ";
		
	cout << "*" << endl;	
}

void ConsoleApp::PrintAboutBorder ()
{
	for (int i = 0; i < (int) mAboutBoxIndent; i++)
		cout << " ";
	for (int i = 0; i < (int) mAboutBoxWidth; i++)
		cout << "*";
	cout << endl;
}


// *** SCREEN SETTINGS ***************************************************/
#pragma mark -

// how wide do you want the screen to go?
void ConsoleApp::SetCols (unsigned int iColsWidth)
{
	assert (mAboutBoxWidth <= iColsWidth);
	mScreenColsWidth = iColsWidth;
	CalcAboutBoxIndent();
}

// how wide is the about box?
void ConsoleApp::SetBox (unsigned int iBoxWidth)
{
	assert (mAboutBoxWidth <= mScreenColsWidth);
	mAboutBoxWidth = iBoxWidth;
	CalcAboutBoxIndent();
}

// how wide is prompt (question room)?
void ConsoleApp::SetPromptWidth (unsigned int iPromptWidth)
{
	assert (iPromptWidth <= mScreenColsWidth);
	mPromptWidth = iPromptWidth;
}

// show commands in the loop
void ConsoleApp::SetCmdVisibility (bool iCmdsVisible)
{
	mCmdsVisible = iCmdsVisible;
}


// *** QUESTION FORMATTING ***********************************************/
#pragma mark -
// This section is a bit of a mess due to the development of the question
// functions using different arguments at different times. The string&
// version should disappear some time.

void ConsoleApp::PrintQuestion (string *iPromptStr)
{
	PrintQuestion (iPromptStr->c_str());
}

void ConsoleApp::PrintQuestion (string &iPromptStr)
{
	assert (iPromptStr.length() < mPromptWidth);
	cout << left << setw (int (mPromptWidth)) << iPromptStr << ": ";
}

void ConsoleApp::PrintQuestion (const char* iPromptCstr)
{
	string theTmpStr (iPromptCstr);
	PrintQuestion (theTmpStr);
}

string ConsoleApp::getAnswer ()
//: collects user input and tidies it for processing
{
	string		theAnswerStr;
	getline (cin, theAnswerStr);
	eraseFlankingSpace (theAnswerStr);
	return theAnswerStr;
}

// *** UI FUNCTIONS ******************************************************/
#pragma mark -

// *** REPORT TO USER
// Note all of these accept a naked c string or "..." as a parameter.

void ConsoleApp::ReportError (const char* iMsg)
{
	cerr << "Error: " << iMsg << "!" << endl;
}

void ConsoleApp::ReportError (Error &theError)
{
	ReportError (theError.what());
}


void ConsoleApp::ReportFatalError (const char* iPrompt)
{
	cerr << "Fatal Error: " << iPrompt << "!" << endl;
//	Quit();
}


void ConsoleApp::ReportProgress (const char* iMsg)
// CHANGE (00.11.30): now pipes its output via Report(), which
// means that behaviour can be overridden in a single central
// location.
{
	string theBuffer (iMsg);
	theBuffer.append (" ...");
	Report (theBuffer.c_str());
}

void ConsoleApp::Report (const char* iPrompt)
//: Send an informative line of text to output. 
// CHANGE (00.11.30): this no longer does anything with the text other
// than terminate the line it is on. Previously, it ended sentences
// and so forth, but this proved fiddly. (Sometimes, you're sending a
// whole sentence, sometimes just a fragment.)
{
/*
	string				theMsgStr (iPrompt);
	string				theLineTerminator;
	string::size_type	theLen = theMsgStr.size();
	
	if (theLen == 0)
		theLineTerminator = "";
	else
	{
		char theLastChar = theMsgStr[theLen - 1];
		if ((theLastChar == '.') or (theLastChar == '!') or
			(theLastChar == '\n') or (theLastChar == '\r'))
			theLineTerminator = "";
		else		
			theLineTerminator = ".";
	}
	cout << theMsgStr << theLineTerminator << endl;
*/
	cout << iPrompt << endl;
}

void ConsoleApp::ReportRaw (const char* iPrompt)
//: Send unedited string to output. 
{
	cout << iPrompt;
}

#pragma mark -
// *** QUESTION & ANSWER FUNCTIONS
// Given a few parameters and a prompt string, these fxns pose a question
// to the user, perform some basic bulletproofing and error checking and
// return the answer
// To Do: should really use strstreams and getline to catch answer.

string ConsoleApp::askString (const char* iPromptCstr)
{
	string	thePromptStr (iPromptCstr);
	
	thePromptStr += "?";
	PrintQuestion (&thePromptStr);

	//char	a[256];
	//cin.getline(a, 256);
	//string	theAnswerStr (a);

	string theAnswerStr = getAnswer();
	
	return (theAnswerStr);
}


char ConsoleApp::askChar (const char* iPromptCstr)
// To Do: strip whitespace off front and back of answer.
{
	// build prompt
	string thePromptStr = iPromptCstr;
	thePromptStr += "?";
	
	// ask question & scoop up answer
	PrintQuestion (&thePromptStr);
	string theAnswer = getAnswer();

	// do a primitive check
	if (theAnswer.length() != 1)
	{
		ReportError ("The answer must be a single character");
		return askChar (iPromptCstr);
	}
	else
	{
		return theAnswer[0];
	}
}


double ConsoleApp::askDouble (const char* iPrompt)
{
	return askDoubleHelper (iPrompt, 0, 0, false, false);
}

double ConsoleApp::askDouble
(const char* iPrompt, answerbounds_t iNoLowerBound, double iUpperBound)
{
	iNoLowerBound = iNoLowerBound; // just to shut compiler up
	return askDoubleHelper (iPrompt, 0, iUpperBound, false, true);
}

double ConsoleApp::askDouble
(const char* iPrompt, double iLowerBound, answerbounds_t iNoUpperBound)
{
	iNoUpperBound = iNoUpperBound; // just to shut compiler up
	return askDoubleHelper (iPrompt, iLowerBound, 0, true, false);
}

double ConsoleApp::askDouble
(const char* iPrompt, double iLowerBound, double iUpperBound)
{
	return askDoubleHelper (iPrompt, iLowerBound, iUpperBound, true, true);
}

double ConsoleApp::askDoubleHelper
(const char* iPrompt, double iLower, double iUpper, bool iLowerSet, bool iUpperSet)
{
	// first build the prompt
	stringstream   thePromptStrm;
	thePromptStrm << iPrompt << "?";

	if (iLowerSet or iUpperSet)
	{
		thePromptStrm << " [";
		thePromptStrm.width (3);
		thePromptStrm.precision (4);
		thePromptStrm.setf (std::ios::showpoint);

		if (iLowerSet and iUpperSet)
			thePromptStrm << iLower << " to " << iUpper;
		else if (iLowerSet)
			thePromptStrm << iLower << " or more";
		else if (iUpperSet)
			thePromptStrm << iUpper << " or less";
		else
			assert (false);			
		thePromptStrm << "]";	
	}
	string thePromptStr = thePromptStrm.str();
	
	// ask question, grab answer
	bool   theAnswerIsBad;
	double theAnswer;
	do 
	{
		// get the user input
		PrintQuestion (thePromptStr.c_str());
		string theAnswerStr = getAnswer();

		// if it's not a double
		if ((not isWhole (theAnswerStr)) and (not isFloat (theAnswerStr)))
		{
			ReportError ("That's not a floating-point number or integer");
			continue;
		}
		
		// convert
		theAnswer = toDouble (theAnswerStr);
		
		// if it's outside the bounds
		if (((iLowerSet and (theAnswer < iLower)) or
			 (iUpperSet and (iUpper < theAnswer))))
		{
			ReportError("That's outside the possible range");
			continue;	
		}
		else
			theAnswerIsBad = false;
	}
	while (theAnswerIsBad);

	return theAnswer;
}




/*

double ConsoleApp::askDouble (const char* iPromptCstr)
// prompts user for a floating-point value
// NOTE: accepts integers as allowable and converts them to floats
// CHANGE: (00.2.21) now checks input
{
	// form & print question
	string 		thePromptStr (iPromptCstr);
	thePromptStr += "?";
	PrintQuestion (thePromptStr.c_str());	
	
	// grab & validate answer
	string		theAnswerStr = getAnswer();
	
	if (isWhole (theAnswerStr))
	{
		return double (toLong (theAnswerStr));
	}
	else if (isFloat (theAnswerStr))
	{
		return toDouble (theAnswerStr);
	}
	else
	{
		ReportError ("That's not a floating-point number or integer");
		return (askDouble (iPromptCstr));	
	}
}
*/

double ConsoleApp::askDoubleWithBounds
(const char* iPrompt, double iLower, double iUpper)
{
	double         theAnswer;
	stringstream   thePromptStr;
	char           theBuffer[64];
	
	std::sprintf (theBuffer, "%s? [%.4g to %.4g]", iPrompt, iLower, iUpper);
//	thePromptStr.precision(4);
//	thePromptStr << iPrompt << "? [" << iLower << "to" << iUpper << "]\0";
//	theAnswer = askDouble ((thePromptStr.str()).c_str());	
	theAnswer = askDouble (theBuffer);	
	
	if ((iLower <= theAnswer) and (theAnswer <= iUpper))
	{
		return theAnswer;
	}
	else
	{
		ReportError("That's outside the possible range");
		return (askDoubleWithBounds (iPrompt, iLower, iUpper));	
	}
}

long ConsoleApp::askInteger (const char* iPromptCstr)
// Changed from the above function so as to allow the input of negative
// numbers and get away from magic arguments	
// To Do: should really bullet proof the answer to see it's an int
// Gotcha: if you try and catch the answer with "cin >> theAnswerInt",
// the eoln is left dangling on the input, as it's not part of an integer,
// and hoses the next input.
// To Do: check string to int conversion.
{
	long					theAnswer;
	stringstream 		thePromptStr;

	thePromptStr << iPromptCstr << "?\0";
	// To do: honestly, there has to be a better way ...
	PrintQuestion ((thePromptStr.str()).c_str());	
	string theAnswerStr = getAnswer();
	theAnswer = atol (theAnswerStr.c_str());
	return theAnswer;
}


long ConsoleApp::askIntegerWithMax (const char* iPromptCstr, long iMax)
// See comments on AskIntQuestion above.
{
	long					theAnswer;
	string				theAnswerStr;
	stringstream 		thePromptStr;

	thePromptStr << iPromptCstr << "? [" << iMax << "-]\0";
	PrintQuestion ((thePromptStr.str()).c_str());
	try
	{	
		theAnswerStr = getAnswer ();
		
		if (theAnswerStr == "")
			throw;
			
		theAnswer = atol (theAnswerStr.c_str());
		
		if (theAnswer <= iMax)
			return theAnswer;
		else
			throw;
	}
	catch (...)
	{
		ReportError("That's outside the possible range");
		return (askIntegerWithMax (iPromptCstr, iMax));	
	}
	
	return 0; // just to shut up compiler
}


long ConsoleApp::askIntegerWithMin (const char* iPromptCstr, long iMin)
// See comments on AskIntQuestion above.
{
	long					theAnswer;
	stringstream 		thePromptStr;

	thePromptStr << iPromptCstr << "? [" << iMin << "+]\0";
	PrintQuestion ((thePromptStr.str()).c_str());	
	string theAnswerStr = getAnswer();
	theAnswer = atol (theAnswerStr.c_str());
	
	if (iMin <= theAnswer)
	{
		return theAnswer;
	}
	else
	{
		ReportError("That's below the possible range");
		return (askIntegerWithMin (iPromptCstr, iMin));	
	}
}


long ConsoleApp::askIntegerWithBounds (const char* iPromptCstr, long iMin, long iMax)
// See comments on AskIntQuestion above.
{
	long					theAnswer;
	stringstream 		thePromptStr;

	thePromptStr << iPromptCstr << "? [" << iMin << "-" << iMax << "]\0";
	PrintQuestion ((thePromptStr.str()).c_str());	
	string theAnswerStr = getAnswer();
	theAnswer = atol (theAnswerStr.c_str());
	
	if ((iMin <= theAnswer) and (theAnswer <= iMax) and sbl::isWhole (theAnswerStr))
	{
		return theAnswer;
	}
	else
	{
		ReportError("That's outside the possible range");
		return (askIntegerWithBounds (iPromptCstr, iMin, iMax));	
	}
}


bool ConsoleApp::askYesNo (const char* iPromptCstr)
// Asks the user a question, and returns true for a 'yes' answer and false
// for a 'no'. Accepts "YyNn" and loops on other input.
{
	string	thePromptStr = iPromptCstr;

	thePromptStr += "? [yn]";
	PrintQuestion (&thePromptStr);
	string theAnswer = getAnswer();
	for (ulong i = 0; i < theAnswer.length(); i++)
		theAnswer[i] = char (std::tolower(int(theAnswer[i])));

	if ((theAnswer == "y") || (theAnswer == "yes"))
	{
		return true;
	}
	else if ((theAnswer == "n") || (theAnswer == "no"))
	{
		return false;
	}
	else
	{
		ReportError ("The answer must be \'y\' or \'n\'");
		return askYesNo (iPromptCstr);
	}	
}


bool ConsoleApp::askYesNo (const char* iPromptCstr, bool iCurrentState)
// Asks the user a question, and returns true for a 'yes' answer and false
// for a 'no'. Accepts "YyNn" and loops on other input.
{
	string	thePromptStr = iPromptCstr;

	char theCurrentChoice = (iCurrentState? 'y' : 'n');
	thePromptStr += "? [yn]";
	PrintQuestion (&thePromptStr);
	thePromptStr += ", default \'";
	thePromptStr += theCurrentChoice;

	string theAnswer = getAnswer();
	for (ulong i = 0; i < theAnswer.length(); i++)
		theAnswer[i] = char (std::tolower(int(theAnswer[i])));

	if (theAnswer == "")
	{
		return iCurrentState;
	}
	else if ((theAnswer == "y") || (theAnswer == "yes"))
	{
		return true;
	}
	else if ((theAnswer == "n") || (theAnswer == "no"))
	{
		return false;
	}
	else
	{
		ReportError ("The answer must be \'y\' or \'n\'");
		return askYesNo (iPromptCstr);
	}	
}


bool ConsoleApp::askEitherOr
( const char *iPromptCstr, char iChoice1, char iChoice2)
// Asks the user a question, and returns true for a 'yes' answer and false
// for a 'no'. Accepts "YyNn" and loops on other input.
{
	string	thePromptStr (iPromptCstr),
				theChoiceStr1 (&iChoice1,1),
				theChoiceStr2 (&iChoice2,1);

	thePromptStr += "? [" + theChoiceStr1 + theChoiceStr2 + "]";
	
	PrintQuestion (&thePromptStr);
	string theAnswer = getAnswer();
	
	if (theAnswer == theChoiceStr1)
	{
		return true;
	}
	else if (theAnswer == theChoiceStr2)
	{
		return false;
	}
	else
	{
		string theErrorStr = "The answer must be \'" + theChoiceStr1
			+ "\' or \'" + theChoiceStr2 + "\'";
		ReportError (theErrorStr.c_str());
		return askEitherOr (iPromptCstr, iChoice1, iChoice2);
	}	
}

/*
int ConsoleApp::askChoice (const char *iPromptCstr, char *iChoiceCstr, char iDefChoice)
//: ask the user to choose one of the chars in the string and returns it's index
// A simplification of askMultiChoice().
{
	// Preconditions & argument prep:
	bool theIsCurrChoice = (iDefChoice != '\0');	
	if (theIsCurrChoice)
		assert (isMemberOf (iDefChoice, iChoiceCstr));

	// Main:
	string thePrompt (iPromptCstr);
	string theChoices (iChoiceCstr);
	
	thePrompt += "? [";
	thePrompt += theChoices;
	if (theIsCurrChoice)
	{
		thePrompt += ", currently \'";
		thePrompt += iDefChoice;
		thePrompt += "\']";
	}
	
	PrintQuestion (thePrompt);
	string theRawAnswerStr = getAnswer();
	
	if ((theRawAnswerStr == "") and theIsCurrChoice)
	{
		// if a blank answer, return default if possible
		cout << "Chose \'" << iDefChoice << "\'." << endl;
		string::size_type theStrIndex = theChoices.find (theRawAnswerStr);
		assert (theStrIndex != string::npos);
		return theStrIndex;
	}
	else
	{	
		// we should have a one char answer
		// is the answer 1 char and in string of choices?
		if (theRawAnswerStr.length() == 1)
		{
			string::size_type theStrIndex = theChoices.find (theRawAnswerStr);
			if (theStrIndex != string::npos)
				return theStrIndex;
		}

		// if we get this far, the answer is bad
		string theErrorStr = "The answer must be one of \'";
		theErrorStr += iChoiceCstr;
		theErrorStr += "\'";
		ReportError (theErrorStr.c_str());
		return askMultiChoice (iPromptCstr, iChoiceCstr);
	}
}
*/

int ConsoleApp::askChoice (const char *iPromptCstr, char *iChoiceCstr, int iDefChoice)
//: ask the user to choose one of the chars in the string and returns it's index
// A simplification of askMultiChoice().
{
	// Preconditions & argument prep:
	bool theIsCurrChoice = (iDefChoice != -1);	
	if (theIsCurrChoice)
	{
		assert (0 <= iDefChoice);
		assert (iDefChoice < std::strlen (iChoiceCstr));
	}

	// Main:
	string thePrompt (iPromptCstr);
	string theChoices (iChoiceCstr);
	
	thePrompt += "? [";
	thePrompt += theChoices;
	if (theIsCurrChoice)
	{
		thePrompt += ", default \'";
		thePrompt += iChoiceCstr[iDefChoice];
		thePrompt += "\'";
	}
	thePrompt += "]";

	PrintQuestion (thePrompt);
	string theRawAnswerStr = getAnswer();
	
	if ((theRawAnswerStr == "") and theIsCurrChoice)
	{
		// if a blank answer, return default if possible
		cout << "Chose \'" << iChoiceCstr[iDefChoice] << "\'." << endl;
		string::size_type theStrIndex = theChoices.find (theRawAnswerStr);
		assert (theStrIndex != string::npos);
		return theStrIndex;
	}
	else
	{	
		// we should have a one char answer
		// is the answer 1 char and in string of choices?
		if (theRawAnswerStr.length() == 1)
		{
			string::size_type theStrIndex = theChoices.find (theRawAnswerStr);
			if (theStrIndex != string::npos)
				return theStrIndex;
		}

		// if we get this far, the answer is bad
		string theErrorStr = "The answer must be one of \'";
		theErrorStr += iChoiceCstr;
		theErrorStr += "\'";
		ReportError (theErrorStr.c_str());
		return askChoice (iPromptCstr, iChoiceCstr, iDefChoice);
	}
}


char ConsoleApp::askMultiChoice (const char *iPromptCstr, const char iChoiceStr[])
// Asks the user a question, and allows one of the chars in the choice
// string as an answer.
// To Do: strip whitespace off front and back of answer.
{
	string	thePromptStr = iPromptCstr;
	
	thePromptStr += "? [";
	thePromptStr += iChoiceStr;
	thePromptStr += "]";
	
	PrintQuestion (thePromptStr);
	string theRawAnswerStr = getAnswer();

	// is the answer 1 char and in string of choices?
	bool	theAnswerIsGood = false;
	if (theRawAnswerStr.length() == 1)
	{
		if (isMemberOf (theRawAnswerStr[0], iChoiceStr))
			return theRawAnswerStr[0];
	}
	
	// if we get this far, the answer is bad
	string theErrorStr = "The answer must be one of \'";
	theErrorStr += iChoiceStr;
	theErrorStr += "\'";
	ReportError (theErrorStr.c_str());
	return askMultiChoice (iPromptCstr, iChoiceStr);
}

char ConsoleApp::askMultiChoice (const char *iPromptCstr, char *iChoiceStr, char iCurrChoice)
// A variant of the above function with a default choice
{
	assert (isMemberOf (iCurrChoice, iChoiceStr));
	
	string	thePromptStr = iPromptCstr;
	
	thePromptStr += "? [";
	thePromptStr += iChoiceStr;
	thePromptStr += ", default \'";
	thePromptStr += iCurrChoice;
	thePromptStr += "\']";
	
	PrintQuestion (thePromptStr);
	string theRawAnswerStr = getAnswer();

	// if a blank answer, return default
	if (theRawAnswerStr == "")
		return iCurrChoice;
		
	// is the answer 1 char and in string of choices?
	bool	theAnswerIsGood = false;
	if (theRawAnswerStr.length() == 1)
	{
		if (isMemberOf (theRawAnswerStr[0], iChoiceStr))
			return theRawAnswerStr[0];
	}
	
	// if we get this far, the answer is bad
	string theErrorStr = "The answer must be one of \'";
	theErrorStr += iChoiceStr;
	theErrorStr += "\'";
	ReportError (theErrorStr.c_str());
	return askMultiChoice (iPromptCstr, iChoiceStr);
}


// *** DEPRECATED & DEBUG FUNCTIONS *************************************/
#pragma mark -

void ConsoleApp::AskStringQuestion (const char* iPrompt, char* oAnswer)
{
	UNUSED (oAnswer);
	// return askString (iPrompt, oAnswer);
	askString (iPrompt);
}

string ConsoleApp::AskStringQuestion (const char* iPromptCstr)
{ return askString (iPromptCstr); }

char ConsoleApp::AskCharQuestion (const char* iPromptCstr)
{ return askChar (iPromptCstr); }

float ConsoleApp::AskFloatQuestion (const char* iPromptCstr)
{ return askDouble (iPromptCstr); }

long ConsoleApp::AskIntQuestion (const char* iPromptCstr)
{ return askInteger (iPromptCstr); }

long ConsoleApp::AskIntWithMaxQuestion (const char* iPromptCstr, long iMax)
{ return askIntegerWithMax (iPromptCstr, iMax); }

long ConsoleApp::AskIntWithMinQuestion (const char* iPromptCstr, long iMin)
{ return askIntegerWithMin (iPromptCstr, iMin); }

long ConsoleApp::AskIntWithBoundsQuestion (const char* iPromptCstr, long iMin, long iMax)
{ return askIntegerWithBounds (iPromptCstr, iMin, iMax); }

bool ConsoleApp::AskYesNoQuestion (const char* iPromptCstr)
{ return askYesNo (iPromptCstr); }

bool ConsoleApp::AskYesNoQuestion (const char* iPromptCstr, bool iCurrentState)
{ return askYesNo (iPromptCstr, iCurrentState); }

bool ConsoleApp::AskEitherOrQuestion
(const char *iPromptCstr, char iChoice1, char iChoice2)
{ return askEitherOr (iPromptCstr, iChoice1, iChoice2); }



SBL_NAMESPACE_STOP

// *** END ***************************************************************/

