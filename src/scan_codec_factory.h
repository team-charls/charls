// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.h"

#include <memory>


namespace charls {

template<typename ScanProcess>
class scan_codec_factory final
{
public:
    std::unique_ptr<ScanProcess> create_codec(const frame_info& frame, const coding_parameters& parameters,
                                              const jpegls_pc_parameters& preset_coding_parameters);

private:
    std::unique_ptr<ScanProcess> try_create_optimized_codec(const frame_info& frame, const coding_parameters& parameters);
};


extern template class scan_codec_factory<class scan_decoder>;
extern template class scan_codec_factory<class scan_encoder>;

} // namespace charls
