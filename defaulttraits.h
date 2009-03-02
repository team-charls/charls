// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once

// Default traits that support all JPEG LS paramaters.

template <class sample, class pixel>
struct DefaultTraitsT 
{
public:
//	enum { ccomponent = 1, };
	typedef sample SAMPLE;
	typedef pixel PIXEL;
	
	int MAXVAL;
	int RANGE;
	int NEAR;
	int LIMIT;
	int qbpp;
	int bpp;
	int RESET;

	DefaultTraitsT(const DefaultTraitsT& src) :
		MAXVAL(src.MAXVAL),
		RANGE(src.RANGE),
		NEAR(src.NEAR),
		qbpp(src.qbpp),
		bpp(src.bpp),
		LIMIT(src.LIMIT),
		RESET(src.RESET)
	{
	}

	DefaultTraitsT(int max, int jls_near)
	{
		NEAR   = jls_near;
		MAXVAL = max;
		RANGE  = (MAXVAL + 2 * NEAR )/(2 * NEAR + 1) + 1;
		bpp = log2(max);	
		LIMIT = 2 * (bpp + max(8,bpp));
		qbpp = log2(RANGE);
		RESET = BASIC_RESET;
	}

	
	inlinehint int ComputeErrVal(int e) const
	{
		int q = Quantize(e);
		return ModRange(q);
	}
	
	inlinehint SAMPLE ComputeReconstructedSample(int Px, int ErrVal)
	{
		return FixReconstructedValue(Px + DeQuantize(ErrVal)); 
	}

	inlinehint bool IsNear(int lhs, int rhs) const
		{ return abs(lhs-rhs) <=NEAR; }

	bool IsNear(Triplet lhs, Triplet rhs) const
	{
		return abs(lhs.v1-rhs.v1) <=NEAR && abs(lhs.v2-rhs.v2) <=NEAR && abs(lhs.v3-rhs.v3) <=NEAR; 
	}

	inlinehint int CorrectPrediction(int Pxc) const
	{
		if ((Pxc & MAXVAL) == Pxc)
			return Pxc;
		
		return (~(Pxc >> 31)) & MAXVAL;		
	}

	inlinehint int ModRange(int Errval) const
	{
		ASSERT(abs(Errval) <= RANGE);
		if (Errval < 0)
			Errval = Errval + RANGE;

		if (Errval >= ((RANGE + 1) / 2))
			Errval = Errval - RANGE;

		ASSERT(abs(Errval) <= RANGE/2);

		return Errval;
	}


private:
	int Quantize(int Errval) const
	{
		if (Errval > 0)
			return  (Errval + NEAR) / (2 * NEAR + 1);
		else
			return - (NEAR - Errval) / (2 * NEAR + 1);		
	}


	inlinehint int DeQuantize(int Errval) const
	{
		return Errval * (2 * NEAR + 1);
	}

	inlinehint SAMPLE FixReconstructedValue(int val) const
	{ 
		if (val < -NEAR)
			val = val + RANGE*(2*NEAR+1);
		else if (val > MAXVAL + NEAR)
			val = val - RANGE*(2*NEAR+1);

		return SAMPLE(CorrectPrediction(val)); 
	}

};


