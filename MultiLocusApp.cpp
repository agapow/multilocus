/**************************************************************************
MultiLocus - calc diversity in allellic data.

Credits:
- By Paul-Michael Agapow, 2003, Dept. Biology, University College
  London, London WC1E 6BT, UK.
- <mail://p.agapow@ucl.ac.uk> <mail://a.burt@ic.ac.uk>
  <http://gershwin.bio.ic.ac.uk>

About:
- The program first ask some questions about the data set, and asks what
  you want to do with it. It can:
  - calculate 5 statistics:
    - the number of different genotypes
    - the genotypic diversity (calculated as 1-Sum[p(i)^2, i], where p(i)
      is the frequency of the i-th genotype).
    - of all n(n-1)/2 possible pairs of loci, how many are "compatible".
      For biallelic loci, "compatible" means that no more than 3 of the 4
      possible genotypes (00, 01, 10, 11) are observed in the data set.
      [Note this will tend to decrease as sample size of isolates
      increases.]
    - the index of association (Maynard Smith et al.)
    - mean standardized covariance (rBar, my formula).
  - search for partitions in the dataset which don't share polymorphisms
  - output the data in PAUP format.
- The input data should be in a file in the same folder as the program,
  with the alleles coded as single letters, digits, or symbols, separated
  by whitespace (space, tab, etc), with unknown as ?. Each row should be
  a different isolate, each column a different site; there should not be
  any site or isolate labels; if there are partitions to be tested or if
  sites are in loci, these must be contiguous. Only variable sites are
  needed for the statistics, only informative sites for the test for
  partitions and the output for PAUP.

Changes:
- 99.1.28: Changed all the variable names to more logical ones.
- 99.1.30: Introduced dynamic arrays.
- 99.3.25: Adapted arrays (TMultiArray) to use templates.
- 99.3.26: Hacked structure around so functions were shorter & "punchier".
- 99.4.21: Placed in the version control system. Hope it works.
- 99.5.10: Handoff. Popgen & Haplo are now completely integrated, albeit
  messily. Original functionality is completely replicated.
- 99.8.1: Now called MultiLocus. Refocus dataflow so calculations are
  done piecemeal rather than in one go. (i.e. make it a fully interactive
  program rather than batch-ish.
- 99.12.10: Nearly there, twisted further in the direction of model-view,
  OO and C++. Version 0.3.

To Do:
- The full GUI version. Use Whisper or YAAF?
- make an openfile() function, such that the data can be in another folder, 
  it uses a dialog box where apropriate and files are saved in a different
  place.  Actually change the names of the saved files to derive from the
  base (data) file.
- credits other information list?
- single datatype for allele data, which has an internal identifier saying
  whether it is haploid or diploid. This would allow us to take away a lot
  "is it diploid?" statements and hide the remaining ones in very low level
  functions like Distance().
- further bulletproof the parsing of the datafile, so if it's described
  wrong or malformed, it won't turn its toes up and die.

**************************************************************************/


// *** INCLUDES
#pragma mark Includes

#include "MultiLocusApp.h"

#include "Sbl.h"

#include "StringUtils.h"
#include "Combination.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <exception>
#include <iomanip>

using std::ifstream;
using std::ofstream;
using std::exception;
using std::cout;
using std::endl;
using std::right;
using std::setw;
using std::ends;
using std::ios;
using std::bad_alloc;
using std::stringstream;
using sbl::StringConcat;
using sbl::FileOpenError;


// *** CONSTANTS & DEFINES
//	kCmd_Open & kCmd_Quit already defined in headers
#pragma mark Constants

enum multiLocusCmd_t
{
	kCmd_DefLink,
	kCmd_DefPop,
	kCmd_Prefs,
	kCmd_Diversity,
	kCmd_PopDiff,
	kCmd_PlotDiv,
	kCmd_Part,
	kCmd_Print,
	kCmd_LinkView,
	kCmd_LinkDef,	
	kCmd_IncludeAll,	
	kCmd_ExcludeIso,		
	kCmd_ExcludeLoci,	
	kCmd_FixMissing,
	kCmd_FreeMissing,
	kCmd_Return
};

const int	kMaxDiffCharacters	= 128;	// to represent alleles
const int 	kMaxFileNameLength	= 31;		// obvious

const char	kPartFileSuffix[] 	= ".part";
const char	kPlotFileSuffix[] 	= ".plot";
const char	kPaupFileSuffix[]		= ".paup";
const char	kStatFileSuffix[]		= ".stats";
const char	kPairFileSuffix[]		= ".pairs";
const char	kThetaFileSuffix[]	= ".theta";


// *** MAIN BODY *********************************************************/
#pragma mark --

// *** LIFECYCLE *********************************************************/

MultiLocusApp::MultiLocusApp ()
{
	// info for about box
	mAppTitle = "MultiLocus";
	mAppVersion = "1.5";
	mAppCredits = "Paul-Michael Agapow (1) & Austin Burt (2)";
	
	mModel = NULL;
	
	mAppInfo.push_back(string("1. <mailto://multiloc@agapow.net>"));
	mAppInfo.push_back(string("2. <mailto://a.burt@ic.ac.uk>"));
	mAppInfo.push_back(string("Dept. Biology, University College London, Silwood Park"));
	mAppInfo.push_back(string(""));
	mAppInfo.push_back(string("<http://www.agapow.net/software/multilocus/>"));
	mAppInfo.push_back(string("June 2009"));
	mAppInfo.push_back(string(""));
	mAppInfo.push_back(string("If you use this software please cite:"));
	mAppInfo.push_back(string("Agapow & Burt (2001) 'Indices of multilocus linkage"));
	mAppInfo.push_back(string("disequilibrium', Molecular Ecology Notes, 1, pp101-102"));	
}

MultiLocusApp::~MultiLocusApp ()
{
	delete mModel;
	mModel = NULL;
}


// *** APPLICATION FLOW **************************************************/
#pragma mark --

void MultiLocusApp::LoadMenu ()
{
	// creat command list 
	mMainCommands.AddCommand (kCmd_Open, 'o', "Open new datafile");	
	mMainCommands.AddCommand (kCmd_DefLink, '1', "Define linkage groups");			
	mMainCommands.AddCommand (kCmd_DefPop, '2', "Define population groups");			
	mMainCommands.AddCommand (kCmd_Prefs, '3', "Set preferences for handling missing data");			
	mMainCommands.AddCommand (kCmd_Diversity, 'g', "Calc. genotypic diversity & linkage disequilibrium");			
	mMainCommands.AddCommand (kCmd_PlotDiv, 'l', "Plot genotypic diversity vs number of loci");			
	mMainCommands.AddCommand (kCmd_PopDiff, 'd', "Population differentiation analysis");			
	mMainCommands.AddCommand (kCmd_Part, 'r', "Test for partitions (haploids only)");			
	mMainCommands.AddCommand (kCmd_Print, 'p', "Print dataset to screen");		
	mMainCommands.AddCommand (kCmd_Quit, 'q', "Quit");	
	mMainCommands.SetConvertShortcut (true);
	SetCmdVisibility (false);
}


// #include "RandomService.h"

// Change: (00.1.26) Shifted a lot of the error trapping up to here for a unified
// mechanism.
void MultiLocusApp::ObeyCmd (cmdId_t iCmdId)
{
	try
	{
		switch (iCmdId)
		{
			case kCmd_Open:
				LoadDataFile ();
				break;

			case kCmd_DefLink:
				DefLinkageGroups ();
				break;

			case kCmd_DefPop:
				DefPopGroups ();
				break;

			case kCmd_Prefs:
				SetPrefs ();
				break;

			case kCmd_Diversity:
				CalcDiversity ();
				break;

			case kCmd_PlotDiv:
				CalcPlotDiv ();
				break;

			case kCmd_PopDiff:
				CalcPopDiffChoice ();
				break;

			case kCmd_Part:
				FindParts ();
				break;

			case kCmd_Print:
				PrintDataSet ();
				break;

			case kCmd_Quit:
				break;

			default:
				assert (false);
				break;
		}
	}
	catch (bad_alloc &theAllocError)
	{
		ReportError ("Memory allocation failed");
		cout << "Increase the memory alloted to this program" << endl;
	}
	catch (Error &theError)
	{
		ReportError (theError);
	}
	catch (exception &theException)
	{
		ReportError (theException.what());
	}
	catch (...)
	{
		ReportError ("Unidentified error");
	}		
}


bool MultiLocusApp::UpdateCmd (cmdId_t iCmdId)
{
	switch (iCmdId)
	{
		case kCmd_Open:
		case kCmd_Quit:
			return true;
			break;

		case kCmd_DefLink:
		case kCmd_DefPop:
		case kCmd_Prefs:
		case kCmd_Diversity:
		case kCmd_PlotDiv:
		case kCmd_PopDiff:
		case kCmd_Print:
			if (mModel != NULL)
				return true;
			else
				return false;
			break;

		case kCmd_Part:
			if ((mModel != NULL) and (mModel->GetPloidy () == kPloidy_Haploid))
				return true;
			else
				return false;
			break;

		default:
			assert (false);
			break;
	}
	
	assert (false);
	return false; // just to keep compiler quiet
}


// *** COMMANDS **********************************************************/
#pragma mark --


// FIND PARTITIONS
// Look for subpops in data. Should only be called for haploids. Nasty &
// computationally expensive.
void MultiLocusApp::FindParts ()
{
	// preconditions
	assert (mModel->GetPloidy () == kPloidy_Haploid);

	if (mModel->GetNumRows() < 4)
	{
		ReportError ("There must be at least 4 isolates to test for partitions");
		return;
	}
	
	try
	{
		cout << endl;
		
		UInt theNumRandomizations;
		if (AskYesNoQuestion ("Find partitions for random datasets"))
			theNumRandomizations = AskIntWithMinQuestion ("Number of randomizations", 1);
		else
			theNumRandomizations = 0;

		// prepare stream for results
		ofstream	thePartFileStream;
		string	thePartFileName (mDataFilePath);
		StringConcat (thePartFileName, kPartFileSuffix, kMaxFileNameLength);
		thePartFileStream.open(thePartFileName.c_str());
		if (not thePartFileStream)
			throw FileOpenError (thePartFileName.c_str());
			
		// do the actual calculations
		UInt theNumParts = mModel->FindPartsLoop (thePartFileStream, theNumRandomizations);

		// tidy up
		thePartFileStream.close();
		
		// print informative closing message
		cout << "Finished. ";
		if (theNumParts == 0)
		{
			cout << "No partitions found. ";
		}
		else
		{
			cout << theNumParts << " partitions found in dataset";
			if (theNumRandomizations != 0)
				cout << " and randomizations";
			cout << ".";
		}
		cout << endl;
		cout << "Results saved in " << thePartFileName << "." << endl;
	}
	catch (...)
	{
		throw;	// Handled by ObeyCommand() now
	}
}


// CALC POPULATION DIFFERENTIATION
// ... or theta bar, a nasty tangled calculation, aka Weir's measure of
// population differention (1996) "Genetic Data Analysis II" p170. This requires
// that there be at least 2 populations but a population may be of size 1.
// The results in such a situation will be technically correct but practically
// meaningless.
// To Do:
// - should partitions of size 1 be allowed?
// - what about unknown characters? 
// - for diploid, count both alleles, right?
void MultiLocusApp::CalcPopDiff ()
{
	if (mModel->mPops.GetNumParts () < 2)
	{
		ReportError ("Differentiation analysis requires 2 or more populations");
		return;
	}

	try
	{
		cout << endl;
		
		// Gather user parameters
		// TO DO: if we only want to analyse some of the populations this
		// is the place to ask.
		UInt theNumRandomizations;
		if (AskYesNoQuestion ("Calculate theta for random datasets"))
			theNumRandomizations = AskIntWithMinQuestion ("Number of randomizations", 1);
		else
			theNumRandomizations = 0;

		// Open stream for results to go into
		// TO DO: what about pops suffix?
		ofstream	theThetaFileStream;
		string	theThetaFileName (mDataFilePath);
		StringConcat (theThetaFileName, kThetaFileSuffix, kMaxFileNameLength);
		theThetaFileStream.open(theThetaFileName.c_str());
		if (not theThetaFileStream)
			throw FileOpenError (theThetaFileName.c_str());
			
		// Do actual calculations
		double theResult = mModel->CalcThetaLoop (theThetaFileStream, theNumRandomizations);
		
		// tidy up
		theThetaFileStream.close();
		cout << "Finished. Original data has a theta of " << theResult << "." << endl;
		cout << "Results saved in " << theThetaFileName << "." << endl;
	}
	catch (...)
	{
		throw;	// Handled by ObeyCommand() now
	}
}


// CALC POPULATION DIFFERENTIATION
// like the above calculation except that it allows a choice of searching sert of pops
void MultiLocusApp::CalcPopDiffChoice ()
{
	UInt theNumPops = mModel->mPops.GetNumParts();
	if (theNumPops < 2)
	{
		ReportError ("Differentiation analysis requires 2 or more populations");
		return;
	}

	try
	{
		cout << endl;
		
		// Gather user parameters - search all pops? randomize? how many?
		bool theSearchAll; 
		if (theNumPops == 2)
		{
			theSearchAll = true; // have to search all of them
		}
		else
		{
			char theSearchChoice = AskMultiChoice ("Analyse all or a subset of populations", "as");
			assert ((theSearchChoice == 'a') or (theSearchChoice == 's'));
			theSearchAll = (theSearchChoice == 'a');
		}
		
		// if searching a subset, gather set of populations
		Combination theSelectedPops;
		if (theSearchAll == false) 
		{
			bool theFinishedSelection = false;
			
			do
			{
				UInt thePopIndex = AskIntWithBoundsQuestion
					("Which population (0 to stop selecting)", 0,theNumPops);
				if (thePopIndex == 0)
				{
					if (theSelectedPops.Size() < 2)
						ReportError ("Differentiation analysis requires 2 or more populations");
					else
						theFinishedSelection = true;
				}
				else
				{
					if (theSelectedPops.Member(thePopIndex - 1))
						ReportError ("That population has already been selected");
					else
						theSelectedPops.Add(thePopIndex - 1);
				}			
			}
			while (theFinishedSelection == false);
			
			theSelectedPops.Sort();
			cout << "Analysing populations:";
			for (int i = 0; i < theSelectedPops.Size(); i++)
				cout << " " << theSelectedPops[i];
			cout << endl;
		}
		else
		{
			cout << "Analysing all populations ..." << endl;
			for (int i = 0; i < theNumPops; i++)
				theSelectedPops.Add(i);
		}
			
		UInt theNumRandomizations;
		if (AskYesNoQuestion ("Calculate theta for random datasets"))
			theNumRandomizations = AskIntWithMinQuestion ("Number of randomizations", 1);
		else
			theNumRandomizations = 0;

		// build file name suffix
		stringstream theFileSuffixStrm;
		theFileSuffixStrm << ".";
		if (theSearchAll)
		{
			theFileSuffixStrm << "all";
		}
		else
		{
			for (int i = 0; i < (theSelectedPops.Size() - 1); i++)
				theFileSuffixStrm << theSelectedPops[i] << ",";
			theFileSuffixStrm << theSelectedPops[theSelectedPops.Size() - 1];
			
		}
		theFileSuffixStrm << ".theta" << ends;
		
		// build whole file name
		string	theThetaFileName (mDataFilePath);
		string	theThetaSuffixStr = theFileSuffixStrm.str();
		StringConcat (theThetaFileName, theThetaSuffixStr.c_str(), kMaxFileNameLength);
		
		// Open stream for results to go into
		ofstream	theThetaFileStream;
		theThetaFileStream.open(theThetaFileName.c_str());
		if (not theThetaFileStream)
			throw FileOpenError (theThetaFileName.c_str());
		
		//DBG_BLOCK
		//(
		//	for (int x = 0; x < theSelectedPops.Size(); x++)
		//		DBG_VAL(theSelectedPops[x]);
		//)
			
		// Do actual calculations
		double theResult = mModel->CalcThetaChoiceLoop (theThetaFileStream, theSelectedPops, theNumRandomizations);
		
		// tidy up
		theThetaFileStream.close();
		cout << "Finished. Original data has a theta of " << theResult << "." << endl;
		cout << "Results saved in " << theThetaFileName << "." << endl;
	}
	catch (...)
	{
		throw;	// Handled by ObeyCommand() now
	}
}


// PLOT DIVERSITY
// Counts the numbers of genotypes in dataset based on an increasing number of loci,
// therefore indicating whether sufficient loci have been sampled.
void MultiLocusApp::CalcPlotDiv ()
{
	try
	{
		cout << endl;
		
		// 1. check for appropriate conditions
		UInt theNumLoci = mModel->GetNumCols();	// how many sites
		if (theNumLoci < 2)
		{
			ReportError ("Diversity plotting requires 2 or more loci");
			return;
		}
		
		// 2. how many samples
		int theNumSamples = AskIntWithBoundsQuestion ("Number of samplings", 10, 1000);

		// 3. init & open files for output
		ReportProgress("Initialising output files");
		ofstream	thePlotFileStream;
		string thePlotFileName = mDataFilePath;
		StringConcat (thePlotFileName, kPlotFileSuffix, kMaxFileNameLength);
		thePlotFileStream.open(thePlotFileName.c_str());
		if (not thePlotFileStream)
			throw FileOpenError (thePlotFileName.c_str());
					
		// 4. actually do the work
		ReportProgress("Sampling diversity");
		mModel->PlotDiv (theNumSamples, thePlotFileStream);
		
		// 5. Tidy up and report finish
		thePlotFileStream.close ();
		cout << "Finished. Results saved in " << thePlotFileName << "." << endl;
	}
	catch (...)
	{
		throw;	// Handled by ObeyCommand() now
	}
}


void MultiLocusApp::CalcDiversity ()
{
	try
	{
		cout << endl;
		// 1. ask for parameters
		bool	theCalcPairwise = AskYesNoQuestion ("Calculate pairwise statistics");
		bool	theRandomDatasets = AskYesNoQuestion ("Generate & analyse randomized datasets");
		int	theNumRandomizations;
		if (theRandomDatasets)
			theNumRandomizations = AskIntWithMinQuestion ("Number of randomizations", 1);
		else
			theNumRandomizations = 0;
		bool	theSaveAsPaup = AskYesNoQuestion ("Save dataset to PAUP file");
		cout << endl;

		// 2. init & open files for output
		ReportProgress("Initialising output files");
		
		// create appropraiet stem name for files
		string theBaseName (mDataFilePath);
		sbl::stripExt (theBaseName);
		
		// set up stats file & stream
		ofstream	theStatsFileStream;
		string theStatsFileName = theBaseName;
		StringConcat (theStatsFileName, kStatFileSuffix, kMaxFileNameLength);
		theStatsFileStream.open(theStatsFileName.c_str());
		if (not theStatsFileStream)
			throw FileOpenError (theStatsFileName.c_str());
			
		// set up paup file & stream
		ofstream	thePaupFileStream;
		string thePaupFileName = theBaseName;
		if (theSaveAsPaup)
		{
			thePaupFileName = theBaseName;
			StringConcat (thePaupFileName, kPaupFileSuffix, kMaxFileNameLength);
			thePaupFileStream.open(thePaupFileName.c_str());
			if (not thePaupFileStream)
				throw FileOpenError (thePaupFileName.c_str());
		}
		
		// set up pairs file & stream
		ofstream thePairsFileStream;
		string thePairsFileName;
		if (theCalcPairwise)
		{
			thePairsFileName = theBaseName;
			StringConcat (thePairsFileName, kPairFileSuffix, kMaxFileNameLength);
			thePairsFileStream.open (thePairsFileName.c_str());
			if (not thePairsFileStream)
				throw FileOpenError (thePairsFileName.c_str());
		}
		
		// 3. actually do the calculations
		ReportProgress("Calculating stats");
		mModel->CalcDiversity (theCalcPairwise, theNumRandomizations, theSaveAsPaup,
		theStatsFileStream, thePairsFileStream, thePaupFileStream);
		
		// 4. Tidy up and report conclusion
		thePairsFileStream.close ();
		theStatsFileStream.close ();
		thePaupFileStream.close ();
		cout << "Finished. Results saved in " << theStatsFileName;
		if (theSaveAsPaup)
		{
			cout << ((theCalcPairwise) ? ", " : " and ");
			cout << thePaupFileName;
		}
		if (theCalcPairwise)
			cout << " and " << thePairsFileName;
		cout << "." << endl;
	}
	catch (FormatError& theError)
	{
		// this one will always result from a "too many diplotypes for
		// PAUP" error
		ReportError(theError);
		cout << "Recode your data so there are fewer diplotypes" << endl;
	}
	catch (...)
	{
		throw;	// Handled by ObeyCommand() now
	}
}


void MultiLocusApp::LoadDataFile ()
{
	cout << endl;
	mDataFilePath = AskStringQuestion ("What is the name of the input data file");
		
	if (mDataFilePath != "")
	{
		try
		{
			if (mModel != NULL)
			{
				delete mModel; 				// if data already loaded delete it
				mModel = NULL;
			}
			mModel = new MultiLocusModel;	// create new one
			
			ifstream theDataFileStrm  (mDataFilePath.c_str());
			if (theDataFileStrm)
			{
				mModel->ParseInput (theDataFileStrm, mDataFilePath.c_str());
				theDataFileStrm.close ();
			}
			else
				throw FileOpenError();
			
			cout << "Data loaded successfully (";
			cout << (mModel->GetPloidy() == kPloidy_Haploid ? "haploid" : "diploid");
			cout << ", " << mModel->GetNumCols() << " loci, " << mModel->GetNumRows()
				<< " isolates)" << endl;
		}
		catch (...)
		{
			delete mModel;
			mModel = NULL;
			ReportError ("The datafile cannot be loaded");
			throw;			// Handled by ObeyCommand() now
		}
	}
}


// Show the user the array.
// Adjusted for use with diploid data
void MultiLocusApp::PrintDataSet ()
{
	cout << endl;
	mModel->PrintDataSet (cout);
	mModel->PrintSettings (cout);
	// cout << endl;	// looks better without extra line of spacing
}


void MultiLocusApp::DefLinkageGroups ()
{
	UInt theNumSites = mModel->GetNumCols();	// how many sites
	UInt theNumSitesFree = theNumSites;	// how many sites yet to be committed
	UInt theMaxAvailSites;					// max to be committed to current group
		
	cout << endl;	
	if (theNumSites == 1)
	{
		cout << "There is a single loci in a single linkage group" << endl;
		return;
	}
	UInt theNumGroups = AskIntWithBoundsQuestion	// num of linkage groups
		("Number of linkage groups", 2, theNumSites);
		
	if (theNumGroups == theNumSites)
	{
		// if every isolate in a different group
		cout << "All loci are in separate groups" << endl;
		mModel->mLinkages.SplitAll ();
	}
	else if (theNumGroups == 1)
	{
		cout << "All loci are a single linkage group" << endl;
		mModel->mLinkages.MergeAll ();
	}
	else
	{	
		// !! find out how many sites are in each group 1 to (N-1). The number
		// in the last partition should be the remainder. Every partition has
		// from 1 to (the number of isolates less the number already
		// allocated and less the number of partitions left to do).
		vector <int>	theParts;
		for (int theGroupIndex = 0; theGroupIndex < (theNumGroups - 1); theGroupIndex++)
		{	
			// build prompt
			// establish range of possible figures
			stringstream thePromptStr;
			thePromptStr << "Number of loci in group #" << theGroupIndex + 1 << ends;
			theMaxAvailSites = theNumSitesFree - (theNumGroups - theGroupIndex - 1);
			UInt theNumInCurrLoci;
			if (theMaxAvailSites == 1)
			{
				cout << "Group (#" << theGroupIndex+1 << ") contains 1 loci." << endl;
				theNumInCurrLoci = 1;
			}
			else
			{
				theNumInCurrLoci = AskIntWithBoundsQuestion ((thePromptStr.str()).c_str(),
					1, theMaxAvailSites);
			}
			theNumSitesFree -= theNumInCurrLoci;
			theParts.push_back (theNumInCurrLoci);
		}
		
		// calculate number for last partition
		theParts.push_back (theNumSitesFree);
		cout << "The final group (#" << theNumGroups << ") contains " <<
			theNumSitesFree;
		if (1 < theNumSitesFree)
			cout << " loci.";
		else
			cout << " locus.";
		cout << endl;
		mModel->mLinkages.SetParts (theParts);
	}
}


void MultiLocusApp::DefPopGroups ()
{
	UInt theNumIso = mModel->GetNumRows();	// how many isolates
	UInt theNumIsoFree = theNumIso;		// how many isos yet to be committed
	UInt theMaxAvailIso;					// max to be committed to current pop

	cout << endl;
	if (theNumIso == 1)
	{
		cout << "There is a single isolate in a single population" << endl;
		return;
	}
	UInt theNumGroups = AskIntWithBoundsQuestion	// num of populations
		("Number of populations", 1, theNumIso);
		
	if (theNumGroups == theNumIso)
	{
		// if every isolate in a different group
		cout << "All isolates are in separate populations" << endl;
		mModel->mPops.SplitAll ();
	}
	else if (theNumGroups == 1)
	{
		cout << "All isolates are in a single population" << endl;
		mModel->mPops.MergeAll ();
	}
	else
	{	
		// !! find out how many isos are in each pop 1 to (N-1). The number
		// in the last partition should be the remainder. Every pop has
		// from 1 to (the number of isolates less the number already
		// allocated and less the number of pops left to do).
		vector <int>	thePops;
		for (int theGroupIndex = 0; theGroupIndex < (theNumGroups - 1); theGroupIndex++)
		{	
			// build prompt
			// establish range of possible figures
			stringstream thePromptStr;
			thePromptStr << "Number of isolates in pop #" << theGroupIndex + 1 << ends;
			theMaxAvailIso = theNumIsoFree - (theNumGroups - theGroupIndex - 1);
			UInt theNumInCurrPop;
			if (theMaxAvailIso == 1)
			{
				cout << "Population (#" << theGroupIndex+1 << ") contains 1 isolate." << endl;
				theNumInCurrPop = 1;
			}
			else
			{
				theNumInCurrPop = AskIntWithBoundsQuestion ((thePromptStr.str()).c_str(),
					1, theMaxAvailIso);
			}
			theNumIsoFree -= theNumInCurrPop;
			thePops.push_back (theNumInCurrPop);
		}
		
		// calculate number for last partition
		thePops.push_back (theNumIsoFree);
		cout << "The final population (#" << theNumGroups << ") contains " <<
			theNumIsoFree << " isolate";
		if (1 < theNumIsoFree)
			cout << "s.";
		cout << endl;
		mModel->mPops.SetParts (thePops);
	}
}


void MultiLocusApp::SetPrefs ()
{
	cout << endl;
	cout << "Note: changing the data included will reset linkage groups & populations.";
	
	CommandMgr	theInclusionCmds;	
	theInclusionCmds.AddCommand (kCmd_IncludeAll, 'a', "Include all data");	
	theInclusionCmds.AddCommand (kCmd_ExcludeIso, 'i', "Exclude isolates with missing data");			
	theInclusionCmds.AddCommand (kCmd_ExcludeLoci, 'l', "Exclude loci with missing data");			
	theInclusionCmds.AddCommand (kCmd_FixMissing, 'f', "Fix missing data during randomizations");			
	theInclusionCmds.AddCommand (kCmd_FreeMissing, 'a', "Allow missing data to move during randomizations");			
	theInclusionCmds.AddCommand (kCmd_Return, 'r', "Return to main menu");			
	theInclusionCmds.SetConvertShortcut (true);
	
	cmdId_t	theUserCmd;
	do 
	{
		try
		{
			// which commands should be on?
			theInclusionCmds.SetCommandActive(false);
			// if either isolates or loci are out, include all is in
			if (mModel->mExcludeLoci or mModel->mExcludeIso)
				theInclusionCmds.SetCommandActive (kCmd_IncludeAll, true);
			if (not mModel->mExcludeIso)
				theInclusionCmds.SetCommandActive (kCmd_ExcludeIso, true);
			if (not mModel->mExcludeLoci)
				theInclusionCmds.SetCommandActive (kCmd_ExcludeLoci, true);
			// free or fix isolates?
			if (mModel->mDoMissingShuffle == kMissing_Fixed)
			{
				theInclusionCmds.SetCommandActive (kCmd_FreeMissing, true);
			}
			else
			{
				assert (mModel->mDoMissingShuffle == kMissing_Free);
				theInclusionCmds.SetCommandActive (kCmd_FixMissing, true);
			}
			// always true	
			theInclusionCmds.SetCommandActive(kCmd_Return, true);
				
			theUserCmd = AskUserCommand (&theInclusionCmds);
			switch (theUserCmd)
			{
				case kCmd_IncludeAll:
					mModel->IncludeAllData ();
					cout << endl << "All data is now included." << endl;
					break;

				case kCmd_ExcludeIso:
					if (mModel->ExcludeMissingIso ())
						cout << endl << "All isolates with missing data are now excluded." << endl;
					else
						cout << endl << "Can't exclude isolates because the dataset would be empty." << endl;
					break;

				case kCmd_ExcludeLoci:
					if (mModel->ExcludeMissingLoci ())
						cout << endl << "All loci with missing data are now excluded." << endl;
					else
						cout << endl << "Can't exclude loci because the dataset would be empty." << endl;
					break;

				case kCmd_FreeMissing:
					mModel->mDoMissingShuffle = kMissing_Free;
					cout << endl << "Missing data will now move during randomization." << endl;
					break;

				case kCmd_FixMissing:
					mModel->mDoMissingShuffle = kMissing_Fixed;
					cout << endl << "Missing data will now be fixed in position during randomization." << endl;
					break;

				case kCmd_Return:
					// do nothing
					break;

				default:
					assert (false);	
			}
		}
		catch (Error &theError)
		{
			ReportError (theError);
		}
		catch (...)
		{
			ReportError ("Unidentified error");
		}
	}
	while (theUserCmd != kCmd_Return);

}
// *** DEPRECIATED FUNCTIONS *********************************************/

// *** END ***************************************************************/

