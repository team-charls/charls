//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#ifndef CHARLS_JLSCODECFACTORY
#define CHARLS_JLSCODECFACTORY

#include <memory>

struct JpegLSPresetCodingParameters;
struct JlsParameters;

template<typename Strategy>
class JlsCodecFactory
{
public:
    std::unique_ptr<Strategy> GetCodec(const JlsParameters& params, const JpegLSPresetCodingParameters&);

private:
    std::unique_ptr<Strategy> GetCodecImpl(const JlsParameters& params);
};

#endif
