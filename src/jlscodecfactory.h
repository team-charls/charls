// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include <memory>

struct JlsParameters;
struct JpegLSPresetCodingParameters;

template<typename Strategy>
class JlsCodecFactory final
{
public:
    std::unique_ptr<Strategy> CreateCodec(const JlsParameters& params, const JpegLSPresetCodingParameters& presets);

private:
    std::unique_ptr<Strategy> CreateOptimizedCodec(const JlsParameters& params);
};
