// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <memory>

struct JlsParameters;
struct JpegLSPresetCodingParameters;

namespace charls {

template<typename Strategy>
class JlsCodecFactory final
{
public:
    std::unique_ptr<Strategy> CreateCodec(const JlsParameters& params, const JpegLSPresetCodingParameters& presets);

private:
    std::unique_ptr<Strategy> CreateOptimizedCodec(const JlsParameters& params);
};

} // namespace charls
