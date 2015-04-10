/* 
 (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
*/ 


#ifndef CHARLS_CONFIG
#define CHARLS_CONFIG


#ifdef NDEBUG
#  ifndef ASSERT
#    define ASSERT(t) { }
#  endif
#else
#include <cassert>
#define ASSERT(t) assert(t)
#endif

#if defined(_WIN32)
#ifdef _MSC_VER
#pragma warning (disable:4512) // assignment operator could not be generated [VS2013]
#endif

#endif


#undef  NEAR

#ifndef inlinehint
#  ifdef _MSC_VER
#    ifdef NDEBUG
#      define inlinehint __forceinline
#    else
#      define inlinehint inline
#    endif
#  elif defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#    define inlinehint inline
#  else 
#    define inlinehint inline
#  endif
#endif


#endif
