// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_HEADER
#define CHARLS_HEADER

#include <memory>


// Default bin sizes for JPEG-LS statistical modeling. Can be overriden at compression time, however this is rarely done.
const int BASIC_T1 = 3;
const int BASIC_T2 = 7;
const int BASIC_T3 = 21;

const int32_t BASIC_RESET = 64;

struct JlsParameters;
class JpegCustomParameters;

template<class STRATEGY>
class JlsCodecFactory 
{
public:
    std::unique_ptr<STRATEGY> GetCodec(const JlsParameters& info, const JlsCustomParameters&);
private:
    std::unique_ptr<STRATEGY> GetCodecImpl(const JlsParameters& info);
};

JLS_ERROR CheckParameterCoherent(const JlsParameters& pparams);

JlsCustomParameters ComputeDefault(int32_t MAXVAL, int32_t NEAR);


#endif
