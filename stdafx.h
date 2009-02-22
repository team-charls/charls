// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short USHORT;


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifdef _DEBUG
#include <assert.h>
#define ASSERT(t) assert(t)
#else
#define ASSERT(t) ;
#endif

