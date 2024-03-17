// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "charls/public_types.h"

namespace charls {

struct coding_parameters final
{
    int32_t near_lossless;
    uint32_t restart_interval;
    charls::interleave_mode interleave_mode;
    color_transformation transformation;

    bool operator==(const coding_parameters& other) const noexcept
    {
        return near_lossless == other.near_lossless && restart_interval == other.restart_interval &&
               interleave_mode == other.interleave_mode && transformation == other.transformation;
    }
};

} // namespace charls
