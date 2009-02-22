// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#include "stdafx.h"
#include "streams.h"
#include "header.h"
               

#include <math.h>
#include <limits>
#include <vector>
#include <STDIO.H>
#include <iostream>

#include "util.h"
 
#include "decoderstrategy.h"
#include "encoderstrategy.h"
#include "context.h"
#include "contextrunmode.h"
#include "lookuptable.h"

// used to determine how large runs should be encoded at a time. 
const int J[32]			= {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15};

const int BASIC_T1		= 3;
const int BASIC_T2		= 7;
const int BASIC_T3		= 21;

#include "losslesstraits.h"
#include "defaulttraits.h"

CTable rgtable[16];

int f_bTableMaxVal		= -1;
signed char rgquant[65535 + 1 + 65535];

#include "scan.h"


template<class STRATEGY>
STRATEGY* JlsCodecFactory<STRATEGY>::GetCodec(const ScanInfo& _info, const Presets& presets)
{
	STRATEGY* pstrategy = NULL;
	if (presets.RESET != 0 && presets.RESET != BASIC_RESET)
	{
		typename DefaultTraitsT<BYTE,BYTE> traits((1 << _info.cbit) - 1, _info.nnear); 
		traits.MAXVAL = presets.MAXVAL;
		traits.RESET = presets.RESET;
		pstrategy = new JlsCodec<DefaultTraitsT<BYTE, BYTE>, STRATEGY>(traits); 
	}
	else
	{
		pstrategy = GetCodecImpl(_info);
	}

	if (pstrategy == NULL)
		return NULL;

	pstrategy->SetPresets(presets);
	return pstrategy;
}

template<class STRATEGY>
STRATEGY* JlsCodecFactory<STRATEGY>::GetCodecImpl(const ScanInfo& _info)
{
	if (_info.nnear != 0)
	{
		if (_info.cbit == 8 && _info.ilv == ILV_SAMPLE)
		{
			typename DefaultTraitsT<BYTE,Triplet> traits((1 << _info.cbit) - 1, _info.nnear); 
			return new JlsCodec<DefaultTraitsT<BYTE,Triplet>, STRATEGY>(traits); 
		}
		if (_info.cbit == 8)
		{
			typename DefaultTraitsT<BYTE, BYTE> traits((1 << _info.cbit) - 1, _info.nnear); 
			return new JlsCodec<DefaultTraitsT<BYTE, BYTE>, STRATEGY>(traits); 
		}
		else
		{
			typename DefaultTraitsT<USHORT, USHORT> traits((1 << _info.cbit) - 1, _info.nnear); 
			return new JlsCodec<DefaultTraitsT<USHORT, USHORT>, STRATEGY>(traits); 
		}
	}
	if (_info.ilv == ILV_SAMPLE && _info.ccomp == 3 && _info.cbit == 8)
		return new JlsCodec<LosslessTraitsT<Triplet,8>, STRATEGY>();
	
	switch (_info.cbit)
	{
		case  7: return new JlsCodec<LosslessTraitsT<BYTE,   7>, STRATEGY>(); 
		case  8: return new JlsCodec<LosslessTraitsT<BYTE,   8>, STRATEGY>(); 
		case  9: return new JlsCodec<LosslessTraitsT<USHORT, 9>, STRATEGY>(); 
		case 10: return new JlsCodec<LosslessTraitsT<USHORT,10>, STRATEGY>(); 
		case 11: return new JlsCodec<LosslessTraitsT<USHORT,11>, STRATEGY>(); 
		case 12: return new JlsCodec<LosslessTraitsT<USHORT,12>, STRATEGY>();
		case 13: return new JlsCodec<LosslessTraitsT<USHORT,13>, STRATEGY>();
		case 14: return new JlsCodec<LosslessTraitsT<USHORT,14>, STRATEGY>();
		case 15: return new JlsCodec<LosslessTraitsT<USHORT,15>, STRATEGY>();
		case 16: return new JlsCodec<LosslessTraitsT<USHORT,16>, STRATEGY>();
	}

	return NULL;
}


template class JlsCodecFactory<DecoderStrategy>;
template class JlsCodecFactory<EncoderStrategy>;