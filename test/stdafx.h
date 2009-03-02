// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <tchar.h>

#ifdef _DEBUG
#include <assert.h>
#define ASSERT(t) assert(t)
#else
#define ASSERT(t) ;
#endif

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short USHORT;

// TODO: reference additional headers your program requires here
