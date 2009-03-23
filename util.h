// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_UTIL
#define CHARLS_UTIL


#ifdef _DEBUG
#include <assert.h>
#define ASSERT(t) assert(t)
#else
#define ASSERT(t) ;
#endif

#if defined(WIN32)
#define CHARLS_IMEXPORT __declspec(dllexport) 

// default signed int types (32 or 64 bit)
#ifdef  _WIN64
typedef __int64 LONG;
#else
typedef int LONG;
#endif

typedef size_t ULONG;

// for debugging
inline __int64 abs(__int64 value)
{
	return value >= 0 ? value : - value;
}



#else
#include <stdint.h>

// default signed int types (32 or 64 bit)
typedef intptr_t LONG;
typedef uintptr_t ULONG;
#endif



enum constants
{
  LONG_BITCOUNT = sizeof(LONG)*8
};

typedef unsigned char BYTE;
typedef unsigned short USHORT;


#include <string.h>
#include <stdlib.h>


#undef  NEAR

#ifdef _MSC_VER
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


const LONG BASIC_RESET	= 64;

inline LONG log_2(ULONG n)
{
	LONG x = 0;
	while (n > (ULONG(1) << x))
	{
		++x;
	}
	return x;

}

struct Size
{
	Size(LONG width, LONG height) :
		cx(width),
		cy(height)
	{}
	LONG cx;
	LONG cy;
};



inline LONG Sign(LONG n)
	{ return (n >> (LONG_BITCOUNT-1)) | 1;}

inline LONG BitWiseSign(LONG i)
	{ return i >> (LONG_BITCOUNT-1); }	


#pragma pack(push, 1)

struct Triplet 
{ 
	Triplet() :
		v1(0),
		v2(0),
		v3(0)
	{};

	Triplet(LONG x1, LONG x2, LONG x3) :
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
