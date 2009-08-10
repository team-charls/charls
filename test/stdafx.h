// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#ifndef STDAFX
#define STDAFX

#if defined(WIN32)
#pragma warning (disable: 4996)
#endif

// enable ASSERT() on linux 
#ifndef WIN32
#ifndef _DEBUG
#define _DEBUG
#endif
#endif

#include <stdio.h>

typedef unsigned char BYTE;
typedef unsigned short USHORT;

#endif
