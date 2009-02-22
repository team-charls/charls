// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once

#pragma pack (push)
#pragma pack (1)

#undef  NEAR

#ifdef _DEBUG
#define inlinehint inline
#else
#define inlinehint __forceinline
#endif


enum interleavemode
{
	ILV_NONE = 0,
	ILV_LINE = 1,
	ILV_SAMPLE = 2,
};

const int BASIC_RESET	= 64;

inline int log2(UINT n)
{
	int x = 0;
	while (n > (1U << x))
	{
		++x;
	}
	return x;

}

struct Size
{
	Size(int width, int height) :
		cx(width),
		cy(height)
	{}
	int cx;
	int cy;
};



inlinehint int Sign(int n)
	{ return (n >> 31) | 1;}

inlinehint int BitWiseSign(int i)
	{ return (i >> 31); }	


struct Triplet 
{ 
	Triplet() :
		v1(0),
		v2(0),
		v3(0)
	{};

	Triplet(int x1, int x2, int x3) :
		v1((BYTE)x1),
		v2((BYTE)x2),
		v3((BYTE)x3)
	{};

	BYTE v1;
	BYTE v2;
	BYTE v3;
};


#pragma pack (pop)

inline bool operator==(const Triplet& lhs, const Triplet& rhs)
	{ return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3; }

inline bool  operator!=(const Triplet& lhs, const Triplet& rhs)
	{ return !(lhs == rhs); }
