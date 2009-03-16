// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef STDAFX
#define STDAFX

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short USHORT;

#ifdef _DEBUG
#include <assert.h>
#define ASSERT(t) assert(t)
#else
#define ASSERT(t) ;
#endif

#if defined(WIN32)
#define CHARLS_IMEXPORT __declspec(dllexport) 
#endif

#include "util.h"

#endif
