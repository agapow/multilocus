/**************************************************************************RandomService.h - encapsulated pseudo-random number generatorCredits:- From SIBIL, the Silwood Biocomputing Library.- By Paul-Michael Agapow, 2003, Dept. Biology, University College London,  London WC1E 6BT, UNITED KINGDOM.- <mail://p.agapow@ucl.ac.uk> <http://www.agapow.net>About:- A psuedo-random number generator, implemented as a linear congruential  generator, that can serve as the base class for a family of RNG services.**************************************************************************/#ifndef RANDOMSERVICE_H#define RANDOMSERVICE_H// *** INCLUDES#include "Sbl.h"#include <cmath>SBL_NAMESPACE_START// *** CONSTANTS & DEFINES// *** CLASS DECLARATION *************************************************/class RandomService{public:	// Lifecycle	RandomService				();	RandomService				( long iSeed );					// Access	virtual void	SetSeed 	( long iSeed ); // can be overidden		// RNGs	// Uniform distribution		double	UniformFloat	();	double	UniformFloat	( double iCeiling );	double	UniformFloat	( double iFloor, double iCeiling );	long		UniformWhole	( long iNumChoices );	long		UniformWhole	( long iFloor, long iCeiling );	// Normal distribution		double	NormalFloat		();	double	NormalFloat		( double iCeiling );	double	NormalFloat		( double iFloor, double iCeiling );	long		NormalWhole		( long iNumChoices );	long		NormalWhole		( long iFloor, long iCeiling );	double   gaussian (double iMean, double iStdDev)	{        double u, v;        do        {            u = Generate();            v = Generate();            }        while (v == 0);		// double theSigma = std::sqrt (iVariance);			        return iMean + iStdDev * std::sqrt(-2*std::log(v)) * std::cos(3.1415 * (2*u - 1));/*		double x, y, r2;		do		{			// choose x,y in uniform square (-1,-1) to (+1,+1)			x = -1.0 + 2.0 * Generate ();			y = -1.0 + 2.0 * Generate ();			// see if it is in the unit circle			r2 = x * x + y * y;		}		while ((r2 > 1.0) or (r2 == 0));		// Box-Muller transform		return iMean + (iSigma * y * std::sqrt (-2.0 * std::log (r2) / r2));*/	}	// Depreciated & Debug	void		Test				();	double	Uniform			();	double	Uniform			( double iCeiling );	double	Uniform			( double iFloor, double iCeiling );		long		Uniform			( long iNumChoices );	long		Uniform			( long iFloor, long iCeiling );private:	// Members	long		mSeed;		// Internals, can be overridden in derived classes	virtual void	InitSeed 	();	virtual double	Generate		(); };SBL_NAMESPACE_STOP#endif// *** END ***************************************************************/