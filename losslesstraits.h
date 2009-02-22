// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once

//
// optimized trait classes for lossless compression of 8 bit color and 8/16 bit monochrome images.
// 

template <class sample, int bitsperpixel>
struct LosslessTraitsImplT 
{
	typedef sample SAMPLE;
	enum { 
		NEAR  = 0,
		bpp   = bitsperpixel,
		qbpp  = bitsperpixel,
		RANGE = (1 << bpp),
		MAXVAL= (1 << bpp) - 1,
		LIMIT = 2 * (bitsperpixel + max(8,bitsperpixel)),
		RESET = BASIC_RESET,
	};

	static inlinehint int ComputeErrVal(int d)
	{ return ModRange(d); }
		
	static inlinehint bool IsNear(int lhs, int rhs) 
		{ return lhs == rhs; }

	static inlinehint int ModRange(int Errval) 
	{
		return int(Errval << ((sizeof(int) * 8)  - bpp)) >> ((sizeof(int) * 8)  - bpp); 
	}
	
	static inlinehint SAMPLE ComputeReconstructedSample(int Px, int ErrVal)
	{
		return SAMPLE(MAXVAL & (Px + ErrVal)); 
	}

	static inlinehint int CorrectPrediction(int Pxc) 
	{
		if ((Pxc & MAXVAL) == Pxc)
			return Pxc;
		
		return (~(Pxc >> 31)) & MAXVAL;		
	}

};

template <class SAMPLE, int bpp>
struct LosslessTraitsT : public LosslessTraitsImplT<SAMPLE, bpp> 
{
//	enum { ccomponent = 1};
	typedef SAMPLE PIXEL;
};



template<>
struct LosslessTraitsT<BYTE,8> : public LosslessTraitsImplT<BYTE, 8> 
{
//	enum { ccomponent = 1};
	typedef SAMPLE PIXEL;

	static inlinehint signed char ModRange(int Errval) 
		{ return (signed char)Errval; }

	static inlinehint int ComputeErrVal(int d)
	{ return signed char(d); }

	static inlinehint BYTE ComputeReconstructedSample(int Px, int ErrVal)
		{ return BYTE(Px + ErrVal);  }
	
};



template<>
struct LosslessTraitsT<USHORT,16> : public LosslessTraitsImplT<USHORT,16> 
{
//	enum { ccomponent = 1};
	typedef SAMPLE PIXEL;

	static inlinehint short ModRange(int Errval) 
		{ return short(Errval); }

	static inlinehint int ComputeErrVal(int d)
	{ return short(d); }

	static inlinehint SAMPLE ComputeReconstructedSample(int Px, int ErrVal)
		{ return SAMPLE(Px + ErrVal);  }

};




template<>
struct LosslessTraitsT<Triplet,8> : public LosslessTraitsImplT<BYTE,8>
{
//	enum { 		ccomponent = 3 };
	typedef Triplet PIXEL;

	static inlinehint bool IsNear(int lhs, int rhs) 
		{ return lhs == rhs; }

	static inlinehint bool IsNear(PIXEL lhs, PIXEL rhs) 
		{ return lhs == rhs; }


	static inlinehint SAMPLE ComputeReconstructedSample(int Px, int ErrVal)
		{ return SAMPLE(Px + ErrVal);  }


};
