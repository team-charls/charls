// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#pragma once



#include "lookuptable.h"

//
// Apply 
//
inlinehint int ApplySign(int i, int sign)
	{ return (sign ^ i) - sign; }									


int CLAMP(int i, int j, int MAXVAL)
{
	if (i > MAXVAL || i < j)
		return j;

	return i;
}

Presets ComputeDefault(int MAXVAL, int NEAR)
{
		Presets preset;

		int FACTOR = (min(MAXVAL, 4095) + 128)/256;
		
		preset.T1 = CLAMP(FACTOR * (BASIC_T1 - 2) + 2 + 3*NEAR, NEAR + 1, MAXVAL);
		preset.T2 = CLAMP(FACTOR * (BASIC_T2 - 3) + 3 + 5*NEAR, preset.T1, MAXVAL);
		preset.T3 = CLAMP(FACTOR * (BASIC_T3 - 4) + 4 + 7*NEAR, preset.T2, MAXVAL);
		preset.MAXVAL = MAXVAL;
		preset.RESET = BASIC_RESET;
		return preset;
}


/* Two alternatives for GetPredictedValue() (second is slightly faster due to reduced branching)

inlinehint int GetPredictedValue(int Ra, int Rb, int Rc)
{
	if (Ra < Rb)
	{
		if (Rc < Ra)
			return Rb;

		if (Rc > Rb)
			return Ra;
	}
	else
	{
		if (Rc < Rb)
			return Ra;

		if (Rc > Ra)
			return Rb;
	}

	return Ra + Rb - Rc;
}

/*/

inlinehint int GetPredictedValue(int Ra, int Rb, int Rc)
{
	// sign trick reduces the number of if statements
	int sgn = BitWiseSign(Rb - Ra);
	
	// is Ra between Rc and Rb? 
	if ((sgn ^ (Rc - Ra)) < 0)
		return Rb;
	
	// is Rb between Rc and Ra?
 	if ((sgn ^ (Rb - Rc)) < 0)
		return Ra;

	// default case, valid if Rc element of [Ra,Rb] 
	return Ra + Rb - Rc;
}

//*/

inlinehint int UnMapErrVal(int mappedError)
{
	//int sign = ~((mappedError & 1) - 1);
	int sign = int(mappedError << 31) >> 31;
	return sign ^ (mappedError >> 1);
}



inlinehint int GetMappedErrVal(int Errval)
{
	int mappedError = (Errval >> 30) ^ (2 * Errval);
	return mappedError;
}



inlinehint  int ComputeContextID(int Q1, int Q2, int Q3)
	{ return (Q1*9 + Q2)*9 + Q3; }


//
//
//
template <class TRAITS, class STRATEGY>
class JlsCodec : public STRATEGY
{
public:
	typedef typename TRAITS::PIXEL PIXEL;
	typedef typename TRAITS::SAMPLE SAMPLE;

public:
	JlsCodec() :
	  _bCompare(0),
	  T1(0),
	  T2(0),
	  T3(0),
	  RUNindex(0),
	  _pquant(0)
	{
		memset(_rghistogramK, 0, sizeof(_rghistogramK));
	}	

	JlsCodec(const TRAITS& inTraits) :
	  traits(inTraits),
	  _bCompare(0),
	  T1(0),
	  T2(0),
	  T3(0),
	  RUNindex(0),
	  _pquant(0)
	{
		memset(_rghistogramK, 0, sizeof(_rghistogramK));
	}	
	

	void SetPresets(const Presets& presets)
	{
		
		Presets presetDefault = ComputeDefault(traits.MAXVAL, traits.NEAR);

		InitParams(presets.T1 != 0 ? presets.T1 : presetDefault.T1,
				   presets.T2 != 0 ? presets.T2 : presetDefault.T2,
				   presets.T3 != 0 ? presets.T3 : presetDefault.T3, 
				   presets.RESET != 0 ? presets.RESET : presetDefault.RESET);


	}

	
 	
	signed char QuantizeGratientOrg(int Di);
	inlinehint int QuantizeGratient(int Di)
		{ 
			ASSERT(QuantizeGratientOrg(Di) == *(_pquant + Di));
			return *(_pquant + Di); 
		}

	void InitQuantizationLUT();
	
	inlinehint UINT DecodeValue(int k, UINT limit, int qbpp);
	inlinehint void EncodeValue(int k, UINT mappederval, UINT limit);
	
	void IncrementRunIndex()
		{ RUNindex = min(31,RUNindex + 1); }
	void DecrementRunIndex()
		{ RUNindex = max(0,RUNindex - 1); }

	int		DecodeRISample(CContextRunMode& ctx);
	Triplet DecodeRIPixel(Triplet Ra, Triplet Rb);
	PIXEL   DecodeRIPixel(int Ra, int Rb);
	int		DecodeRunPixels(PIXEL Ra, PIXEL* ptype, PIXEL* ptypeUnc, int ipixel, int cpixelMac);
	int		DecodeRunMode(PIXEL Ra, PIXEL* ptype, PIXEL* ptypeUnc, const PIXEL* ptypePrev, int ipixel, int cpixelMac);
	
	void	EncodeRISample(CContextRunMode& ctx, int Errval);
	SAMPLE	EncodeRIPixel(int x, int Ra, int Rb);
	Triplet EncodeRIPixel(Triplet x, Triplet Ra, Triplet Rb);
	void	EncodeRunPixels(int RUNcnt, bool bEndofline);
	int		EncodeRunMode(PIXEL* ptype, const PIXEL* ptypeUnc, const PIXEL* ptypePrev, int ipixel, int ctypeRem);

	inlinehint SAMPLE DecodeRegular(int Qs, int pred);
	inlinehint SAMPLE EncodeRegular(int Qs, int x, int pred);

	inlinehint void CheckedAssign(SAMPLE& typeDst, SAMPLE type)
	{
		ASSERT(!_bCompare || traits.IsNear(typeDst, type));
		typeDst = type;
	}

	inlinehint void CheckedAssign(Triplet& typeDst, Triplet type)
	{
		ASSERT(!_bCompare || traits.IsNear(typeDst, type));
		typeDst = type;
	}
	
	void DoLine(SAMPLE* ptype, const SAMPLE* ptypePrev, int csample, SAMPLE* ptypeUnc, DecoderStrategy*);
	void DoLine(SAMPLE* ptype, const SAMPLE* ptypePrev, int csample, const SAMPLE* ptypeUnc, EncoderStrategy*);
	void DoLine(Triplet* ptype, Triplet* ptypePrev, int csample, Triplet* ptypeUnc, DecoderStrategy*);
	void DoLine(Triplet* ptype, Triplet* ptypePrev, int csample, Triplet* ptypeUnc, EncoderStrategy*);
	void DoScan(PIXEL* ptype, Size size, BYTE* pbyteCompressed, int cbyteCompressed);         

public:
	void InitDefault();
	void InitParams(int t1, int t2, int t3, int nReset);

	int  EncodeScan(const void* pvoid, const Size& size, void* pvoidOut, int cbyte, void* pvoidCompare);
	int  DecodeScan(void* pvoidOut, const Size& size, const void* pvoidIn, int cbyte, bool bCompare);

protected:
	TRAITS traits;
	signed char* _pquant;
	std::vector<signed char> _rgquant;
	JlsContext _rgcontext[365];	
	CContextRunMode _contextRunmode[2];
	int RUNindex;
	
	int T3; 
	int T2;
	int T1;	
	
	// debugging
	int _rghistogramK[16];
	bool _bCompare;
};



template<class TRAITS, class STRATEGY>
typename TRAITS::SAMPLE JlsCodec<TRAITS,STRATEGY>::DecodeRegular(int Qs, int pred)
{		
	int sign		= BitWiseSign(Qs);
	JlsContext& ctx	= _rgcontext[ApplySign(Qs, sign)];
	int k			= ctx.GetGolomb();	
	int Px			= traits.CorrectPrediction(pred + ApplySign(ctx.C, sign));    

	int ErrVal;
	const Code& code		= rgtableShared[k].Get(PeekByte());
	if (code.GetLength() != 0)
	{
		Skip(code.GetLength());
		ErrVal = code.GetValue(); 
		ASSERT(abs(ErrVal) < 65535);
	}
	else
	{
		ErrVal = UnMapErrVal(DecodeValue(k, traits.LIMIT, traits.qbpp)); 
		ASSERT(abs(ErrVal) < 65535);
	}	
	ErrVal = ErrVal ^ ((traits.NEAR == 0) ? ctx.GetErrorCorrection(k) : 0);
	ctx.UpdateVariables(ErrVal, traits.NEAR, traits.RESET);			
	return traits.ComputeReconstructedSample(Px, ApplySign(ErrVal, sign)); 
}


template<class TRAITS, class STRATEGY>
typename TRAITS::SAMPLE JlsCodec<TRAITS,STRATEGY>::EncodeRegular(int Qs, int x, int pred)
{
	int sign		= BitWiseSign(Qs);
	JlsContext& ctx	= _rgcontext[ApplySign(Qs, sign)];
	int k			= ctx.GetGolomb();
	int Px			= traits.CorrectPrediction(pred + ApplySign(ctx.C, sign));	
	int ErrVal		= traits.ComputeErrVal(ApplySign(x - Px, sign));

	EncodeValue(k, GetMappedErrVal(ctx.GetErrorCorrection(k | traits.NEAR) ^ ErrVal), traits.LIMIT);	
	ctx.UpdateVariables(ErrVal, traits.NEAR, traits.RESET);
	ASSERT(traits.IsNear(traits.ComputeReconstructedSample(Px, ApplySign(ErrVal, sign)), x));
	return static_cast<TRAITS::SAMPLE>(traits.ComputeReconstructedSample(Px, ApplySign(ErrVal, sign)));
}
						   


inlinehint std::pair<int, UINT> CreateEncodedValue(int k, UINT mappederval)
{
	UINT highbits = mappederval >> k;
	return std::make_pair(highbits + k + 1, (1 << k) | (mappederval & ((1 << k) - 1)));
}


CTable InitTable(int k)
{
	CTable table;
	for (short nerr = 0; ; nerr++)
	{		
		// Q is not used when k != 0
		UINT merrval = GetMappedErrVal(nerr);//, k, -1);
		std::pair<int, UINT> paircode = CreateEncodedValue(k, merrval);
		if (paircode.first > CTable::cbit)
			break;

		Code code = Code( nerr, short(paircode.first) );
		table.AddEntry(BYTE(paircode.second), code);
	}
	
	for (short nerr = -1; ; nerr--)
	{		
		// Q is not used when k != 0
		UINT merrval = GetMappedErrVal(nerr);//, k, -1);
		std::pair<int, UINT> paircode = CreateEncodedValue(k, merrval);
		if (paircode.first > CTable::cbit)
			break;

		Code code = Code(nerr, short(paircode.first));
		table.AddEntry(BYTE(paircode.second), code);
	}

	return table;
}

	
		
template<class TRAITS, class STRATEGY>
inlinehint UINT JlsCodec<TRAITS,STRATEGY>::DecodeValue(int k, UINT limit, int qbpp)
	{
		UINT highbits = ReadHighbits();
		
		if (highbits >= limit - (qbpp + 1))
			return ReadValue(qbpp) + 1;

	 	if (k == 0)
			return highbits;

		return (highbits << k) + ReadValue(k);
	}


	
template<class TRAITS, class STRATEGY>
inlinehint void JlsCodec<TRAITS,STRATEGY>::EncodeValue(int k, UINT mappederval, UINT limit)
{
	UINT highbits = mappederval >> k;

	if (highbits < limit - traits.qbpp - 1)
	{
		if (highbits + 1 > 31)
		{
			AppendToBitStream(0, highbits / 2);
			highbits = highbits - highbits / 2;													
		}
		AppendToBitStream(1, highbits + 1);
		AppendToBitStream((mappederval & ((1 << k) - 1)), k);
		return;
	}

	if (limit - traits.qbpp > 31)
	{
		AppendToBitStream(0, 31);
		AppendToBitStream(1, limit - traits.qbpp - 31);			
	}
	else
	{
		AppendToBitStream(1, limit - traits.qbpp);			
	}
	AppendToBitStream((mappederval - 1) & ((1 << traits.qbpp) - 1), traits.qbpp);
}




#pragma warning (disable: 4127)

template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::InitQuantizationLUT()
{
	if (traits.NEAR == 0 && traits.MAXVAL == (1 << traits.bpp) - 1)
	{
		Presets presets = ComputeDefault(traits.MAXVAL, traits.NEAR);
		if (presets.T1 == T1 && presets.T2 == T2 && presets.T3 == T3)
		{
			if (traits.bpp == 8) 
			{
				_pquant = &rgquant8Ll[rgquant8Ll.size() / 2 ]; 
				return;
			}
			if (traits.bpp == 10) 
			{
				_pquant = &rgquant10Ll[rgquant10Ll.size() / 2 ]; 
				return;
			}			
			if (traits.bpp == 12) 
			{
				_pquant = &rgquant12Ll[rgquant12Ll.size() / 2 ]; 
				return;
			}			
			if (traits.bpp == 16) 
			{
				_pquant = &rgquant16Ll[rgquant16Ll.size() / 2 ]; 
				return;
			}			
		}	
	}

	int RANGE = 1 << traits.bpp;

	_rgquant.resize(RANGE * 2);
	
	_pquant = &_rgquant[RANGE];
	for (int i = -RANGE; i < RANGE; ++i)
	{
		_pquant[i] = QuantizeGratientOrg(i);
	}

}



template<class TRAITS, class STRATEGY>
signed char JlsCodec<TRAITS,STRATEGY>::QuantizeGratientOrg(int Di)
{
	if (Di <= -T3) return  -4;
	if (Di <= -T2) return  -3;
	if (Di <= -T1) return  -2;
	if (Di < -traits.NEAR)  return  -1;
	if (Di <=  traits.NEAR) return   0;
	if (Di < T1)   return   1;
	if (Di < T2)   return   2;
	if (Di < T3)   return   3;
	
	return  4;
}



template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::EncodeRunPixels(int RUNcnt, bool bEndofline)
{
	while (RUNcnt >= (1 << J[RUNindex])) 
	{
		AppendOnesToBitStream(1);
		RUNcnt = RUNcnt - (1 << J[RUNindex]);
		IncrementRunIndex();
	}
	
	if (bEndofline) 
	{
		if (RUNcnt != 0) 
		{
			AppendOnesToBitStream(1);	
		}
	}
	else
	{
		AppendToBitStream(RUNcnt, J[RUNindex] + 1);	// leading 0 + actual remaining length
	}
}



template<class TRAITS, class STRATEGY>
int JlsCodec<TRAITS,STRATEGY>::DecodeRunPixels(PIXEL Ra, PIXEL* ptype, PIXEL* ptypeUnc, int ipixel, int cpixelMac)
{
	while (ReadBit())
	{
		int cpixel = min(1 << J[RUNindex], cpixelMac - ipixel);
		for (int i = 0; i < cpixel; ++i)
		{
			ptype[ipixel] = Ra;
			CheckedAssign(ptypeUnc[ipixel], ptype[ipixel]);
			ipixel++;
			ASSERT(ipixel <= cpixelMac);
		}	

		if (cpixel == (1 << J[RUNindex]))
		{
			IncrementRunIndex();
		}

		if (ipixel == cpixelMac)
 			return ipixel;
	}
	
	// incomplete run 	
	UINT cpixelRun = (J[RUNindex] > 0) ? ReadValue(J[RUNindex]) : 0;
	
	for (UINT i = 0; i < cpixelRun; ++i)
	{
		ptype[ipixel] = Ra;
		CheckedAssign(ptypeUnc[ipixel], ptype[ipixel]);
		ipixel++;
	}

	return ipixel;
}


template<class TRAITS, class STRATEGY>
int JlsCodec<TRAITS,STRATEGY>::DecodeRISample(CContextRunMode& ctx)
{
	int k = ctx.GetGolomb();
	UINT EMErrval = DecodeValue(k, traits.LIMIT - J[RUNindex]-1, traits.qbpp);	
	int Errval = ctx.ComputeErrVal(EMErrval + ctx._nRItype, k);
	ctx.UpdateVariables(Errval, EMErrval);
	return Errval;
}



template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::EncodeRISample(CContextRunMode& ctx, int Errval)
{
	int k			= ctx.GetGolomb();
	bool map		= ctx.ComputeMap(Errval, k);
	UINT EMErrval	= 2 * abs(Errval) - ctx._nRItype - map;	

	ASSERT(Errval == ctx.ComputeErrVal(EMErrval + ctx._nRItype, k));
	EncodeValue(k, EMErrval, traits.LIMIT-J[RUNindex]-1);
	ctx.UpdateVariables(Errval, EMErrval);
}



template<class TRAITS, class STRATEGY>
Triplet JlsCodec<TRAITS,STRATEGY>::DecodeRIPixel(Triplet Ra, Triplet Rb)
{ 
	int Errval1 = DecodeRISample(_contextRunmode[0]);
	int Errval2 = DecodeRISample(_contextRunmode[0]);
	int Errval3 = DecodeRISample(_contextRunmode[0]);

//*

	return Triplet(traits.ComputeReconstructedSample(Rb.v1, Errval1 * Sign(Rb.v1  - Ra.v1)),
				   traits.ComputeReconstructedSample(Rb.v2, Errval2 * Sign(Rb.v2  - Ra.v2)),
				   traits.ComputeReconstructedSample(Rb.v3, Errval3 * Sign(Rb.v3  - Ra.v3)));

/*/
	return Triplet(Rb.v1 + Errval1 * Sign(Rb.v1  - Ra.v1),
				   Rb.v2 + Errval2 * Sign(Rb.v2  - Ra.v2),
				   Rb.v3 + Errval3 * Sign(Rb.v3  - Ra.v3));
//*/
}



template<class TRAITS, class STRATEGY>
Triplet JlsCodec<TRAITS,STRATEGY>::EncodeRIPixel(Triplet x, Triplet Ra, Triplet Rb)
{
	const int RItype		= 0;

	int errval1	= traits.ComputeErrVal(Sign(Rb.v1 - Ra.v1) * (x.v1 - Rb.v1));
	EncodeRISample(_contextRunmode[RItype], errval1);

	int errval2	= traits.ComputeErrVal(Sign(Rb.v2 - Ra.v2) * (x.v2 - Rb.v2));
	EncodeRISample(_contextRunmode[RItype], errval2);

	int errval3	= traits.ComputeErrVal(Sign(Rb.v3 - Ra.v3) * (x.v3 - Rb.v3));
	EncodeRISample(_contextRunmode[RItype], errval3);

	return x;
}



template<class TRAITS, class STRATEGY>
typename TRAITS::PIXEL JlsCodec<TRAITS,STRATEGY>::DecodeRIPixel(int Ra, int Rb)
{
	if (abs(Ra - Rb) <= traits.NEAR)
	{
		int ErrVal		= DecodeRISample(_contextRunmode[1]);
		return static_cast<SAMPLE>(traits.ComputeReconstructedSample(Ra, ErrVal));
	}
	else
	{
	 	int ErrVal		= DecodeRISample(_contextRunmode[0]);
		return static_cast<SAMPLE>(traits.ComputeReconstructedSample(Rb, ErrVal * Sign(Rb - Ra)));
	}
}



template<class TRAITS, class STRATEGY>
typename TRAITS::SAMPLE JlsCodec<TRAITS,STRATEGY>::EncodeRIPixel(int x, int Ra, int Rb)
{
	if (abs(Ra - Rb) <= traits.NEAR)
	{
		int ErrVal	= traits.ComputeErrVal(x - Ra);
		EncodeRISample(_contextRunmode[1], ErrVal);
		return static_cast<TRAITS::SAMPLE>(traits.ComputeReconstructedSample(Ra, ErrVal));
	}
	else
	{
		int ErrVal	= traits.ComputeErrVal((x - Rb) * Sign(Rb - Ra));
		EncodeRISample(_contextRunmode[0], ErrVal);
		return static_cast<TRAITS::SAMPLE>(traits.ComputeReconstructedSample(Rb, ErrVal * Sign(Rb - Ra)));
	}
}




template<class TRAITS, class STRATEGY>
int JlsCodec<TRAITS,STRATEGY>::EncodeRunMode(PIXEL* ptype, const PIXEL* ptypeUnc, const PIXEL* ptypePrev, int ipixel, int ctypeRem)
{
	ptype		+= ipixel;
	ptypeUnc	+= ipixel;
	ptypePrev	+= ipixel;
	ctypeRem	-= ipixel;

	PIXEL Ra =  ptype[-1];
	int RUNcnt = 0;
	
	while (traits.IsNear(ptypeUnc[RUNcnt],Ra)) 
	{
		ptype[RUNcnt] = Ra;
		RUNcnt++;
		
		if (RUNcnt == ctypeRem)
			break;
	}

	EncodeRunPixels(RUNcnt, RUNcnt == ctypeRem);

	if (RUNcnt == ctypeRem)
		return ipixel + RUNcnt;
	
	ptype[RUNcnt] = EncodeRIPixel(ptypeUnc[RUNcnt], Ra, ptypePrev[RUNcnt]);
	ASSERT(traits.IsNear(ptype[RUNcnt], ptypeUnc[RUNcnt]));
	DecrementRunIndex();
	return ipixel + RUNcnt + 1;

}



template<class TRAITS, class STRATEGY>
int JlsCodec<TRAITS,STRATEGY>::DecodeRunMode(PIXEL Ra, PIXEL* ptype, PIXEL* ptypeUnc, const PIXEL* ptypePrev, int ipixel, int cpixelMac)
{
	ipixel = DecodeRunPixels(Ra, ptype, ptypeUnc, ipixel, cpixelMac);

	if (ipixel == cpixelMac)
 		return ipixel;

	// run interruption
	PIXEL Rb = ptypePrev[ipixel];
	ptype[ipixel] =	DecodeRIPixel(Ra, Rb);
	CheckedAssign(ptypeUnc[ipixel], ptype[ipixel]);
	DecrementRunIndex();
	return ipixel + 1;
}



template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::DoLine(typename TRAITS::SAMPLE* ptype, const typename TRAITS::SAMPLE* ptypePrev, int csample, typename TRAITS::SAMPLE* ptypeUnc, DecoderStrategy*)
{
	int ipixel = 0;
	int Rb = ptypePrev[ipixel-1];
	int Rd = ptypePrev[ipixel];
		
	while(ipixel < csample)
	{	
		int Ra = ptype[ipixel -1];
		int Rc = Rb;
		Rb = Rd;
		Rd = ptypePrev[ipixel + 1];

		int Qs = ComputeContextID(QuantizeGratient(Rd - Rb), QuantizeGratient(Rb - Rc), QuantizeGratient(Rc - Ra));
		
		if (Qs == 0)
		{
			ipixel = DecodeRunMode((SAMPLE)Ra, ptype, ptypeUnc, ptypePrev, ipixel, csample);
			Rb = ptypePrev[ipixel-1];
			Rd = ptypePrev[ipixel];	
		}
		else
		{
			ptype[ipixel] = DecodeRegular(Qs, GetPredictedValue(Ra, Rb, Rc));
			CheckedAssign(ptypeUnc[ipixel], ptype[ipixel]);
			ipixel++;
		}		
		
	}
}



template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::DoLine(typename TRAITS::SAMPLE* ptype, const typename TRAITS::SAMPLE* ptypePrev, int csample, const typename TRAITS::SAMPLE* ptypeUnc, EncoderStrategy*)
{
	int ipixel = 0;
	int Rb = ptypePrev[ipixel-1];
	int Rd = ptypePrev[ipixel];
		
	while(ipixel < csample)
	{	
		int Ra = ptype[ipixel -1];
		int Rc = Rb;
		Rb = Rd;
		Rd = ptypePrev[ipixel + 1];
		
		int Qs = ComputeContextID(QuantizeGratient(Rd - Rb), QuantizeGratient(Rb - Rc), QuantizeGratient(Rc - Ra));
					
		if (Qs == 0)
		{
			ipixel = EncodeRunMode(ptype, ptypeUnc, ptypePrev, ipixel, csample);	
			Rb = ptypePrev[ipixel-1];
			Rd = ptypePrev[ipixel];	
		}
		else
		{
			ptype[ipixel] = EncodeRegular(Qs, ptypeUnc[ipixel], GetPredictedValue(Ra, Rb, Rc));	
			ipixel++;
		}		
	}
}



template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::DoLine(Triplet* ptype, Triplet* ptypePrev, int csample, Triplet* ptypeUnc, EncoderStrategy*)
{
	int ipixel = 0;
	while(ipixel < csample)
	{		
		Triplet Ra = ptype[ipixel -1];
		Triplet Rc = ptypePrev[ipixel-1];
		Triplet Rb = ptypePrev[ipixel];
		Triplet Rd = ptypePrev[ipixel + 1];
		
		int Qs1 = ComputeContextID(QuantizeGratient(Rd.v1 - Rb.v1), QuantizeGratient(Rb.v1 - Rc.v1), QuantizeGratient(Rc.v1 - Ra.v1));
		int Qs2 = ComputeContextID(QuantizeGratient(Rd.v2 - Rb.v2), QuantizeGratient(Rb.v2 - Rc.v2), QuantizeGratient(Rc.v2 - Ra.v2));
		int Qs3 = ComputeContextID(QuantizeGratient(Rd.v3 - Rb.v3), QuantizeGratient(Rb.v3 - Rc.v3), QuantizeGratient(Rc.v3 - Ra.v3));
		
		if (Qs1 == 0 && Qs2 == 0 && Qs3 == 0)
		{
			ipixel = EncodeRunMode(ptype, ptypeUnc, ptypePrev, ipixel, csample);
		}
		else
		{
			Triplet Rx;
			Rx.v1 = EncodeRegular(Qs1, ptypeUnc[ipixel].v1, GetPredictedValue(Ra.v1, Rb.v1, Rc.v1));
			Rx.v2 = EncodeRegular(Qs2, ptypeUnc[ipixel].v2, GetPredictedValue(Ra.v2, Rb.v2, Rc.v2));
			Rx.v3 = EncodeRegular(Qs3, ptypeUnc[ipixel].v3, GetPredictedValue(Ra.v3, Rb.v3, Rc.v3));
			ptype[ipixel] = Rx;
			ipixel++;
		}
		
	}
};



template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::DoLine(Triplet* ptype, Triplet* ptypePrev, int csample, Triplet* ptypeUnc, DecoderStrategy*)
{
	int ipixel = 0;
	while(ipixel < csample)
	{		
		Triplet Ra = ptype[ipixel -1];
		Triplet Rc = ptypePrev[ipixel-1];
		Triplet Rb = ptypePrev[ipixel];
		Triplet Rd = ptypePrev[ipixel + 1];
		
		int Qs1 = ComputeContextID(QuantizeGratient(Rd.v1 - Rb.v1), QuantizeGratient(Rb.v1 - Rc.v1), QuantizeGratient(Rc.v1 - Ra.v1));
		int Qs2 = ComputeContextID(QuantizeGratient(Rd.v2 - Rb.v2), QuantizeGratient(Rb.v2 - Rc.v2), QuantizeGratient(Rc.v2 - Ra.v2));
		int Qs3 = ComputeContextID(QuantizeGratient(Rd.v3 - Rb.v3), QuantizeGratient(Rb.v3 - Rc.v3), QuantizeGratient(Rc.v3 - Ra.v3));
		
		if (Qs1 == 0 && Qs2 == 0 && Qs3 == 0)
		{
			ipixel = DecodeRunMode(Ra, ptype, ptypeUnc, ptypePrev, ipixel, csample);
		}
		else
		{
			BYTE value1 = DecodeRegular(Qs1, GetPredictedValue(Ra.v1, Rb.v1, Rc.v1));
			BYTE value2 = DecodeRegular(Qs2, GetPredictedValue(Ra.v2, Rb.v2, Rc.v2));
			BYTE value3 = DecodeRegular(Qs3, GetPredictedValue(Ra.v3, Rb.v3, Rc.v3));

			ptype[ipixel] = Triplet(value1, value2, value3);
			CheckedAssign(ptypeUnc[ipixel], ptype[ipixel]);			
			ipixel++;
		}
		
	}
}



template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::DoScan(PIXEL* ptype, Size size, BYTE* pbyteCompressed, int cbyteCompressed)
{		
	Init(pbyteCompressed, cbyteCompressed);

	std::vector<PIXEL> vectmp;
	vectmp.resize(2 * ( size.cx + 4));

	PIXEL* ptypePrev = &vectmp[1];	
	PIXEL* ptypeCur = &vectmp[size.cx + 4 + 1];	
	
	for (int iline = 0; iline < size.cy; ++iline)
	{
		ptypePrev[size.cx]	  = ptypePrev[size.cx - 1];
		ptypeCur[-1]		  = ptypePrev[0];
		DoLine(ptypeCur, ptypePrev, size.cx, ptype + iline * size.cx, (STRATEGY*)(NULL));
		std::swap(ptypePrev, ptypeCur);		
	}
}



template<class TRAITS, class STRATEGY>
int JlsCodec<TRAITS,STRATEGY>::EncodeScan(const void* pvoid, const Size& size, void* pvoidOut, int cbyte, void* pvoidCompare)
{
	const PIXEL* ptype = static_cast<const PIXEL*>(pvoid);
	BYTE* pbyteCompressed = static_cast<BYTE*>(pvoidOut);
	
	if (pvoidCompare != NULL)
	{
		DecoderStrategy* pdecoder = new JlsCodec<TRAITS,DecoderStrategy>(traits);
		BYTE* pbyteCompare = (BYTE*)pvoidCompare;
		pdecoder->Init(pbyteCompare, cbyte); 
		EncoderStrategy::_qdecoder = pdecoder;
	}

	DoScan(const_cast<PIXEL*>(ptype), size, pbyteCompressed, cbyte);

	Flush();

	return	GetLength();

}



template<class TRAITS, class STRATEGY>
int JlsCodec<TRAITS,STRATEGY>::DecodeScan(void* pvoidOut, const Size& size, const void* pvoidIn, int cbyte, bool bCompare)
{
	PIXEL* ptypeOut			= static_cast<PIXEL*>(pvoidOut);
	BYTE* pbyteCompressed	= const_cast<BYTE*>(static_cast<const BYTE*>(pvoidIn));
	_bCompare = bCompare;

	BYTE rgbyte[20];

	int cbyteRead = 0;
	::memcpy(rgbyte, pbyteCompressed, 4);
	cbyteRead += 4;

	int cbyteScanheader = rgbyte[3] - 2;

	::memcpy(rgbyte, pbyteCompressed, cbyteScanheader);
	cbyteRead += cbyteScanheader;

	DoScan(const_cast<PIXEL*>(ptypeOut), size, pbyteCompressed + cbyteRead, cbyte);

	return GetCurBytePos() - pbyteCompressed;
}






template<class TRAITS, class STRATEGY>
void JlsCodec<TRAITS,STRATEGY>::InitParams(int t1, int t2, int t3, int nReset)
{
	T1 = t1;
	T2 = t2;
	T3 = t3;

	InitQuantizationLUT();

	JlsContext ctxDefault			 = JlsContext(max(2, (traits.RANGE + 32)/64));
	for (UINT Q = 0; Q < sizeof(_rgcontext) / sizeof(_rgcontext[0]); ++Q)
	{
		_rgcontext[Q] = ctxDefault;
	}

	_contextRunmode[0] = CContextRunMode(max(2, (traits.RANGE + 32)/64), 0, nReset);
	_contextRunmode[1] = CContextRunMode(max(2, (traits.RANGE + 32)/64), 1, nReset);
	RUNindex = 0;
}
