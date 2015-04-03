// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_HEADER
#define CHARLS_HEADER


#include "publictypes.h"

const int BASIC_RESET = 64;


struct JlsParameters;
class JpegCustomParameters;


JLS_ERROR CheckParameterCoherent(const JlsParameters& pparams);

JlsCustomParameters ComputeDefault(int32_t MAXVAL, int32_t NEAR);


#endif
