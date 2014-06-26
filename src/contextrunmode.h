// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_CONTEXTRUNMODE
#define CHARLS_CONTEXTRUNMODE

#include <cstdint>

// Implements statistical modelling for the run mode context.
// Computes model dependent parameters like the golomb code lengths
struct CContextRunMode
{
	// Note: members are sorted based on their size.
	LONG A;
	LONG _nRItype;
	uint8_t _nReset;
	uint8_t N;
	uint8_t Nn;

	CContextRunMode() :
		A(),
		_nRItype(),
		_nReset(),
		N(),
		Nn()
	{
	}


	CContextRunMode(LONG a, LONG nRItype, LONG nReset) :
		A(a),
		_nRItype(nRItype),
		_nReset(static_cast<uint8_t>(nReset)),
		N(1),
		Nn(0)
	{
	}


	inlinehint LONG GetGolomb() const
	{
		LONG Ntest = N;
		LONG TEMP = A + (N >> 1) * _nRItype;
		LONG k = 0;
		for(; Ntest < TEMP; k++) 
		{
			Ntest <<= 1;
			ASSERT(k <= 32);
		}
		return k;
	}


	void UpdateVariables(LONG Errval, LONG EMErrval)
	{
		if (Errval < 0)
		{
			Nn = Nn + 1;
		}
		A = A + ((EMErrval + 1 - _nRItype) >> 1);
		if (N == _nReset) 
		{
			A = A >> 1;
			N = N >> 1;
			Nn = Nn >> 1;
		}
		N = N + 1;
	}


	inlinehint LONG ComputeErrVal(LONG temp, LONG k) const
	{
		bool map = temp & 1;

		LONG errvalabs = (temp + LONG(map)) / 2;

		if ((k != 0 || (2 * Nn >= N)) == map)
		{
			ASSERT(map == ComputeMap(-errvalabs, k));
			return -errvalabs;
		}

		ASSERT(map == ComputeMap(errvalabs, k));
		return errvalabs;
	}


	bool ComputeMap(LONG Errval, LONG k) const
	{
		if ((k == 0) && (Errval > 0) && (2 * Nn < N))
			return true;

		if ((Errval < 0) && (2 * Nn >= N))
			return true;

		if ((Errval < 0) && (k != 0))
			return true;

		return false;
	}


	inlinehint bool ComputeMapNegativeE(LONG k) const
	{
		return  k != 0 || (2 * Nn >= N );
	}
};

#endif
