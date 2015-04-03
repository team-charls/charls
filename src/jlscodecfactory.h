//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#pragma once

#include <memory>

struct JlsCustomParameters;
struct JlsParameters;

template<class STRATEGY>
class JlsCodecFactory
{
public:
    std::unique_ptr<STRATEGY> GetCodec(const JlsParameters& info, const JlsCustomParameters&);
private:
    std::unique_ptr<STRATEGY> GetCodecImpl(const JlsParameters& info);
};
