// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_UTIL
#define CHARLS_UTIL



#undef  NEAR

#ifdef _USRDLL
#ifdef _DEBUG
#define inlinehint 
#else
#define inlinehint __forceinline
#endif
#else
#define inlinehint __inline
#endif




#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif


const int BASIC_RESET	= 64;

inline int log_2(UINT n)
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



inline int Sign(int n)
	{ return (n >> 31) | 1;}

inline int BitWiseSign(int i)
	{ return (i >> 31); }	


#pragma pack(push, 1)

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


#pragma pack(pop)


#include "interface.h"

inline bool operator==(const Triplet& lhs, const Triplet& rhs)
	{ return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3; }

inline bool  operator!=(const Triplet& lhs, const Triplet& rhs)
	{ return !(lhs == rhs); }

#endif
