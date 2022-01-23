// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.h"

#include <memory>


namespace charls {

class decoder_strategy;
class encoder_strategy;

template<typename Strategy>
class jls_codec_factory final
{
public:
    std::unique_ptr<Strategy> create_codec(const frame_info& frame, const coding_parameters& parameters,
                                           const jpegls_pc_parameters& preset_coding_parameters);

private:
    std::unique_ptr<Strategy> try_create_optimized_codec(const frame_info& frame, const coding_parameters& parameters);
};

#ifndef _MSC_VER // IntelliSense fails to parse next lines and __INTELLISENSE__ cannot exclude it. Not needed for MSVC build.
extern template class jls_codec_factory<decoder_strategy>;
extern template class jls_codec_factory<encoder_strategy>;
#endif

} // namespace charls
