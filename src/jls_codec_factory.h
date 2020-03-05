// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.h"

#include <memory>


namespace charls {

class DecoderStrategy;
class EncoderStrategy;

template<typename Strategy>
class JlsCodecFactory final
{
public:
    std::unique_ptr<Strategy> CreateCodec(const frame_info& frame, const coding_parameters& parameters, const jpegls_pc_parameters& preset_coding_parameters);

private:
    std::unique_ptr<Strategy> CreateOptimizedCodec(const frame_info& frame, const coding_parameters& parameters);
};

extern template class JlsCodecFactory<DecoderStrategy>;
extern template class JlsCodecFactory<EncoderStrategy>;

} // namespace charls
