// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_CONTEXTRUNMODE
#define CHARLS_CONTEXTRUNMODE

struct CContextRunMode 
{
	CContextRunMode(int a, int nRItype, int nReset) :
		A(a),
		N(1),	
		Nn(0),
		_nRItype(nRItype),
		_nReset((BYTE)nReset)
	{
	}

	int A;
	BYTE N;
	BYTE Nn;
	int _nRItype;
	BYTE _nReset;

	CContextRunMode()
	{}


	inlinehint int GetGolomb() const
	{
		UINT Ntest	= N;
		UINT TEMP	= A + (N >> 1) * _nRItype;
		UINT k = 0;
		for(; Ntest < TEMP; k++) 
		{ 
			Ntest <<= 1;
			ASSERT(k <= 32); 
		};
		return k;
	}


	void UpdateVariables(int Errval, UINT EMErrval)
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

	inlinehint int ComputeErrVal(UINT temp, int k)
	{
		bool map = temp & 1;

		UINT errvalabs = (temp + map) / 2;

		if ((k != 0 || (2 * Nn >= N)) == map)
		{
			ASSERT(map == ComputeMap(-int(errvalabs), k));
			return -int(errvalabs);
		}

		ASSERT(map == ComputeMap(errvalabs, k));	
		return errvalabs;
	}


	bool ComputeMap(int Errval, int k) const
	{
		if ((k == 0) && (Errval > 0) && (2 * Nn < N))
			return 1;

		else if ((Errval < 0) && (2 * Nn >= N))
			return 1;		 

		else if ((Errval < 0) && (k != 0))
			return 1;

		return 0;
	}


	inlinehint int ComputeMapNegativeE(int k) const
	{
		return  k != 0 || (2 * Nn >= N );
	}
};

#endif
