// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/public_types.h>

#include <memory>

struct JlsParameters;

namespace charls {

template<typename Strategy>
class JlsCodecFactory final
{
public:
    std::unique_ptr<Strategy> CreateCodec(const JlsParameters& params, const jpegls_pc_parameters& preset_coding_parameters);

private:
    std::unique_ptr<Strategy> CreateOptimizedCodec(const JlsParameters& params);
};

} // namespace charls
