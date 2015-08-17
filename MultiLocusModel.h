/**************************************************************************
MultiLocusModel.h - abstracted domain model fro MultiLocus

Credits:
- From SIBIL, the Silwood Biocomputing Library.
- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,
  London WC1E 6BT, UNITED KINGDOM.
- <mail://p.agapow@ucl.ac.uk> <http://www.agapow.net>

About:
- Ideally this should be completely divorcable from any interface or
  application loop so as to be portable to other architectures.

**************************************************************************/

#ifndef MULTILOCUSMODEL_H
#define MULTILOCUSMODEL_H


// *** INCLUDES

#include "Sbl.h"

#include "Partition.h"
#include "RandomService.h"
#include "StreamScanner.h"
//#include "Combination.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <map>

using std::vector;
using std::ifstream;
using std::ofstream;
using std::ostream;
using std::string;
using std::pair;
using std::map;

using namespace sbl;

class Combination;


// *** CONSTANTS & DEFINES

#define MATRIX(X)		vector< vector<X> >

enum ploidy_t				// type-safety for haplo vs diplo Q.
{
	kPloidy_None,
	kPloidy_Haploid,
	kPloidy_Diploid
};

enum distance_t			// aloow ambiguity in distances or not?
{
	kDistance_Strict,
	kDistance_Relaxed
};

// types for allele states, to divorce interface from implementation

typedef string tAllele;								// haploid							

struct tAllelePair									// diploid
{
	string alleleA;
	string alleleB;
	char transNumDTypes;
};

// types for linkage groups

typedef pair<UInt,UInt>	linkageGroup_t;


// type for missing allele movement/behaviour under shuffling
enum missing_t				
{
	kMissing_Fixed,
	kMissing_Free
};


// *** CLASS DECLARATION *************************************************/

class MultiLocusModel
{
public:
	// Lifecycle
	MultiLocusModel		();
	~MultiLocusModel		();
				
	// Access
	ploidy_t		GetPloidy		();
	const char*	GetDataString	(UInt iRowIndex, UInt iColIndex);
	UInt			GetNumRows		();
	UInt			GetNumCols		();

	bool			IsDataRankable ();

	// Manipulation
	void			ParseInput				(ifstream& ioInputFile, const char* iDataFileName);
	void			BackupDataset			();
	void			RestoreDataset			();
	void			ShuffleDataset			();

	void 			BackupOriginal			();
	void			RestoreOriginal		();
	void 			BackupWorkingData		();
	void			RestoreWorkingData	();
	void			DetermineDimensions	();

	void			IncludeAllData			();
	bool			ExcludeMissingIso		();
	bool			ExcludeMissingLoci	();

	bool			IsMissing				(UInt iRowIndex, UInt iColIndex);
	bool			IsMissing				(const char* ikSymbol);
	bool			IsMissing 				(const string& ikSymbol);
	bool			IsColMissing			(UInt iColIndex);
	bool			IsColMissing			();
	bool			IsRowMissing			(UInt iRowIndex);
	bool			IsRowMissing			();

	void			DeleteCol				(UInt iColIndex);
	void			DeleteRow				(UInt iRowIndex);

	void			PrintDataSet			(ostream& ioOutStream);
	
	// Calculations
	void	InitPaupFile				(ofstream& iPaupStream);
	void	InitStatsFile 				(ofstream& iStatsStream);
	void	InitThetaFile				(ofstream& iThetaStream);
	void	InitPairsFile 				(ofstream& iPairsStream);
	void	InitPlotFile				(ofstream& ioPlotStream);
	void	InitFileWithSettings		(ofstream& iFileStream);
	void	PrintSettings				(ostream& oSettingsStream);

	void	PlotDiv 						(int iNumSamples, ofstream& ioPlotStream);
	void	CalcDiversity				(bool iDoPairwiseStats, int iNumRandomizations,
											bool iDoPaupOutput, ofstream& iStatsStream,
											ofstream& iPairsStream, ofstream& iPaupStream );
	void	OutputAsPaup				(ofstream& iPaupStream);
	
	void	PrepRBarSCalc				();
	void	CalcIndexAssocRBarD		(double& oIndexAssoc, double& oRBarD);
	void	CalcPorpCompat				(double& iPorpCompat);
	void	CalcNumDiff					(double& iDiversity, int& iNumDiff, int& iMaxFreq);
	void	CalcRBarS					(double& iRBarS);
	void	CalcVarDistances			();
	// void	CalcVarSimilarityCoeff 	();
	void	CalcPairwiseStats			(ofstream& oOutStream,
												vector<double>& oPairwiseRVals,
												vector<double>& oPVals, bool iIsOriginalData);

	UInt	FindPartsLoop	(ofstream& ioPartStream, UInt iNumRandomizations);

	double	CalcThetaLoop 			(ofstream& ioResults, UInt iNumRandomizations);
	double	CalcThetaChoiceLoop	(ofstream& ioResults, Combination& theSelectedPops,
												UInt iNumRandomizations);
	
	// dimensions of data
	UInt				mNumPairsIsolates;	// calculated
	UInt				mNumPairsSites;		// calculated
	
	bool				mIsDataRankable;
	
	// double			mExpVar;
	vector<double>	mVarDist;
	double			mSumVarDist;
	double			mMaxSumCov1;
	double			mSumVar2, mMaxSumCov2;

	vector<tAllelePair>		mDiploTrans;	// array of unique dtypes
	vector< vector<int> >	mStepMatrix;	// distances between diplotypes
		
	// for partitioning of loci & isolates	
	Partition					mLinkages;
	Partition					mPops;
	
	// for data inclusion & behaviour
	bool							mExcludeLoci;
	bool							mExcludeIso;
	missing_t					mDoMissingShuffle;
	
private:
	// internals
	ploidy_t 					mPloidy;
	
	// the various representations of the data
	MATRIX(tAllele)*			mHaploData;				// the current data
	MATRIX(tAllelePair)*		mDiploData;
	
	MATRIX(tAllele)*			mBackupHaploData;		// the saved manipulated data
	MATRIX(tAllelePair)*		mBackupDiploData;

	MATRIX(tAllele)*			mOriginalHaploData;	// the original data
	MATRIX(tAllelePair)*		mOriginalDiploData;
	
	RandomService				mRng;
	
	string						mDataName;

	// internals for searching of partition
	UInt	FindParts		(ofstream& ioPartStream, UInt iRepNum);
	void	OutputPart		(ofstream& ioPartStream, vector<int>& iPart);
	bool	TestPart			(Combination& iPart1, Combination& iPart2);
	bool	TestPart			(vector<int>& iPart1, vector<int>& iPart2);
	
	// internals for calculating theta
	void		CalcTheta			(double& oTheta);
	void		ShufflePops			(Combination& iSelectedPops);
	void		CalcThetaChoice	(double& oTheta, Combination& iSelectedPops);

	// internals for shuffling of data
	void	ShufflePop 		(int iFrom, int iTo);
	void	ShuffleBlock	(int iFromAllele, int iToAllele, int iFromPop,
								int iToPop);
	void	SwapAllele		(int iAllelePosn, int iFromPop, int iToPop);
	
	// internals for reading in data from stream
	void	ParseHaploidInput 	(StreamScanner& iScanner, UInt iNumCols);
	void	ParseDiploidInput 	(StreamScanner& iScanner, UInt iNumCols);
	bool	IsValidAllele		 	(string& iAlleleStr);
	
	// primitives
	int	Distance 			(tAllele iChar1, tAllele iChar2);
	int	Distance 			(tAllelePair iChar1, tAllelePair iChar2);
	int	StrictDistance 	(tAllele iChar1, tAllele iChar2);
	int	StrictDistance 	(tAllelePair iChar1, tAllelePair iChar2);
	UInt	StrictDistance 	(UInt iFromRowIndex, UInt iToRowIndex);
	UInt	StrictDistance 	(UInt iFromIso, UInt iToIso, UInt iTargetLoci);
	
	char	GenerateDTypeSymbol 		(UInt iDipTypeIndex);
	void	InitDTypeTranslations	();
	
	void		CalcIsoDistArray		(vector<int>& oDistArray,
											distance_t iIsDistStrict = kDistance_Relaxed);
	UInt		CountGtypesFromDist	(vector<int>& oDistArray);
	double	CalcDivFromDist 		(vector<int>& oDistArray);
	void		CountFreqsFromDist	(vector<int>& oDistArray, vector<int>& oGtypeFreq);

	bool	IsAlleleRankable		(string& iAlleleStr);
	bool	IsHomozygous			(UInt iRowIndex, UInt iColIndex);
};


#endif
// *** END ***************************************************************/



